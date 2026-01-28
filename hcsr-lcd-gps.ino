#include <TinyGPS++.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <MFRC522.h>

// --- GPS Configuration ---
static const uint32_t GPSBaud = 9600;
SoftwareSerial ss(2, 3);
TinyGPSPlus gps;

// --- I2C LCDs ---
LiquidCrystal_I2C lcd1(0x27, 16, 2);
LiquidCrystal_I2C lcd2(0x23, 16, 2);

// --- Ultrasonic pins ---
const int trigBio = 4;
const int echoBio = 5;
const int trigNonBio = 6;
const int echoNonBio = 7;

// --- LED Strip Relay Control ---
const int relayLED = 8;

// --- RFID RC522 pins ---
#define SS_PIN_BIO 10
#define RST_PIN_BIO 9
#define SS_PIN_NONBIO A0
#define RST_PIN_NONBIO A1

MFRC522 rfidBio(SS_PIN_BIO, RST_PIN_BIO);
MFRC522 rfidNonBio(SS_PIN_NONBIO, RST_PIN_NONBIO);

// --- Authorized RFID UIDs ---
byte authorizedUID_Bio1[4] = {0xF3, 0x37, 0xB3, 0x39};
byte authorizedUID_Bio2[4] = {0x00, 0x00, 0x00, 0x00};
byte authorizedUID_NonBio1[4] = {0x43, 0xFE, 0xB5, 0x38};
byte authorizedUID_NonBio2[4] = {0x00, 0x00, 0x00, 0x00};

unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 5000;

bool bioTriggered = false;
bool nonBioTriggered = false;
unsigned long bioStart = 0;
unsigned long nonBioStart = 0;
const unsigned long DIST_DISPLAY_TIME = 3000;

unsigned long lastLEDBlink = 0;
unsigned long LED_BLINK_INTERVAL = 3000;
bool ledState = false;

bool bioAccessGranted = false;
bool nonBioAccessGranted = false;
unsigned long bioAccessTime = 0;
unsigned long nonBioAccessTime = 0;
const unsigned long ACCESS_DURATION = 10000;

bool fastBlinkMode = false;
const unsigned long FAST_BLINK_INTERVAL = 200;
const unsigned long SLOW_BLINK_INTERVAL = 3000;

void setup() {
  Serial.begin(9600);
  ss.begin(GPSBaud);
  SPI.begin();
  
  rfidBio.PCD_Init();
  rfidNonBio.PCD_Init();
  
  Serial.println("System Ready");
  
  lcd1.init();
  lcd1.backlight();
  lcd2.init();
  lcd2.backlight();
  
  lcd1.setCursor(0, 0);
  lcd1.print("Biodegradable");
  lcd1.setCursor(0, 1);
  lcd1.print("Scan RFID...");
  
  lcd2.setCursor(0, 0);
  lcd2.print("Non-Biodegradable");
  lcd2.setCursor(0, 1);
  lcd2.print("Scan RFID...");
  
  pinMode(trigBio, OUTPUT);
  pinMode(echoBio, INPUT);
  pinMode(trigNonBio, OUTPUT);
  pinMode(echoNonBio, INPUT);
  
  pinMode(relayLED, OUTPUT);
  digitalWrite(relayLED, HIGH);
}

long readDistance(int trigPin, int echoPin) {
  long sum = 0;
  int count = 5;
  for (int i = 0; i < count; i++) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH, 50000);
    if (duration == 0) count--;
    else sum += duration * 0.034 / 2;
    delay(20);
  }
  if (count == 0) return -1;
  return sum / count;
}

bool checkUID(byte *cardUID, byte *authorizedUID) {
  for (byte i = 0; i < 4; i++) {
    if (cardUID[i] != authorizedUID[i]) return false;
  }
  return true;
}

void handleRFID_Bio() {
  if (!rfidBio.PICC_IsNewCardPresent() || !rfidBio.PICC_ReadCardSerial()) {
    return;
  }
  
  Serial.print("Bio UID: ");
  for (byte i = 0; i < rfidBio.uid.size; i++) {
    Serial.print(rfidBio.uid.uidByte[i] < 0x10 ? " 0x0" : " 0x");
    Serial.print(rfidBio.uid.uidByte[i], HEX);
  }
  Serial.println();
  
  if (checkUID(rfidBio.uid.uidByte, authorizedUID_Bio1) || 
      checkUID(rfidBio.uid.uidByte, authorizedUID_Bio2)) {
    
    bioAccessGranted = true;
    bioAccessTime = millis();
    
    lcd1.clear();
    lcd1.setCursor(0, 0);
    lcd1.print("Access Granted!");
    lcd1.setCursor(0, 1);
    lcd1.print("Welcome :)");
    
    Serial.println("Bio: Granted");
    
    for (int i = 0; i < 5; i++) {
      digitalWrite(relayLED, LOW);
      delay(150);
      digitalWrite(relayLED, HIGH);
      delay(150);
    }
    
    delay(1500);
    
  } else {
    
    lcd1.clear();
    lcd1.setCursor(0, 0);
    lcd1.print("Access Denied!");
    lcd1.setCursor(0, 1);
    lcd1.print("Invalid Card");
    
    Serial.println("Bio: Denied");
    
    digitalWrite(relayLED, LOW);
    delay(2000);
    digitalWrite(relayLED, HIGH);
    
    lcd1.clear();
    lcd1.setCursor(0, 0);
    lcd1.print("Biodegradable");
    lcd1.setCursor(0, 1);
    lcd1.print("Scan RFID...");
  }
  
  rfidBio.PICC_HaltA();
  rfidBio.PCD_StopCrypto1();
}

