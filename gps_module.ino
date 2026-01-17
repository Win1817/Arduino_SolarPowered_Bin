// GPS Hardware Serial Test
// This code uses the built-in Serial pins 0 (RX) and 1 (TX)
// This is MORE RELIABLE than SoftwareSerial

unsigned long charCount = 0;
unsigned long lastPrintTime = 0;

void setup()
{
  Serial.begin(9600);
  
  delay(2000);
  
  Serial.println("\n========================================");
  Serial.println("  GPS HARDWARE SERIAL TEST (Pins 0 & 1)");
  Serial.println("========================================");
  Serial.println("Displaying raw sentences from GPS...");
  Serial.println("GPS TX connected to Arduino Pin 0 (RX)");
  Serial.println("GPS RX connected to Arduino Pin 1 (TX)");
  Serial.println("Baud rate: 9600");
  Serial.println("\nIMPORTANT: You must DISCONNECT GPS TX/RX");
  Serial.println("BEFORE uploading this code!");
  Serial.println("After upload completes, RECONNECT the wires.");
  Serial.println("========================================\n");
  Serial.println("Waiting for GPS data...\n");
  
  // Hardware Serial is already initialized at 9600 baud
}

void loop()
{
  // Read and display raw NMEA sentences from Hardware Serial
  while (Serial.available() > 0) {
    char c = Serial.read();
    Serial.write(c);  // Print exactly what GPS sends
    charCount++;
  }
  
  // Every 10 seconds, print a status line
  if (millis() - lastPrintTime >= 10000) {
    Serial.println("\n[STATUS] Total characters received: ");
    Serial.println(charCount);
    Serial.println("[Waiting for NMEA sentences starting with $GPRMC, $GPGGA, etc...]\n");
    lastPrintTime = millis();
  }
}
