#include <TinyGPS++.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <BH1750.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>

/* ===================== GPS ===================== */
// Uses Hardware Serial (pins 0, 1)
// NOTE: Serial Monitor won't work when GPS is connected!
// For debugging, temporarily disconnect GPS
TinyGPSPlus gps;
static const uint32_t GPSBaud = 9600;

/* ===================== LCD ===================== */
LiquidCrystal_I2C lcd1(0x27, 16, 2); // Biodegradable
LiquidCrystal_I2C lcd2(0x25, 16, 2); // Non-Biodegradable

/* ===================== BH1750 (Light Sensor) ===================== */
BH1750 lightMeter;
float currentLux = 0;
const float LUX_THRESHOLD = 50.0;
unsigned long lastLuxRead = 0;
const unsigned long LUX_INTERVAL = 1000;
bool ambientLEDOn = false;

/* ===================== DUAL RFID (MFRC522) ===================== */
// Shared RST pin for both RFID modules
#define SHARED_RST_PIN 9

// Biodegradable bin RFID
#define RFID_BIO_SS_PIN 8
MFRC522 rfidBio(RFID_BIO_SS_PIN, SHARED_RST_PIN);

// Non-Biodegradable bin RFID
#define RFID_NONBIO_SS_PIN 10
MFRC522 rfidNonBio(RFID_NONBIO_SS_PIN, SHARED_RST_PIN);

// Authorized RFID UIDs (add your card/tag UIDs here)
String authorizedUIDs[] = {
  "AA BB CC DD",  // Example UID 1
  "11 22 33 44"   // Example UID 2
};
const int numAuthorizedUIDs = 2;

/* ===================== ULTRASONIC (Bin Level Detection) ===================== */
const int trigBioFull = A0;
const int echoBioFull = A1;
const int trigNonBioFull = A2;
const int echoNonBioFull = A3;

const long MAX_VALID_DISTANCE = 300;    // cm
const long FULL_THRESHOLD = 10;         // cm for full detection (closer = fuller)
const int DIST_SAMPLES = 7;

/* ===================== RELAY (LED Control) ===================== */
const int relayLED = 3;

/* ===================== BUZZER ===================== */
const int buzzerPin = 2;

// Buzzer patterns
void beepSuccess() {
  tone(buzzerPin, 2000, 100);  // 2kHz for 100ms
  delay(150);
  tone(buzzerPin, 2500, 100);
}

void beepError() {
  tone(buzzerPin, 500, 200);   // Low tone for 200ms
  delay(250);
  tone(buzzerPin, 500, 200);
}

void beepFull() {
  for (int i = 0; i < 3; i++) {
    tone(buzzerPin, 1500, 100);
    delay(150);
  }
}

void beepShort() {
  tone(buzzerPin, 2000, 50);
}

/* ===================== SERVOS (Lock Mechanism) ===================== */
const int servoBioPin = 6;
const int servoNonBioPin = 7;
Servo servoBio;
Servo servoNonBio;

const int SERVO_LOCKED = 0;    // Locked position
const int SERVO_UNLOCKED = 90; // Unlocked position

bool bioLocked = false;
bool nonBioLocked = false;

/* ===================== STABLE FULL DETECTION ===================== */
bool bioFull = false;
bool nonBioFull = false;

const int FULL_BUFFER_SIZE = 5; // moving average buffer
long bioFullBuffer[FULL_BUFFER_SIZE] = {MAX_VALID_DISTANCE+1};
long nonBioFullBuffer[FULL_BUFFER_SIZE] = {MAX_VALID_DISTANCE+1};
int bioIndex = 0, nonBioIndex = 0;

/* ===================== SMS ===================== */
const char phoneNumber[] = "+639567669410";
bool bioSMSSent = false;
bool nonBioSMSSent = false;
SoftwareSerial sim800(4, 5); // RX=4, TX=5

