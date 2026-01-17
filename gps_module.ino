#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// --- GPS Configuration ---
static const uint32_t GPSBaud = 9600; 

// Define Software Serial pins: RX Pin 2, TX Pin 3
// GPS TX connects to Arduino RX (Pin 2)
SoftwareSerial ss(2, 3); // RX, TX

TinyGPSPlus gps;

unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 1000; // Print data every 1 second

void setup()
{
  // Start Serial Monitor for GPS output
  Serial.begin(9600); 
  
  // Wait for Serial Monitor to open
  delay(2000);
  
  Serial.println("\n========================================");
  Serial.println("  GPS Serial Monitor Test");
  Serial.println("========================================");
  Serial.println("Waiting for GPS data on Pin 2...");
  Serial.println("Make sure GPS TX is connected to Pin 2");
  Serial.println("GPS module needs clear sky view for lock");
  Serial.println("========================================\n");
  
  // Start Software Serial for GPS communication
  ss.begin(GPSBaud); 
}

void loop()
{
  // Process incoming GPS data from the Software Serial buffer (ss)
  while (ss.available() > 0) {
    char c = ss.read();
    
    // Print raw NMEA sentences as they arrive
    Serial.write(c);
    
    // Encode the character for GPS parsing
    if (gps.encode(c)) {
      // A complete sentence was parsed (happens silently)
    }
  }

  // Print parsed GPS data every 1 second
  if (millis() - lastPrintTime >= PRINT_INTERVAL) {
    printGPSData();
    lastPrintTime = millis();
  }

  // Safety check: If no data received after 15 seconds, alert user
  if (millis() > 15000 && gps.charsProcessed() < 10) {
    Serial.println("\n\n!!! ERROR: No GPS data received !!!");
    Serial.println("Troubleshooting:");
    Serial.println("1. Check GPS TX wire connected to Arduino Pin 2");
    Serial.println("2. Check GPS power (VCC and GND)");
    Serial.println("3. Verify GPS baud rate (try 4800 if 9600 fails)");
    Serial.println("4. Ensure GPS has clear sky view");
    Serial.println("\nHalting program...\n");
    while(true);
  }
}

void printGPSData()
{
  Serial.println("\n--- GPS Data ---");
  
  // Location
  Serial.print("Location: ");
  if (gps.location.isValid()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("          Longitude: ");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println("INVALID");
  }

  // Date
  Serial.print("Date: ");
  if (gps.date.isValid()) {
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.print(gps.date.day());
    Serial.print("/");
    Serial.println(gps.date.year());
  } else {
    Serial.println("INVALID");
  }

  // Time
  Serial.print("Time: ");
  if (gps.time.isValid()) {
    if (gps.time.hour() < 10) Serial.print("0");
    Serial.print(gps.time.hour());
    Serial.print(":");
    if (gps.time.minute() < 10) Serial.print("0");
    Serial.print(gps.time.minute());
    Serial.print(":");
    if (gps.time.second() < 10) Serial.print("0");
    Serial.println(gps.time.second());
  } else {
    Serial.println("INVALID");
  }

  // Altitude
  Serial.print("Altitude: ");
  if (gps.altitude.isValid()) {
    Serial.print(gps.altitude.meters(), 2);
    Serial.println(" meters");
  } else {
    Serial.println("INVALID");
  }

  // Speed
  Serial.print("Speed: ");
  if (gps.speed.isValid()) {
    Serial.print(gps.speed.kmph(), 2);
    Serial.println(" km/h");
  } else {
    Serial.println("INVALID");
  }

  // Course (Direction)
  Serial.print("Course: ");
  if (gps.course.isValid()) {
    Serial.print(gps.course.deg(), 2);
    Serial.println(" degrees");
  } else {
    Serial.println("INVALID");
  }

  // Satellite Count
  Serial.print("Satellites: ");
  Serial.println(gps.satellites.value());

  // HDOP (Horizontal Dilution of Precision)
  Serial.print("HDOP: ");
  if (gps.hdop.isValid()) {
    Serial.println(gps.hdop.hdop(), 2);
  } else {
    Serial.println("INVALID");
  }

  // Characters Processed
  Serial.print("Total characters processed: ");
  Serial.println(gps.charsProcessed());

  Serial.println("---\n");
}
