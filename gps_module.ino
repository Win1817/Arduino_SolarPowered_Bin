#include <SoftwareSerial.h>

// Define Software Serial pins: RX Pin 2, TX Pin 3
SoftwareSerial ss(2, 3); // RX, TX

void setup()
{
  Serial.begin(9600);
  
  delay(2000);
  
  Serial.println("\n========================================");
  Serial.println("  GPS RAW DATA DIAGNOSTIC TEST");
  Serial.println("========================================");
  Serial.println("Listening on Pin 2 for GPS data...");
  Serial.println("Testing multiple baud rates...\n");
  
  // Test at 9600 baud first
  Serial.println("Starting at 9600 baud...");
  ss.begin(9600);
}

void loop()
{
  static unsigned long startTime = millis();
  static int testPhase = 0;
  static bool phase1Done = false;
  
  // Phase 1: Test 9600 baud for 10 seconds
  if (!phase1Done && millis() - startTime < 10000) {
    if (ss.available() > 0) {
      Serial.print("Data at 9600 baud: ");
      while (ss.available() > 0) {
        char c = ss.read();
        Serial.write(c);
      }
      Serial.println();
    }
  }
  
  // After 10 seconds, switch to 4800 baud
  if (!phase1Done && millis() - startTime >= 10000) {
    phase1Done = true;
    Serial.println("\n--- Switching to 4800 baud ---\n");
    ss.end();
    ss.begin(4800);
    startTime = millis();
  }
  
  // Phase 2: Test 4800 baud for 10 seconds
  if (phase1Done && millis() - startTime < 10000) {
    if (ss.available() > 0) {
      Serial.print("Data at 4800 baud: ");
      while (ss.available() > 0) {
        char c = ss.read();
        Serial.write(c);
      }
      Serial.println();
    }
  }
  
  // After another 10 seconds, switch to 115200 baud
  if (phase1Done && millis() - startTime >= 10000) {
    Serial.println("\n--- Switching to 115200 baud ---\n");
    ss.end();
    ss.begin(115200);
    phase1Done = false;
    startTime = millis();
  }
}