/* ===================== SETUP ===================== */
void setup() {
  // Initialize Hardware Serial for GPS
  Serial.begin(GPSBaud);
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize SPI for RFID
  SPI.begin();

  // LCD Setup
  lcd1.init(); 
  lcd1.backlight();
  lcd2.init(); 
  lcd2.backlight();
  
  lcd1.clear();
  lcd2.clear();
  lcd1.print("Biodegradable");
  lcd2.print("Non-Biodegrad.");
  delay(1500);
  
  lcd1.setCursor(0,1); 
  lcd1.print("Initializing...");
  lcd2.setCursor(0,1); 
  lcd2.print("Initializing...");

  // RFID Setup
  rfidBio.PCD_Init();
  delay(10);
  rfidNonBio.PCD_Init();
  
  // Buzzer
  pinMode(buzzerPin, OUTPUT);
  beepShort(); // Startup beep

  // Ultrasonic pins
  pinMode(trigBioFull, OUTPUT); 
  pinMode(echoBioFull, INPUT);
  pinMode(trigNonBioFull, OUTPUT); 
  pinMode(echoNonBioFull, INPUT);

  // Relay
  pinMode(relayLED, OUTPUT);
  digitalWrite(relayLED, HIGH); // LED OFF initially (active LOW relay)

  // Servos - Start UNLOCKED
  servoBio.attach(servoBioPin);
  servoBio.write(SERVO_UNLOCKED);
  servoNonBio.attach(servoNonBioPin);
  servoNonBio.write(SERVO_UNLOCKED);

  // Light sensor
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  // SIM800A setup
  sim800.begin(9600);
  delay(1000);
  sim800.println("AT"); 
  delay(100);
  sim800.println("AT+CMGF=1"); // SMS text mode
  delay(100);

  lcd1.setCursor(0,1); 
  lcd1.print("Ready - OPEN   ");
  lcd2.setCursor(0,1); 
  lcd2.print("Ready - OPEN   ");
  
  beepSuccess(); // Ready beep
}

/* ===================== RFID CHECKING ===================== */
void checkRFID() {
  // Check Biodegradable bin RFID
  if (rfidBio.PICC_IsNewCardPresent() && rfidBio.PICC_ReadCardSerial()) {
    String cardUID = getUID(rfidBio);
    processCard(cardUID, "BIO");
    rfidBio.PICC_HaltA();
    rfidBio.PCD_StopCrypto1();
  }

  // Check Non-Biodegradable bin RFID
  if (rfidNonBio.PICC_IsNewCardPresent() && rfidNonBio.PICC_ReadCardSerial()) {
    String cardUID = getUID(rfidNonBio);
    processCard(cardUID, "NON-BIO");
    rfidNonBio.PICC_HaltA();
    rfidNonBio.PCD_StopCrypto1();
  }
}

/* ===================== Get RFID UID ===================== */
String getUID(MFRC522 &rfid) {
  String cardUID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardUID += String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    cardUID += String(rfid.uid.uidByte[i], HEX);
  }
  cardUID.trim();
  cardUID.toUpperCase();
  return cardUID;
}

/* ===================== Process RFID Card ===================== */
void processCard(String cardUID, String binType) {
  bool authorized = false;
  
  // Check if authorized
  for (int i = 0; i < numAuthorizedUIDs; i++) {
    if (cardUID == authorizedUIDs[i]) {
      authorized = true;
      break;
    }
  }

  if (authorized) {
    beepSuccess();
    if (binType == "BIO") {
      lcd1.setCursor(0,1); 
      lcd1.print("User: OK       ");
    } else {
      lcd2.setCursor(0,1); 
      lcd2.print("User: OK       ");
    }
  } else {
    beepError();
    if (binType == "BIO") {
      lcd1.setCursor(0,1); 
      lcd1.print("Unknown user   ");
    } else {
      lcd2.setCursor(0,1); 
      lcd2.print("Unknown user   ");
    }
  }
  
  delay(1500); // Show message for 1.5 seconds
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
    digitalWrite(trigPin, LOW); 
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH); 
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    long dur = pulseIn(echoPin, HIGH, 50000);
    readings[i] = (dur > 0) ? dur * 0.034 / 2 : MAX_VALID_DISTANCE + 1;
    delay(5);
  }
  
  // Median sort
  for (int i = 0; i < DIST_SAMPLES - 1; i++) {
    for (int j = i + 1; j < DIST_SAMPLES; j++) {
      if (readings[i] > readings[j]) {
        long t = readings[i];
        readings[i] = readings[j];
        readings[j] = t;
      }
    }
  }
  
  return readings[DIST_SAMPLES/2];
}

