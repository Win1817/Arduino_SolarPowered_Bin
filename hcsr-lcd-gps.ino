#include <TinyGPS++.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// --- GPS Configuration ---
static const uint32_t GPSBaud = 9600;
SoftwareSerial ss(2, 3); // RX, TX
TinyGPSPlus gps;

// --- I2C LCDs ---
LiquidCrystal_I2C lcd1(0x27, 16, 2); // Biodegradable
LiquidCrystal_I2C lcd2(0x23, 16, 2); // Non-Biodegradable

// --- Ultrasonic pins ---
// Biodegradable
const int trigBio = 4;
const int echoBio = 5;

// Non-Biodegradable
const int trigNonBio = 6;
const int echoNonBio = 7;

unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 5000; // 5 sec

// For temporary distance display
bool bioTriggered = false;
bool nonBioTriggered = false;
unsigned long bioStart = 0;
unsigned long nonBioStart = 0;
const unsigned long DIST_DISPLAY_TIME = 3000; // 3 sec

void setup() {
  Serial.begin(9600);
  ss.begin(GPSBaud);

  // LCD init
  lcd1.init(); lcd1.backlight();
  lcd2.init(); lcd2.backlight();

  // LCD default labels
  lcd1.setCursor(0, 0); lcd1.print("Biodegradable");
  lcd2.setCursor(0, 0); lcd2.print("Non-Biodegradable");

  // Ultrasonic pins
  pinMode(trigBio, OUTPUT);
  pinMode(echoBio, INPUT);
  pinMode(trigNonBio, OUTPUT);
  pinMode(echoNonBio, INPUT);
}

// Function to read distance in cm, averaging 5 readings
long readDistance(int trigPin, int echoPin) {
  long sum = 0;
  int count = 5;
  for (int i = 0; i < count; i++) {
    digitalWrite(trigPin, LOW); delayMicroseconds(2);
    digitalWrite(trigPin, HIGH); delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH, 50000);
    if (duration == 0) count--; // ignore failed reading
    else sum += duration * 0.034 / 2;
    delay(20);
  }
  if (count == 0) return -1;
  return sum / count;
}

void loop() {
  // --- Read GPS ---
  while (ss.available() > 0) gps.encode(ss.read());

  unsigned long currentMillis = millis();

  // --- Check Ultrasonic sensors ---
  long distBio = readDistance(trigBio, echoBio);
  if (distBio != -1 && distBio < 50) { // trigger if something is within 50cm
    bioTriggered = true;
    bioStart = currentMillis;
    lcd1.clear();
    lcd1.setCursor(0, 0);
    lcd1.print("Distance:");
    lcd1.setCursor(0, 1);
    lcd1.print(distBio);
    lcd1.print(" cm");
  }

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

  // --- Return to Bin+GPS after DIST_DISPLAY_TIME ---
  if (bioTriggered && currentMillis - bioStart > DIST_DISPLAY_TIME) {
    bioTriggered = false;
    displayGPS(lcd1, "Biodegradable");
  }
  if (nonBioTriggered && currentMillis - nonBioStart > DIST_DISPLAY_TIME) {
    nonBioTriggered = false;
    displayGPS(lcd2, "Non-Biodegradable");
  }

  // --- Regular 5 sec Bin/GPS display ---
  if (!bioTriggered && !nonBioTriggered && currentMillis - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    displayGPS(lcd1, "Biodegradable");
    displayGPS(lcd2, "Non-Biodegradable");
    lastDisplayUpdate = currentMillis;
  }
}

// Function to show Bin + GPS info
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
