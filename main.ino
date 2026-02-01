#include <TinyGPS++.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <BH1750.h>
#include <Servo.h>

/* ===================== GPS ===================== */
static const uint32_t GPSBaud = 9600;
SoftwareSerial ss(2, 3);   // RX, TX
TinyGPSPlus gps;

/* ===================== LCD ===================== */
LiquidCrystal_I2C lcd1(0x27, 16, 2); // Biodegradable
LiquidCrystal_I2C lcd2(0x25, 16, 2); // Non-Biodegradable

/* ===================== BH1750 ===================== */
BH1750 lightMeter;
float currentLux = 0;
const float LUX_THRESHOLD = 50.0;
unsigned long lastLuxRead = 0;
const unsigned long LUX_INTERVAL = 1000;
bool ambientLEDOn = false;

/* ===================== ULTRASONIC ===================== */
// Human detection sensors
const int trigBio = 4;
const int echoBio = 5;
const int trigNonBio = 6;
const int echoNonBio = 7;

// Bin full detection sensors
const int trigBioFull = A0;
const int echoBioFull = A1;
const int trigNonBioFull = A2;
const int echoNonBioFull = A3;

const long HUMAN_DETECT_THRESHOLD = 50; // cm
const long MAX_VALID_DISTANCE = 300;    // cm
const long FULL_THRESHOLD = 100;        // cm for full detection
const int DIST_SAMPLES = 7;

/* ===================== RELAY ===================== */
const int relayLED = 8;

/* ===================== SERVOS ===================== */
const int servoBioPin = 11;
const int servoNonBioPin = 12;
Servo servoBio;
Servo servoNonBio;

const unsigned long SERVO_OPEN_TIME = 1000;   // ms to fully open
const unsigned long SERVO_CLOSE_TIME = 1000;  // ms to fully close
const unsigned long SERVO_CLOSE_DELAY = 3000; // ms after human leaves

bool bioServoOpen = false;
bool nonBioServoOpen = false;
unsigned long bioServoTimer = 0;
unsigned long nonBioServoTimer = 0;
bool bioStopWritten = true;
bool nonBioStopWritten = true;

/* ===================== STABLE FULL DETECTION ===================== */
bool bioFull = false;
bool nonBioFull = false;

const int FULL_BUFFER_SIZE = 5; // moving average buffer
long bioFullBuffer[FULL_BUFFER_SIZE] = {MAX_VALID_DISTANCE+1};
long nonBioFullBuffer[FULL_BUFFER_SIZE] = {MAX_VALID_DISTANCE+1};
int bioIndex = 0, nonBioIndex = 0;

/* ===================== SMS ===================== */
const char phoneNumber[] = "+639567669410"; // manually include carrier number
bool bioSMSSent = false;
bool nonBioSMSSent = false;
SoftwareSerial sim800(7, 8); // RX, TX (change pins if needed)

/* ===================== SETUP ===================== */
void setup() {
  Serial.begin(9600);
  ss.begin(GPSBaud);
  Wire.begin();

  // LCD
  lcd1.init(); lcd1.backlight();
  lcd2.init(); lcd2.backlight();
  lcd1.print("Biodegradable"); lcd1.setCursor(0,1); lcd1.print("Dist: ---");
  lcd2.print("Non-Biodegradable"); lcd2.setCursor(0,1); lcd2.print("Dist: ---");

  // Ultrasonic pins
  pinMode(trigBio, OUTPUT); pinMode(echoBio, INPUT);
  pinMode(trigNonBio, OUTPUT); pinMode(echoNonBio, INPUT);
  pinMode(trigBioFull, OUTPUT); pinMode(echoBioFull, INPUT);
  pinMode(trigNonBioFull, OUTPUT); pinMode(echoNonBioFull, INPUT);

  // Relay
  pinMode(relayLED, OUTPUT);
  digitalWrite(relayLED, HIGH);

  // Servos
  servoBio.attach(servoBioPin); servoBio.write(90);      
  servoNonBio.attach(servoNonBioPin); servoNonBio.write(90); 

  // Light sensor
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  // SIM800A setup
  sim800.begin(9600);
  delay(1000);
  sim800.println("AT"); delay(100); sim800.println("AT+CMGF=1"); delay(100); // SMS text mode

  Serial.println("SYSTEM READY");
}

/* ===================== LIGHT SENSOR ===================== */
void updateAmbientLight() {
  if (millis() - lastLuxRead >= LUX_INTERVAL) {
    lastLuxRead = millis();
    currentLux = lightMeter.readLightLevel();
    ambientLEDOn = (currentLux < LUX_THRESHOLD);
  }
}

/* ===================== MEDIAN DISTANCE ===================== */
long readDistanceStable(int trigPin, int echoPin) {
  long readings[DIST_SAMPLES];
  for (int i = 0; i < DIST_SAMPLES; i++) {
    digitalWrite(trigPin, LOW); delayMicroseconds(2);
    digitalWrite(trigPin, HIGH); delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long dur = pulseIn(echoPin, HIGH, 50000);
    readings[i] = (dur > 0) ? dur * 0.034 / 2 : MAX_VALID_DISTANCE + 1;
    delay(5);
  }
  // median
  for (int i = 0; i < DIST_SAMPLES - 1; i++)
    for (int j = i + 1; j < DIST_SAMPLES; j++)
      if (readings[i] > readings[j]) { long t = readings[i]; readings[i] = readings[j]; readings[j] = t; }
  return readings[DIST_SAMPLES/2];
}

