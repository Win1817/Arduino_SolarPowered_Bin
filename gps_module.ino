#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

// GPS module pins
#define GPS_RX_PIN  4   // Connect to GPS TX
#define GPS_TX_PIN  3   // Connect to GPS RX

// Create SoftwareSerial object
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);  // RX, TX

// Create TinyGPS++ object
TinyGPSPlus gps;

void setup() {
  Serial.begin(9600);         // For Serial Monitor
  gpsSerial.begin(9600);      // NEO-6M default baud rate is 9600

  Serial.println(F("NEO-6M GPS Module - TinyGPS++ Test"));
  Serial.println(F("Waiting for GPS fix... (may take 1-5 minutes outdoors)"));
  Serial.println(F("Make sure antenna has clear sky view!"));
}

void loop() {
  // Feed data from GPS to TinyGPS++
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      // New valid sentence received - display data
      displayGPSInfo();
    }
  }
}

void displayGPSInfo() {
  Serial.println(F("----------------------------------------"));

  if (gps.location.isValid()) {
    Serial.print(F("Location: "));
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(", "));
    Serial.println(gps.location.lng(), 6);

    Serial.print(F("Altitude: "));
    Serial.print(gps.altitude.meters());
    Serial.println(F(" meters"));

    Serial.print(F("Speed: "));
    Serial.print(gps.speed.kmph());
    Serial.println(F(" km/h"));
  } else {
    Serial.println(F("Location: INVALID"));
  }

  if (gps.date.isValid()) {
    Serial.print(F("Date: "));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.println(gps.date.year());
  }

  if (gps.time.isValid()) {
    Serial.print(F("Time (UTC): "));
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.println(gps.time.second());
  }

  Serial.print(F("Satellites: "));
  Serial.println(gps.satellites.value());

  Serial.print(F("HDOP (Accuracy): "));
  Serial.println(gps.hdop.hdop());
  Serial.println();
}