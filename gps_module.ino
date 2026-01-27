#include <TinyGPS++.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// --- GPS Configuration ---
static const uint32_t GPSBaud = 9600; 

// GPS connected to SoftwareSerial pins: RX=2, TX=3
SoftwareSerial ss(2, 3); // RX, TX
TinyGPSPlus gps;

// --- I2C LCDs Configuration ---
LiquidCrystal_I2C lcd1(0x27, 16, 2); // Biodegradable
LiquidCrystal_I2C lcd2(0x23, 16, 2); // Non-Biodegradable

unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 5000; // 5 seconds

bool showGPS = false; // flag to toggle between bin label and GPS

void setup() {
  Serial.begin(9600);
  Serial.println("\n--- Starting GPS Monitor ---");
  Serial.println("Waiting for GPS data on Pin 2...");
  ss.begin(GPSBaud);

  // Initialize LCD1
  lcd1.init();
  lcd1.backlight();

  // Initialize LCD2
  lcd2.init();
  lcd2.backlight();

  // Show initial bin labels
  lcd1.setCursor(0, 0);
  lcd1.print("Biodegradable");
  lcd2.setCursor(0, 0);
  lcd2.print("Non-Biodegradable");
}

void loop() {
  // Read GPS data
  while (ss.available() > 0) {
    char c = ss.read();
    gps.encode(c);
  }

  // Switch display every 5 seconds
  if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    if (showGPS) {
      // Show GPS stats
      displayGPS(lcd1);
      displayGPS(lcd2);
    } else {
      // Show bin labels
      lcd1.clear();
      lcd1.setCursor(0, 0);
      lcd1.print("Biodegradable");

      lcd2.clear();
      lcd2.setCursor(0, 0);
      lcd2.print("Non-Biodegradable");
    }

    showGPS = !showGPS; // toggle flag
    lastDisplayUpdate = millis();
  }

  // Safety check: no GPS data
  if (millis() > 10000 && gps.charsProcessed() < 10) {
    lcd1.clear();
    lcd1.setCursor(0, 0);
    lcd1.print("GPS ERROR!");
    lcd2.clear();
    lcd2.setCursor(0, 0);
    lcd2.print("Check Wires/Baud");
    Serial.println("\n!!! GPS Data stream failed. Check wiring and baud rate. !!!");
    while(true); // halt
  }
}

void displayGPS(LiquidCrystal_I2C &lcd) {
  lcd.clear();

  // Line 1: Latitude and Longitude
  lcd.setCursor(0, 0);
  if (gps.location.isValid()) {
    lcd.print("Lat:");
    lcd.print(gps.location.lat(), 4);
    lcd.setCursor(0, 1);
    lcd.print("Lon:");
    lcd.print(gps.location.lng(), 4);
  } else {
    lcd.print("Loc: Waiting...");
  }

  // Line 2: Satellites and Altitude
  lcd.setCursor(0, 1);
  lcd.print("S:");
  lcd.print(gps.satellites.value());
  lcd.print(" A:");
  if (gps.altitude.isValid()) {
    lcd.print(gps.altitude.meters(), 0);
  } else {
    lcd.print("--");
  }
}
