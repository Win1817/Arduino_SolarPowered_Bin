#include <TinyGPS++.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <BH1750.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HX711.h>

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

/* ===================== RFID (MFRC522) ===================== */
#define RST_PIN 5
#define SS_PIN 53
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Authorized RFID UIDs (add your card UIDs here)
String authorizedUIDs[] = {
  "AA BB CC DD",  // Example UID 1
  "11 22 33 44"   // Example UID 2
};
const int numAuthorizedUIDs = 2;

/* ===================== WEIGHT SENSORS (HX711) ===================== */
// Biodegradable bin weight sensor
#define BIO_DOUT_PIN 4
#define BIO_SCK_PIN 6
HX711 scaleBio;
float bioWeight = 0;
const float BIO_WEIGHT_THRESHOLD = 5000.0; // grams (5 kg)

// Non-Biodegradable bin weight sensor
#define NONBIO_DOUT_PIN 7
#define NONBIO_SCK_PIN A4
HX711 scaleNonBio;
float nonBioWeight = 0;
const float NONBIO_WEIGHT_THRESHOLD = 5000.0; // grams (5 kg)

// Calibration factors (adjust these based on your load cell)
const float BIO_CALIBRATION_FACTOR = -7050.0;
const float NONBIO_CALIBRATION_FACTOR = -7050.0;

/* ===================== ULTRASONIC ===================== */
// Bin full detection sensors
const int trigBioFull = A0;
const int echoBioFull = A1;
const int trigNonBioFull = A2;
const int echoNonBioFull = A3;

const long MAX_VALID_DISTANCE = 300;    // cm
const long FULL_THRESHOLD = 100;        // cm for full detection
const int DIST_SAMPLES = 7;

/* ===================== RELAY ===================== */
const int relayLED = 8;

/* ===================== SERVOS (LOCK MECHANISM) ===================== */
const int servoBioPin = 11;
const int servoNonBioPin = 12;
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
SoftwareSerial sim800(9, 10); // RX, TX

/* ===================== SETUP ===================== */
void setup() {
  Serial.begin(9600);
  ss.begin(GPSBaud);
  Wire.begin();
  SPI.begin();

  // LCD
  lcd1.init(); lcd1.backlight();
  lcd2.init(); lcd2.backlight();
  lcd1.print("Biodegradable");
  lcd2.print("Non-Biodegrad.");
  delay(1000);
  lcd1.setCursor(0,1); lcd1.print("Initializing...");
  lcd2.setCursor(0,1); lcd2.print("Initializing...");

  // RFID
  mfrc522.PCD_Init();
  Serial.println("RFID Ready");

  // Weight sensors
  scaleBio.begin(BIO_DOUT_PIN, BIO_SCK_PIN);
  scaleBio.set_scale(BIO_CALIBRATION_FACTOR);
  scaleBio.tare(); // Reset to zero
  
  scaleNonBio.begin(NONBIO_DOUT_PIN, NONBIO_SCK_PIN);
  scaleNonBio.set_scale(NONBIO_CALIBRATION_FACTOR);
  scaleNonBio.tare(); // Reset to zero
  
  Serial.println("Weight sensors calibrated");

  // Ultrasonic pins (only full detection sensors)
  pinMode(trigBioFull, OUTPUT); pinMode(echoBioFull, INPUT);
  pinMode(trigNonBioFull, OUTPUT); pinMode(echoNonBioFull, INPUT);

  // Relay
  pinMode(relayLED, OUTPUT);
  digitalWrite(relayLED, HIGH);

  // Servos - Start UNLOCKED
  servoBio.attach(servoBioPin);
  servoBio.write(SERVO_UNLOCKED);
  servoNonBio.attach(servoNonBioPin);
  servoNonBio.write(SERVO_UNLOCKED);
  
  Serial.println("Servos UNLOCKED");

  // Light sensor
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  // SIM800A setup
  sim800.begin(9600);
  delay(1000);
  sim800.println("AT"); delay(100);
  sim800.println("AT+CMGF=1"); delay(100); // SMS text mode

  lcd1.setCursor(0,1); lcd1.print("Ready - OPEN   ");
  lcd2.setCursor(0,1); lcd2.print("Ready - OPEN   ");
  
  Serial.println("SYSTEM READY");
}

