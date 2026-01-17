/*
 * SIM800A SMS Sender
 * Hardware Connection:
 * SIM800A TX -> Arduino RX (Pin 10)
 * SIM800A RX -> Arduino TX (Pin 11)
 * SIM800A GND -> Arduino GND
 * SIM800A VCC -> External 5V Power Supply (2A recommended)
 */

#include <SoftwareSerial.h>

// Create software serial for SIM800A
SoftwareSerial sim800(10, 11); // RX, TX

String phoneNumber = "";
String message = "";
bool waitingForNumber = false;
bool waitingForMessage = false;

void setup() {
  Serial.begin(9600);
  sim800.begin(9600);
  
  delay(3000); // Wait for module to initialize
  
  Serial.println("=== SIM800A SMS Sender ===");
  Serial.println("Initializing module...");
  
  // Initialize SIM800A
  sim800.println("AT");
  delay(1000);
  
  // Set SMS to text mode
  sim800.println("AT+CMGF=1");
  delay(1000);
  
  // Set character set to GSM
  sim800.println("AT+CSCS=\"GSM\"");
  delay(1000);
  
  Serial.println("Module ready!");
  Serial.println("\nType 'SEND' to send a new SMS");
  Serial.println("-------------------------------");
}

void loop() {
  // Read from SIM800A and print to Serial Monitor
  if (sim800.available()) {
    String response = sim800.readString();
    Serial.print(response);
  }
  
  // Read from Serial Monitor
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (!waitingForNumber && !waitingForMessage) {
      // Waiting for SEND command
      if (input.equalsIgnoreCase("SEND")) {
        waitingForNumber = true;
        phoneNumber = "";
        message = "";
        Serial.println("\nEnter recipient phone number (with country code, e.g., +639123456789):");
      } else {
        Serial.println("Type 'SEND' to send a new SMS");
      }
    }
    else if (waitingForNumber) {
      // Got phone number
      phoneNumber = input;
      Serial.print("Phone number set to: ");
      Serial.println(phoneNumber);
      Serial.println("\nEnter your message:");
      waitingForNumber = false;
      waitingForMessage = true;
    }
    else if (waitingForMessage) {
      // Got message
      message = input;
      Serial.print("Message: ");
      Serial.println(message);
      Serial.println("\nPress ENTER to send, or type 'CANCEL' to abort:");
      waitingForMessage = false;
    }
    else if (input.equalsIgnoreCase("CANCEL")) {
      Serial.println("\nSMS cancelled.");
      Serial.println("\nType 'SEND' to send a new SMS");
      phoneNumber = "";
      message = "";
    }
    else {
      // Send the SMS
      sendSMS(phoneNumber, message);
      phoneNumber = "";
      message = "";
    }
  }
}

void sendSMS(String number, String text) {
  Serial.println("\n--- Sending SMS ---");
  Serial.print("To: ");
  Serial.println(number);
  Serial.print("Message: ");
  Serial.println(text);
  Serial.println("-------------------");
  
  // Set SMS format to text mode
  sim800.println("AT+CMGF=1");
  delay(1000);
  
  // Set recipient number
  sim800.print("AT+CMGS=\"");
  sim800.print(number);
  sim800.println("\"");
  delay(1000);
  
  // Send message
  sim800.print(text);
  delay(500);
  
  // Send Ctrl+Z to finish
  sim800.write(26);
  delay(5000);
  
  Serial.println("\nSMS sent!");
  Serial.println("\nType 'SEND' to send another SMS");
  Serial.println("-------------------------------");
}
