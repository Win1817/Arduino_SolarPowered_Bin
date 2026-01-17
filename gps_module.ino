#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// --- GPS Module Configuration ---
// Define the pins for Software Serial communication
static const int RXPin = 2; // Arduino Pin 2 receives data from GPS TX
static const int TXPin = 3; // Arduino Pin 3 transmits commands to GPS RX (usually not needed)
static const uint32_t GPSBaud = 9600; // Standard baud rate for most common GPS modules

// Create the SoftwareSerial object for the GPS
SoftwareSerial ss(RXPin, TXPin);

// Create the TinyGPS++ object
TinyGPSPlus gps;

void setup()
{
  // Start the main Serial Monitor connection (for viewing results)
  Serial.begin(115200);
  Serial.println(F("Starting GPS Test (TinyGPS++)"));

  // Start the serial connection to the GPS module
  ss.begin(GPSBaud);
}

void loop()
{
  // Process incoming GPS data character by character
  while (ss.available() > 0) {
    // Feed the character into the TinyGPS++ parser
    if (gps.encode(ss.read())) {
      // If encode returns true, a complete, valid sentence was processed.
      displayInfo();
    }
  }

  // Safety check: If we haven't received any data after 5 seconds, something is wrong.
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("ERROR: No GPS data received! Check wiring and power."));
    while(true); // Halt the program
  }
}

// Function to display the parsed GPS data
void displayInfo()
{
  Serial.println(F("--------------------------------------"));
  
  Serial.print(F("Time: "));
  if (gps.time.isValid()) {
    Serial.print(gps.time.hour()); Serial.print(":");
    Serial.print(gps.time.minute()); Serial.print(":");
    Serial.println(gps.time.second());
  } else {
    Serial.println(F("Invalid"));
  }

  Serial.print(F("Date: "));
  if (gps.date.isValid()) {
    Serial.print(gps.date.day()); Serial.print("/");
    Serial.print(gps.date.month()); Serial.print("/");
    Serial.println(gps.date.year());
  } else {
    Serial.println(F("Invalid"));
  }

  Serial.print(F("Location: "));
  if (gps.location.isValid()) {
    Serial.print(gps.location.lat(), 6); // Print latitude with 6 decimal places
    Serial.print(F(", "));
    Serial.print(gps.location.lng(), 6); // Print longitude with 6 decimal places
    Serial.println();
  } else {
    Serial.println(F("Invalid"));
  }

  Serial.print(F("Satellites in use: "));
  Serial.println(gps.satellites.value());
  
  Serial.print(F("Altitude (meters): "));
  if (gps.altitude.isValid()) {
    Serial.println(gps.altitude.meters());
  } else {
    Serial.println(F("Invalid"));
  }
}
