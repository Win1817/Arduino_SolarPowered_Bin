#include <TinyGPS++.h>
#include <LiquidCrystal.h>

// --- GPS Configuration (Using Hardware Serial Pins 0 & 1) ---
// NOTE: When using Hardware Serial (pins 0 & 1), you cannot use the standard Serial Monitor 
// while the GPS is connected, as they share the same physical connection path.
static const uint32_t GPSBaud = 9600; 

// The TinyGPS++ object will use the Serial object (Hardware Serial)
TinyGPSPlus gps;

// --- LCD Configuration (HD44780 in 4-bit Mode) ---
// Initialize the library with the numbers of the interface pins
// (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup()
{
  // Initialize the LCD: 16 columns and 2 rows
  lcd.begin(16, 2);
  lcd.print("GPS Initializing...");

  // Start Hardware Serial for GPS communication
  // Set the baud rate to match your GPS module (usually 9600)
  Serial.begin(GPSBaud); 
}

void loop()
{
  // Process incoming GPS data from the Hardware Serial buffer
  while (Serial.available() > 0) {
    if (gps.encode(Serial.read())) {
      // A valid sentence was processed, update the display
      displayInfo();
    }
  }

  // Safety check: If we haven't received any data after 10 seconds, something is wrong.
  if (millis() > 10000 && gps.charsProcessed() < 10) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("GPS ERROR!");
    lcd.setCursor(0, 1);
    lcd.print("Check Wiring");
    while(true); // Halt the program
  }
}

// Function to display the parsed GPS data on the LCD
void displayInfo()
{
  // --- Line 1: Location (Lat/Lon) ---
  lcd.setCursor(0, 0);
  
  if (gps.location.isValid()) {
    // Display Latitude (4 decimal places)
    lcd.print(gps.location.lat(), 4);
    lcd.print(" ");
    // Display Longitude (3 decimal places)
    lcd.print(gps.location.lng(), 3);
  } else {
    lcd.print("LOC: Waiting...");
  }

  // --- Line 2: Satellites and Altitude ---
  lcd.setCursor(0, 1);
  
  // Satellites
  lcd.print("S:");
  lcd.print(gps.satellites.value());
  lcd.print(" ");

  // Altitude (in meters, integer value)
  lcd.print("A:");
  if (gps.altitude.isValid()) {
    lcd.print(gps.altitude.meters(), 0); 
  } else {
    lcd.print("--");
  }
}
