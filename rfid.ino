#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  10    // SDA / SS pin
#define RST_PIN 9     // Reset pin

MFRC522 rfid(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup() {
  Serial.begin(9600);           // Initialize serial communications
  while (!Serial);              // Wait for serial (good for some boards)
  
  SPI.begin();                  // Init SPI bus
  rfid.PCD_Init();              // Init MFRC522
  
  Serial.println("Tap an RFID card/tag to read its UID...");
  Serial.println("Approach with card →");
}

void loop() {
  // Look for new cards
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Show UID on serial monitor
  Serial.print("Card UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Halt PICC (stop reading)
  rfid.PICC_HaltA();
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();

  delay(1000);  // Small delay to avoid multiple reads
}