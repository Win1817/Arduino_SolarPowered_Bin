/*
 * Solar-Powered Smart Waste Bin System
 * Two Independent Bins: Biodegradable & Non-Biodegradable
 * Features: Ultrasonic detection, Load cell monitoring, RFID authentication,
 *           GSM/GPS notification, Servo control, LCD display
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <HX711.h>
#include <SoftwareSerial.h>
#include <MFRC522.h>
#include <SPI.h>
#include <TinyGPS++.h>

// ========== PIN DEFINITIONS ==========

// Biodegradable Bin
#define BIO_ULTRASONIC_TRIG 2
#define BIO_ULTRASONIC_ECHO 3
#define BIO_SERVO_PIN 4
#define BIO_LOADCELL_DOUT A0
#define BIO_LOADCELL_SCK A1
#define BIO_LED_GREEN 5
#define BIO_LED_RED 6

// Non-Biodegradable Bin
#define NONBIO_ULTRASONIC_TRIG 7
#define NONBIO_ULTRASONIC_ECHO 8
#define NONBIO_SERVO_PIN 9
#define NONBIO_LOADCELL_DOUT A2
#define NONBIO_LOADCELL_SCK A3
#define NONBIO_LED_GREEN 10
#define NONBIO_LED_RED 11

// RFID (Shared for both bins)
#define RFID_SS_PIN 53
#define RFID_RST_PIN 49

// GSM Module (SIM800L)
#define GSM_RX 14  // Connect to TX of SIM800L
#define GSM_TX 15  // Connect to RX of SIM800L

// GPS Module
#define GPS_RX 16  // Connect to TX of GPS
#define GPS_TX 17  // Connect to RX of GPS

// ========== CONFIGURATION ==========

// Distance thresholds (cm)
#define DETECTION_DISTANCE 30  // Distance to detect person
#define BIN_HEIGHT 40          // Height of bin in cm

// Weight thresholds (grams)
#define MAX_WEIGHT_BIO 5000     // 5kg max for biodegradable
#define MAX_WEIGHT_NONBIO 5000  // 5kg max for non-biodegradable

// Timing
#define LID_OPEN_TIME 5000      // Keep lid open for 5 seconds
#define SENSOR_READ_INTERVAL 500 // Read sensors every 500ms

// Phone number for SMS notifications
#define PHONE_NUMBER "+639123456789"  // Replace with actual number

// Authorized RFID UIDs (Add your RFID card UIDs here)
byte authorizedUIDs[][4] = {
  {0xDE, 0xAD, 0xBE, 0xEF},  // Example UID 1
  {0x12, 0x34, 0x56, 0x78}   // Example UID 2
};
#define NUM_AUTHORIZED_CARDS 2

// ========== OBJECT INITIALIZATION ==========

// LCD Display (I2C address 0x27, 16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servos
Servo bioServo;
Servo nonBioServo;

// Load Cells
HX711 bioLoadCell;
HX711 nonBioLoadCell;

// RFID
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

// GSM & GPS
SoftwareSerial gsmSerial(GSM_RX, GSM_TX);
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;

// ========== GLOBAL VARIABLES ==========

// Bin states
enum BinState { AVAILABLE, FULL, LOCKED };
BinState bioState = AVAILABLE;
BinState nonBioState = AVAILABLE;

// Current weights
float bioWeight = 0;
float nonBioWeight = 0;

// Lid states
bool bioLidOpen = false;
bool nonBioLidOpen = false;
unsigned long bioLidOpenTime = 0;
unsigned long nonBioLidOpenTime = 0;

// Notification flags
bool bioNotificationSent = false;
bool nonBioNotificationSent = false;

// GPS coordinates
double latitude = 0.0;
double longitude = 0.0;

// Timing
unsigned long lastSensorRead = 0;

// ========== SETUP ==========

void setup() {
  Serial.begin(9600);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Waste Bin");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  // Initialize pins
  pinMode(BIO_ULTRASONIC_TRIG, OUTPUT);
  pinMode(BIO_ULTRASONIC_ECHO, INPUT);
  pinMode(NONBIO_ULTRASONIC_TRIG, OUTPUT);
  pinMode(NONBIO_ULTRASONIC_ECHO, INPUT);
  
  pinMode(BIO_LED_GREEN, OUTPUT);
  pinMode(BIO_LED_RED, OUTPUT);
  pinMode(NONBIO_LED_GREEN, OUTPUT);
  pinMode(NONBIO_LED_RED, OUTPUT);
  
  // Initialize servos
  bioServo.attach(BIO_SERVO_PIN);
  nonBioServo.attach(NONBIO_SERVO_PIN);
  bioServo.write(0);  // Closed position
  nonBioServo.write(0);
  
  // Initialize load cells
  bioLoadCell.begin(BIO_LOADCELL_DOUT, BIO_LOADCELL_SCK);
  nonBioLoadCell.begin(NONBIO_LOADCELL_DOUT, NONBIO_LOADCELL_SCK);
  
  // Calibrate load cells (you need to calibrate with known weights)
  bioLoadCell.set_scale(2280.f);  // Calibration factor - adjust this
  bioLoadCell.tare();
  nonBioLoadCell.set_scale(2280.f);
  nonBioLoadCell.tare();
  
  // Initialize RFID
  SPI.begin();
  rfid.PCD_Init();
  
  // Initialize GSM
  gsmSerial.begin(9600);
  delay(1000);
  initGSM();
  
  // Initialize GPS
  gpsSerial.begin(9600);
  
  // Update LEDs
  digitalWrite(BIO_LED_GREEN, HIGH);
  digitalWrite(NONBIO_LED_GREEN, HIGH);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  delay(2000);
  
  updateDisplay();
}

// ========== MAIN LOOP ==========

void loop() {
  unsigned long currentMillis = millis();
  
  // Read sensors periodically
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = currentMillis;
    
    // Read weights
    bioWeight = bioLoadCell.get_units(5);
    nonBioWeight = nonBioLoadCell.get_units(5);
    
    // Check if bins are full
    checkBinStatus();
    
    // Update display
    updateDisplay();
  }
  
  // Update GPS data
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      if (gps.location.isValid()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
      }
    }
  }
  
  // Check RFID for authentication
  checkRFID();
  
  // Handle biodegradable bin
  handleBin(
    BIO_ULTRASONIC_TRIG, BIO_ULTRASONIC_ECHO,
    bioServo, bioState, bioLidOpen, bioLidOpenTime,
    bioWeight, MAX_WEIGHT_BIO, "Bio"
  );
  
  // Handle non-biodegradable bin
  handleBin(
    NONBIO_ULTRASONIC_TRIG, NONBIO_ULTRASONIC_ECHO,
    nonBioServo, nonBioState, nonBioLidOpen, nonBioLidOpenTime,
    nonBioWeight, MAX_WEIGHT_NONBIO, "NonBio"
  );
  
  // Close lids if time expired
  checkLidTimers();
}

// ========== BIN HANDLING ==========

void handleBin(int trigPin, int echoPin, Servo &servo, 
               BinState &state, bool &lidOpen, unsigned long &lidOpenTime,
               float weight, float maxWeight, String binType) {
  
  if (state == LOCKED) {
    return;  // Bin is locked, waiting for RFID
  }
  
  if (state == FULL) {
    return;  // Bin is full, automatic opening disabled
  }
  
  // Read ultrasonic sensor
  long distance = readUltrasonic(trigPin, echoPin);
  
  // Person detected and bin not full
  if (distance > 0 && distance < DETECTION_DISTANCE && !lidOpen) {
    openLid(servo, lidOpen, lidOpenTime, binType);
  }
}

void checkLidTimers() {
  unsigned long currentMillis = millis();
  
  // Check biodegradable lid
  if (bioLidOpen && (currentMillis - bioLidOpenTime >= LID_OPEN_TIME)) {
    closeLid(bioServo, bioLidOpen, "Bio");
  }
  
  // Check non-biodegradable lid
  if (nonBioLidOpen && (currentMillis - nonBioLidOpenTime >= LID_OPEN_TIME)) {
    closeLid(nonBioServo, nonBioLidOpen, "NonBio");
  }
}

void openLid(Servo &servo, bool &lidOpen, unsigned long &lidOpenTime, String binType) {
  servo.write(90);  // Open position (adjust angle as needed)
  lidOpen = true;
  lidOpenTime = millis();
  
  Serial.println(binType + " lid opened");
}

void closeLid(Servo &servo, bool &lidOpen, String binType) {
  servo.write(0);  // Closed position
  lidOpen = false;
  
  Serial.println(binType + " lid closed");
}

// ========== SENSOR READING ==========

long readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);  // Timeout after 30ms
  if (duration == 0) return -1;  // No echo received
  
  long distance = duration * 0.034 / 2;  // Convert to cm
  return distance;
}

// ========== BIN STATUS CHECKING ==========

void checkBinStatus() {
  // Check biodegradable bin
  if (bioWeight >= MAX_WEIGHT_BIO && bioState == AVAILABLE) {
    bioState = FULL;
    digitalWrite(BIO_LED_GREEN, LOW);
    digitalWrite(BIO_LED_RED, HIGH);
    
    if (!bioNotificationSent) {
      sendSMSNotification("Biodegradable");
      bioNotificationSent = true;
    }
    
    Serial.println("Biodegradable bin is FULL!");
  }
  
  // Check non-biodegradable bin
  if (nonBioWeight >= MAX_WEIGHT_NONBIO && nonBioState == AVAILABLE) {
    nonBioState = FULL;
    digitalWrite(NONBIO_LED_GREEN, LOW);
    digitalWrite(NONBIO_LED_RED, HIGH);
    
    if (!nonBioNotificationSent) {
      sendSMSNotification("Non-Biodegradable");
      nonBioNotificationSent = true;
    }
    
    Serial.println("Non-Biodegradable bin is FULL!");
  }
}

// ========== RFID AUTHENTICATION ==========

void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }
  
  // Check if UID matches authorized cards
  bool authorized = false;
  for (int i = 0; i < NUM_AUTHORIZED_CARDS; i++) {
    if (checkUID(rfid.uid.uidByte, authorizedUIDs[i], rfid.uid.size)) {
      authorized = true;
      break;
    }
  }
  
  if (authorized) {
    Serial.println("Authorized RFID detected!");
    
    // Unlock bins if they are full
    if (bioState == FULL) {
      bioState = LOCKED;
      digitalWrite(BIO_LED_RED, LOW);
      digitalWrite(BIO_LED_GREEN, HIGH);
      openLid(bioServo, bioLidOpen, bioLidOpenTime, "Bio");
    }
    
    if (nonBioState == FULL) {
      nonBioState = LOCKED;
      digitalWrite(NONBIO_LED_RED, LOW);
      digitalWrite(NONBIO_LED_GREEN, HIGH);
      openLid(nonBioServo, nonBioLidOpen, nonBioLidOpenTime, "NonBio");
    }
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Granted");
    delay(2000);
  } else {
    Serial.println("Unauthorized RFID!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied");
    delay(2000);
  }
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

bool checkUID(byte *uid1, byte *uid2, byte size) {
  for (byte i = 0; i < size; i++) {
    if (uid1[i] != uid2[i]) return false;
  }
  return true;
}

// ========== GSM FUNCTIONS ==========

void initGSM() {
  Serial.println("Initializing GSM...");
  
  gsmSerial.println("AT");
  delay(1000);
  
  gsmSerial.println("AT+CMGF=1");  // Set SMS to text mode
  delay(1000);
  
  Serial.println("GSM initialized");
}

void sendSMSNotification(String binType) {
  Serial.println("Sending SMS notification...");
  
  String message = binType + " bin is FULL! ";
  message += "Location: ";
  
  if (latitude != 0.0 && longitude != 0.0) {
    message += "Lat: " + String(latitude, 6);
    message += ", Lon: " + String(longitude, 6);
    message += " https://maps.google.com/?q=" + String(latitude, 6) + "," + String(longitude, 6);
  } else {
    message += "GPS unavailable";
  }
  
  gsmSerial.println("AT+CMGS=\"" + String(PHONE_NUMBER) + "\"");
  delay(1000);
  gsmSerial.print(message);
  delay(1000);
  gsmSerial.write(26);  // Ctrl+Z to send
  delay(5000);
  
  Serial.println("SMS sent: " + message);
}

// ========== DISPLAY FUNCTIONS ==========

void updateDisplay() {
  lcd.clear();
  
  // Line 1: Biodegradable status
  lcd.setCursor(0, 0);
  lcd.print("B:");
  lcd.print((int)bioWeight);
  lcd.print("g ");
  
  if (bioState == AVAILABLE) {
    lcd.print("OK");
  } else if (bioState == FULL) {
    lcd.print("FULL");
  } else {
    lcd.print("OPEN");
  }
  
  // Line 2: Non-biodegradable status
  lcd.setCursor(0, 1);
  lcd.print("N:");
  lcd.print((int)nonBioWeight);
  lcd.print("g ");
  
  if (nonBioState == AVAILABLE) {
    lcd.print("OK");
  } else if (nonBioState == FULL) {
    lcd.print("FULL");
  } else {
    lcd.print("OPEN");
  }
}