/* ===================== RFID USER LOGGING ===================== */
void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Read UID
  String cardUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    cardUID += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    cardUID += String(mfrc522.uid.uidByte[i], HEX);
  }
  cardUID.trim();
  cardUID.toUpperCase();

  Serial.print("Card UID: ");
  Serial.println(cardUID);

  // Check if authorized (for logging purposes)
  bool authorized = false;
  for (int i = 0; i < numAuthorizedUIDs; i++) {
    if (cardUID == authorizedUIDs[i]) {
      authorized = true;
      break;
    }
  }

  if (authorized) {
    Serial.println("AUTHORIZED USER - Access logged");
    lcd1.setCursor(0,1); lcd1.print("User logged    ");
    lcd2.setCursor(0,1); lcd2.print("User logged    ");
    delay(1000);
  } else {
    Serial.println("UNKNOWN USER - Access logged");
    lcd1.setCursor(0,1); lcd1.print("Unknown user   ");
    lcd2.setCursor(0,1); lcd2.print("Unknown user   ");
    delay(1000);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

/* ===================== LIGHT SENSOR ===================== */
void updateAmbientLight() {
  if (millis() - lastLuxRead >= LUX_INTERVAL) {
    lastLuxRead = millis();
    currentLux = lightMeter.readLightLevel();
    ambientLEDOn = (currentLux < LUX_THRESHOLD);
  }
}

/* ===================== WEIGHT READING ===================== */
void updateWeights() {
  if (scaleBio.is_ready()) {
    bioWeight = scaleBio.get_units(5); // Average of 5 readings
    if (bioWeight < 0) bioWeight = 0; // Prevent negative weights
  }
  
  if (scaleNonBio.is_ready()) {
    nonBioWeight = scaleNonBio.get_units(5); // Average of 5 readings
    if (nonBioWeight < 0) nonBioWeight = 0; // Prevent negative weights
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
  // median sort
  for (int i = 0; i < DIST_SAMPLES - 1; i++)
    for (int j = i + 1; j < DIST_SAMPLES; j++)
      if (readings[i] > readings[j]) {
        long t = readings[i];
        readings[i] = readings[j];
        readings[j] = t;
      }
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
  Serial.print("SMS SENT: ");
  Serial.println(message);
}

/* ===================== LOOP ===================== */
void loop() {
  unsigned long now = millis();

  // GPS
  while (ss.available()) gps.encode(ss.read());

  // RFID check (for user logging only)
  checkRFID();

  // Light sensor
  updateAmbientLight();

  // Weight sensors
  updateWeights();

  // Bin full raw distances
  long dBioFullRaw = readDistanceStable(trigBioFull, echoBioFull);
  long dNonBioFullRaw = readDistanceStable(trigNonBioFull, echoNonBioFull);

  // Moving average for stable full detection
  long dBioFullAvg = getStableDistance(bioFullBuffer, FULL_BUFFER_SIZE, bioIndex, dBioFullRaw);
  long dNonBioFullAvg = getStableDistance(nonBioFullBuffer, FULL_BUFFER_SIZE, nonBioIndex, dNonBioFullRaw);

  // Set full status (using both distance and weight)
  bioFull = (dBioFullAvg <= FULL_THRESHOLD) || (bioWeight >= BIO_WEIGHT_THRESHOLD);
  nonBioFull = (dNonBioFullAvg <= FULL_THRESHOLD) || (nonBioWeight >= NONBIO_WEIGHT_THRESHOLD);

  // ----------- LOCK CONTROL BASED ON FULL STATUS ---------
  // Biodegradable bin
  if (bioFull && !bioLocked) {
    servoBio.write(SERVO_LOCKED);
    bioLocked = true;
    Serial.println("BIO BIN LOCKED - FULL");
  } else if (!bioFull && bioLocked) {
    servoBio.write(SERVO_UNLOCKED);
    bioLocked = false;
    Serial.println("BIO BIN UNLOCKED - Not full");
  }

  // Non-Biodegradable bin
  if (nonBioFull && !nonBioLocked) {
    servoNonBio.write(SERVO_LOCKED);
    nonBioLocked = true;
    Serial.println("NON-BIO BIN LOCKED - FULL");
  } else if (!nonBioFull && nonBioLocked) {
    servoNonBio.write(SERVO_UNLOCKED);
    nonBioLocked = false;
    Serial.println("NON-BIO BIN UNLOCKED - Not full");
  }

  // ----------- SEND SMS WHEN FULL ---------
  if (bioFull && !bioSMSSent) {
    String msg = "Biodegradable bin FULL! Weight: ";
    msg += String(bioWeight, 1);
    msg += "g";
    sendSMS(phoneNumber, msg.c_str());
    bioSMSSent = true;
  } else if (!bioFull) {
    bioSMSSent = false; // reset when emptied
  }

  if (nonBioFull && !nonBioSMSSent) {
    String msg = "Non-Biodegradable bin FULL! Weight: ";
    msg += String(nonBioWeight, 1);
    msg += "g";
    sendSMS(phoneNumber, msg.c_str());
    nonBioSMSSent = true;
  } else if (!nonBioFull) {
    nonBioSMSSent = false; // reset when emptied
  }

  // Ambient LED
  digitalWrite(relayLED, ambientLEDOn ? LOW : HIGH);

  // LCD update
  static unsigned long lastDisplayUpdate = 0;
  if (now - lastDisplayUpdate >= 500) {
    // LCD1 - Biodegradable
    lcd1.setCursor(0, 1);
    if (bioLocked) {
      lcd1.print("LOCKED ");
    } else {
      lcd1.print("OPEN   ");
    }
    lcd1.print(String(bioWeight, 0) + "g");
    lcd1.print(bioFull ? " FULL" : "     ");

    // LCD2 - Non-Biodegradable
    lcd2.setCursor(0, 1);
    if (nonBioLocked) {
      lcd2.print("LOCKED ");
    } else {
      lcd2.print("OPEN   ");
    }
    lcd2.print(String(nonBioWeight, 0) + "g");
    lcd2.print(nonBioFull ? " FULL" : "     ");

    lastDisplayUpdate = now;
  }

  // Debug output
  Serial.print("BioWeight: "); Serial.print(bioWeight, 1);
  Serial.print("g | NonBioWeight: "); Serial.print(nonBioWeight, 1);
  Serial.print("g | BioDist: "); Serial.print(dBioFullAvg);
  Serial.print("cm | NonBioDist: "); Serial.print(dNonBioFullAvg);
  Serial.print("cm | BioLocked: "); Serial.print(bioLocked);
  Serial.print(" | NonBioLocked: "); Serial.print(nonBioLocked);
  Serial.print(" | Lux: "); Serial.println(currentLux);

  delay(100);
}