/* ===================== MOVING AVERAGE FOR FULL DETECTION ===================== */
long getStableDistance(long* buffer, int size, int& index, long newValue) {
  buffer[index] = newValue;
  index = (index + 1) % size;
  long sum = 0;
  for (int i = 0; i < size; i++) sum += buffer[i];
  return sum / size;
}

/* ===================== SEND SMS ===================== */
void sendSMS(const char* number, const char* message) {
  sim800.print("AT+CMGS=\"");
  sim800.print(number);
  sim800.println("\"");
  delay(100);
  sim800.print(message);
  delay(100);
  sim800.write(26); // Ctrl+Z to send
  delay(500);
  Serial.print("SMS SENT: "); Serial.println(message);
}

/* ===================== LOOP ===================== */
void loop() {
  unsigned long now = millis();

  // GPS
  while (ss.available()) gps.encode(ss.read());

  // Light sensor
  updateAmbientLight();

  // Human detection distances
  long dBio = readDistanceStable(trigBio, echoBio);
  long dNonBio = readDistanceStable(trigNonBio, echoNonBio);

  // Bin full raw distances
  long dBioFullRaw = readDistanceStable(trigBioFull, echoBioFull);
  long dNonBioFullRaw = readDistanceStable(trigNonBioFull, echoNonBioFull);

  // Moving average for stable full detection
  long dBioFullAvg = getStableDistance(bioFullBuffer, FULL_BUFFER_SIZE, bioIndex, dBioFullRaw);
  long dNonBioFullAvg = getStableDistance(nonBioFullBuffer, FULL_BUFFER_SIZE, nonBioIndex, dNonBioFullRaw);

  // Set full status if average below threshold
  bool prevBioFull = bioFull;
  bool prevNonBioFull = nonBioFull;

  bioFull = (dBioFullAvg <= FULL_THRESHOLD);
  nonBioFull = (dNonBioFullAvg <= FULL_THRESHOLD);

  // ----------- SEND SMS WHEN FULL ---------
  if(bioFull && !bioSMSSent){
    sendSMS(phoneNumber, "Biodegradable bin is FULL!");
    bioSMSSent = true;
  } else if(!bioFull) {
    bioSMSSent = false; // reset when emptied
  }

  if(nonBioFull && !nonBioSMSSent){
    sendSMS(phoneNumber, "Non-Biodegradable bin is FULL!");
    nonBioSMSSent = true;
  } else if(!nonBioFull) {
    nonBioSMSSent = false; // reset when emptied
  }

  bool bioHumanPresent = (dBio <= HUMAN_DETECT_THRESHOLD);
  bool nonBioHumanPresent = (dNonBio <= HUMAN_DETECT_THRESHOLD);

  // --------- BIO SERVO ---------
  if (bioHumanPresent && !bioServoOpen) {
    servoBio.write(0);          
    delay(SERVO_OPEN_TIME);
    servoBio.write(90);         
    bioServoOpen = true;
    bioServoTimer = now;
    bioStopWritten = true;      
    Serial.println("BIO SERVO OPEN");
  } 
  else if (!bioHumanPresent && bioServoOpen && now - bioServoTimer > SERVO_CLOSE_DELAY) {
    servoBio.write(180);        
    delay(SERVO_CLOSE_TIME);
    servoBio.write(90);         
    bioServoOpen = false;
    bioStopWritten = true;
    Serial.println("BIO SERVO CLOSED");
  }

  // --------- NON-BIO SERVO ---------
  if (nonBioHumanPresent && !nonBioServoOpen) {
    servoNonBio.write(0);       
    delay(SERVO_OPEN_TIME);
    servoNonBio.write(90);      
    nonBioServoOpen = true;
    nonBioServoTimer = now;
    nonBioStopWritten = true;
    Serial.println("NON-BIO SERVO OPEN");
  } 
  else if (!nonBioHumanPresent && nonBioServoOpen && now - nonBioServoTimer > SERVO_CLOSE_DELAY) {
    servoNonBio.write(180);     
    delay(SERVO_CLOSE_TIME);
    servoNonBio.write(90);      
    nonBioServoOpen = false;
    nonBioStopWritten = true;
    Serial.println("NON-BIO SERVO CLOSED");
  }

  // Ambient LED
  digitalWrite(relayLED, ambientLEDOn ? LOW : HIGH);

  // LCD update
  static unsigned long lastDisplayUpdate = 0;
  if(now - lastDisplayUpdate >= 500){
    lcd1.setCursor(0,1);
    lcd1.print("Dist: "); 
    if(dBio<MAX_VALID_DISTANCE) lcd1.print(dBio); else lcd1.print("---"); 
    lcd1.print("cm ");
    if(bioFull) lcd1.print("FULL"); else lcd1.print("     ");

    lcd2.setCursor(0,1);
    lcd2.print("Dist: "); 
    if(dNonBio<MAX_VALID_DISTANCE) lcd2.print(dNonBio); else lcd2.print("---"); 
    lcd2.print("cm ");
    if(nonBioFull) lcd2.print("FULL"); else lcd2.print("     ");

    lastDisplayUpdate = now;
  }

  // Debug
  Serial.print("dBio: "); Serial.print(dBio); 
  Serial.print(" | dNonBio: "); Serial.print(dNonBio);
  Serial.print(" | dBioFullAvg: "); Serial.print(dBioFullAvg);
  Serial.print(" | dNonBioFullAvg: "); Serial.println(dNonBioFullAvg);

  delay(100); 
}