/* ===================== MOVING AVERAGE ===================== */
long getStableDistance(long* buffer, int size, int& index, long newValue) {
  buffer[index] = newValue;
  index = (index + 1) % size;
  
  long sum = 0;
  for (int i = 0; i < size; i++) {
    sum += buffer[i];
  }
  
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
}

/* ===================== MAIN LOOP ===================== */
void loop() {
  unsigned long now = millis();

  // GPS data reading (Hardware Serial)
  while (Serial.available()) {
    gps.encode(Serial.read());
  }

  // RFID check
  checkRFID();

  // Light sensor
  updateAmbientLight();

  // Bin level detection
  long dBioFullRaw = readDistanceStable(trigBioFull, echoBioFull);
  long dNonBioFullRaw = readDistanceStable(trigNonBioFull, echoNonBioFull);

  // Moving average for stable detection
  long dBioFullAvg = getStableDistance(bioFullBuffer, FULL_BUFFER_SIZE, bioIndex, dBioFullRaw);
  long dNonBioFullAvg = getStableDistance(nonBioFullBuffer, FULL_BUFFER_SIZE, nonBioIndex, dNonBioFullRaw);

  // Determine full status (distance only, no weight sensor)
  bioFull = (dBioFullAvg <= FULL_THRESHOLD);
  nonBioFull = (dNonBioFullAvg <= FULL_THRESHOLD);

  // ----------- LOCK CONTROL ---------
  // Biodegradable bin
  if (bioFull && !bioLocked) {
    servoBio.write(SERVO_LOCKED);
    bioLocked = true;
    beepFull();
  } else if (!bioFull && bioLocked) {
    servoBio.write(SERVO_UNLOCKED);
    bioLocked = false;
    beepSuccess();
  }

  // Non-Biodegradable bin
  if (nonBioFull && !nonBioLocked) {
    servoNonBio.write(SERVO_LOCKED);
    nonBioLocked = true;
    beepFull();
  } else if (!nonBioFull && nonBioLocked) {
    servoNonBio.write(SERVO_UNLOCKED);
    nonBioLocked = false;
    beepSuccess();
  }

  // ----------- SMS ALERTS ---------
  if (bioFull && !bioSMSSent) {
    String msg = "ALERT! Biodegradable bin is FULL. Distance: ";
    msg += String(dBioFullAvg);
    msg += "cm. Location: ";
    if (gps.location.isValid()) {
      msg += String(gps.location.lat(), 6);
      msg += ",";
      msg += String(gps.location.lng(), 6);
    } else {
      msg += "GPS unavailable";
    }
    sendSMS(phoneNumber, msg.c_str());
    bioSMSSent = true;
  } else if (!bioFull) {
    bioSMSSent = false;
  }

  if (nonBioFull && !nonBioSMSSent) {
    String msg = "ALERT! Non-Biodegradable bin is FULL. Distance: ";
    msg += String(dNonBioFullAvg);
    msg += "cm. Location: ";
    if (gps.location.isValid()) {
      msg += String(gps.location.lat(), 6);
      msg += ",";
      msg += String(gps.location.lng(), 6);
    } else {
      msg += "GPS unavailable";
    }
    sendSMS(phoneNumber, msg.c_str());
    nonBioSMSSent = true;
  } else if (!nonBioFull) {
    nonBioSMSSent = false;
  }

  // Ambient LED control
  digitalWrite(relayLED, ambientLEDOn ? LOW : HIGH);

  // LCD update
  static unsigned long lastDisplayUpdate = 0;
  if (now - lastDisplayUpdate >= 500) {
    // LCD1 - Biodegradable
    lcd1.setCursor(0, 1);
    lcd1.print(bioLocked ? "LOCKED " : "OPEN   ");
    lcd1.print(String(dBioFullAvg) + "cm");
    if (bioFull) lcd1.print(" FULL");
    else lcd1.print("     ");

    // LCD2 - Non-Biodegradable
    lcd2.setCursor(0, 1);
    lcd2.print(nonBioLocked ? "LOCKED " : "OPEN   ");
    lcd2.print(String(dNonBioFullAvg) + "cm");
    if (nonBioFull) lcd2.print(" FULL");
    else lcd2.print("     ");

    lastDisplayUpdate = now;
  }

  delay(100);
}