void handleRFID_NonBio() {
  if (!rfidNonBio.PICC_IsNewCardPresent() || !rfidNonBio.PICC_ReadCardSerial()) {
    return;
  }
  
  Serial.print("NonBio UID: ");
  for (byte i = 0; i < rfidNonBio.uid.size; i++) {
    Serial.print(rfidNonBio.uid.uidByte[i] < 0x10 ? " 0x0" : " 0x");
    Serial.print(rfidNonBio.uid.uidByte[i], HEX);
  }
  Serial.println();
  
  if (checkUID(rfidNonBio.uid.uidByte, authorizedUID_NonBio1) || 
      checkUID(rfidNonBio.uid.uidByte, authorizedUID_NonBio2)) {
    
    nonBioAccessGranted = true;
    nonBioAccessTime = millis();
    
    lcd2.clear();
    lcd2.setCursor(0, 0);
    lcd2.print("Access Granted!");
    lcd2.setCursor(0, 1);
    lcd2.print("Welcome :)");
    
    Serial.println("NonBio: Granted");
    
    for (int i = 0; i < 5; i++) {
      digitalWrite(relayLED, LOW);
      delay(150);
      digitalWrite(relayLED, HIGH);
      delay(150);
    }
    
    delay(1500);
    
  } else {
    
    lcd2.clear();
    lcd2.setCursor(0, 0);
    lcd2.print("Access Denied!");
    lcd2.setCursor(0, 1);
    lcd2.print("Invalid Card");
    
    Serial.println("NonBio: Denied");
    
    digitalWrite(relayLED, LOW);
    delay(2000);
    digitalWrite(relayLED, HIGH);
    
    lcd2.clear();
    lcd2.setCursor(0, 0);
    lcd2.print("Non-Biodegradable");
    lcd2.setCursor(0, 1);
    lcd2.print("Scan RFID...");
  }
  
  rfidNonBio.PICC_HaltA();
  rfidNonBio.PCD_StopCrypto1();
}

void displayGPS(LiquidCrystal_I2C &lcd, String binLabel) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(binLabel);
  lcd.setCursor(0, 1);
  if (gps.location.isValid()) {
    lcd.print("Lat:");
    lcd.print(gps.location.lat(), 4);
    lcd.print(" ");
    lcd.print(gps.location.lng(), 4);
  } else {
    lcd.print("Loc: N/A");
  }
}

void loop() {
  while (ss.available() > 0) gps.encode(ss.read());
  
  unsigned long currentMillis = millis();
  
  if (!bioAccessGranted) {
    handleRFID_Bio();
  }
  if (!nonBioAccessGranted) {
    handleRFID_NonBio();
  }
  
  if (bioAccessGranted && currentMillis - bioAccessTime > ACCESS_DURATION) {
    bioAccessGranted = false;
    lcd1.clear();
    lcd1.setCursor(0, 0);
    lcd1.print("Biodegradable");
    lcd1.setCursor(0, 1);
    lcd1.print("Scan RFID...");
  }
  
  if (nonBioAccessGranted && currentMillis - nonBioAccessTime > ACCESS_DURATION) {
    nonBioAccessGranted = false;
    lcd2.clear();
    lcd2.setCursor(0, 0);
    lcd2.print("Non-Biodegradable");
    lcd2.setCursor(0, 1);
    lcd2.print("Scan RFID...");
  }
  
  if (bioAccessGranted) {
    long distBio = readDistance(trigBio, echoBio);
    if (distBio != -1 && distBio < 50) {
      bioTriggered = true;
      bioStart = currentMillis;
      
      lcd1.clear();
      lcd1.setCursor(0, 0);
      lcd1.print("Distance:");
      lcd1.setCursor(0, 1);
      lcd1.print(distBio);
      lcd1.print(" cm");
    }
  }
  
  if (nonBioAccessGranted) {
    long distNonBio = readDistance(trigNonBio, echoNonBio);
    if (distNonBio != -1 && distNonBio < 50) {
      nonBioTriggered = true;
      nonBioStart = currentMillis;
      
      lcd2.clear();
      lcd2.setCursor(0, 0);
      lcd2.print("Distance:");
      lcd2.setCursor(0, 1);
      lcd2.print(distNonBio);
      lcd2.print(" cm");
    }
  }
  
  if (bioTriggered && currentMillis - bioStart > DIST_DISPLAY_TIME) {
    bioTriggered = false;
    displayGPS(lcd1, "Biodegradable");
  }
  
  if (nonBioTriggered && currentMillis - nonBioStart > DIST_DISPLAY_TIME) {
    nonBioTriggered = false;
    displayGPS(lcd2, "Non-Biodegradable");
  }
  
  if (bioTriggered || nonBioTriggered) {
    if (currentMillis - lastLEDBlink >= SLOW_BLINK_INTERVAL) {
      ledState = !ledState;
      digitalWrite(relayLED, ledState ? LOW : HIGH);
      lastLEDBlink = currentMillis;
    }
  } else {
    digitalWrite(relayLED, HIGH);
    ledState = false;
    lastLEDBlink = currentMillis;
  }
  
  if (bioAccessGranted && !bioTriggered && currentMillis - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    displayGPS(lcd1, "Biodegradable");
  }
  
  if (nonBioAccessGranted && !nonBioTriggered && currentMillis - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    displayGPS(lcd2, "Non-Biodegradable");
  }
  
  if (currentMillis - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = currentMillis;
  }
}
