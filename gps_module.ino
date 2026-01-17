#include <SoftwareSerial.h>

// GPS connected to pins 4 and 5 (as per your current setup)
SoftwareSerial ss(4, 5); // RX, TX

unsigned long charCount = 0;
unsigned long lastPrintTime = 0;

void setup()
{
  Serial.begin(9600);
  
  delay(2000);
  
  Serial.println("\n========================================");
  Serial.println("  GPS RAW NMEA SENTENCE VIEWER");
  Serial.println("========================================");
  Serial.println("Displaying raw sentences from GPS...");
  Serial.println("Connected to SoftwareSerial pins 4 (RX), 5 (TX)");
  Serial.println("Testing baud rate: 9600\n");
  
  ss.begin(9600);
}

void loop()
{
  // Read and display raw NMEA sentences
  while (ss.available() > 0) {
    char c = ss.read();
    Serial.write(c);  // Print exactly what GPS sends
    charCount++;
  }
  
  // Every 10 seconds, print a status line
  if (millis() - lastPrintTime >= 10000) {
    Serial.println("\n[STATUS] Characters received so far: ");
    Serial.println(charCount);
    Serial.println("[Waiting for NMEA sentences starting with $GPRMC, $GPGGA, etc...]\n");
    lastPrintTime = millis();
  }
}
