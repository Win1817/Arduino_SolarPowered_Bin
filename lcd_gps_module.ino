#include <TinyGPS++.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// --- GPS Configuration ---
static const uint32_t GPSBaud = 9600; 

// Define Software Serial pins: RX Pin 2, TX Pin 3
// GPS TX connects to Arduino RX (Pin 2)
SoftwareSerial ss(2, 3); // RX, TX

TinyGPSPlus gps;

// --- I2C LCD Configuration ---
// IMPORTANT: Check your I2C address (0x27 or 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2); 

unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 500; // Update display every 500ms

void setup()
{
  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("GPS Debug Mode");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  // Start Serial Monitor for debugging GPS output
  Serial.begin(9600); 
  Serial.println("\n--- Starting GPS Debug Monitor ---");
  Serial.println("Waiting for GPS data on Pin 2...");
  
  // Start Software Serial for GPS communication
  ss.begin(GPSBaud); 
  
  delay(1000);
}

void loop()
{
  // Process incoming GPS data from the Software Serial buffer (ss)
  while (ss.available() > 0) {
    char c = ss.read();
    if (gps.encode(c)) {
      // A valid sentence was processed
      Serial.print(".");
    }
  }

  // Update the display every 500ms to reduce flicker
  if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    displayInfo();
    lastDisplayUpdate = millis();
  }

  // Safety check: If we haven't received any data after 10 seconds, something is wrong.
  if (millis() > 10000 && gps.charsProcessed() < 10) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("GPS ERROR!");
    lcd.setCursor(0, 1);
    lcd.print("Check Wires/Baud");
    Serial.println("\n!!! GPS Data stream failed. Check wiring and baud rate. !!!");
    while(true); // Halt the program
  }
}

// Function to display the parsed GPS data on the I2C LCD
void displayInfo()
{
  // --- Line 1: Location (Lat/Lon) ---
  lcd.setCursor(0, 0);
  lcd.print("                "); // Clear the line
  lcd.setCursor(0, 0);
  
  if (gps.location.isValid()) {
    lcd.print(gps.location.lat(), 4);
    lcd.print(" ");
    lcd.print(gps.location.lng(), 3);
    Serial.print("Lat: ");
    Serial.print(gps.location.lat(), 4);
    Serial.print(" Lon: ");
    Serial.println(gps.location.lng(), 4);
  } else {
    lcd.print("LOC: Waiting...");
  }

  // --- Line 2: Satellites and Altitude ---
  lcd.setCursor(0, 1);
  lcd.print("                "); // Clear the line
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
