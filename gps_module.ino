#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

// SoftwareSerial: RX pin (connect to GPS TX), TX pin (connect to GPS RX)
SoftwareSerial gpsSerial(4, 3);  // RX=4, TX=3

TinyGPSPlus gps;  // The parsing object

void setup() {
  Serial.begin(9600);       // Serial Monitor output
  gpsSerial.begin(9600);    // NEO-6M default baud rate

  Serial.println(F("GY-GPS6MV2 / NEO-6M Test"));
  Serial.println(F("Waiting for satellite fix..."));
  Serial.println(F("Take module outside with clear sky view."));
  Serial.println(F("LED on module blinks slowly → searching; faster → fixed."));
  Serial.println("--------------------------------------------");
}

void loop() {
  // Feed GPS data to the parser
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      displayGPSInfo();  // New valid sentence parsed → show data
    }
  }

  // Safety check: no characters received for 5 seconds?
  static unsigned long last = millis();
  if (millis() - last > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS data received yet – check wiring, power, or if outdoors"));
    last = millis();
  }
}

void displayGPSInfo() {
  Serial.print(F("Location: "));
  if (gps.location.isValid()) {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(", "));
    Serial.print(gps.location.lng(), 6);
  } else {
    Serial.print(F("INVALID / NO FIX"));
  }

  Serial.print(F("  |  Date: "));
  if (gps.date.isValid()) {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  } else {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  |  Time (UTC): "));
  if (gps.time.isValid()) {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
  } else {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  |  Sat: "));
  Serial.print(gps.satellites.value());

  Serial.print(F("  |  Alt (m): "));
  if (gps.altitude.isValid()) {
    Serial.print(gps.altitude.meters(), 1);
  } else {
    Serial.print(F("N/A"));
  }

  Serial.print(F("  |  Speed (km/h): "));
  if (gps.speed.isValid()) {
    Serial.print(gps.speed.kmph(), 1);
  } else {
    Serial.print(F("N/A"));
  }

  Serial.println();
}