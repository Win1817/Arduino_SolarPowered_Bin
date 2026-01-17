/*
 * SIM800A SMS Sender with Module Testing
 * Hardware Connection:
 * SIM800A TX -> Arduino RX (Pin 10)
 * SIM800A RX -> Arduino TX (Pin 11)
 * SIM800A GND -> Arduino GND
 * SIM800A VBAT -> 18650 Battery 3.7V
 */

#include <SoftwareSerial.h>

// Create software serial for SIM800A
SoftwareSerial sim800(10, 11); // RX, TX

String phoneNumber = "";
String message = "";
bool waitingForNumber = false;
bool waitingForMessage = false;
bool moduleReady = false;

void setup() {
  Serial.begin(9600);
  sim800.begin(9600);
  
  Serial.println("=== SIM800A SMS Sender ===");
  Serial.println("Starting module test...\n");
  
  delay(3000); // Wait for module to boot
  
  // Test module functionality
  if (testModule()) {
    moduleReady = true;
    Serial.println("\n✓ Module is ready!");
    Serial.println("\n--- Instructions ---");
    Serial.println("Type 'SEND' to send a new SMS");
    Serial.println("Type 'TEST' to test module again");
    Serial.println("--------------------");
  } else {
    Serial.println("\n✗ Module test FAILED!");
    Serial.println("Please check:");
    Serial.println("- Power supply (3.7V battery)");
    Serial.println("- Wiring connections");
    Serial.println("- SIM card is inserted");
    Serial.println("\nType 'TEST' to try again");
  }
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
    
    // Handle TEST command
    if (input.equalsIgnoreCase("TEST")) {
      Serial.println("\n--- Running Module Test ---");
      if (testModule()) {
        moduleReady = true;
        Serial.println("\n✓ Module is ready!");
      } else {
        moduleReady = false;
        Serial.println("\n✗ Module test FAILED!");
      }
      Serial.println("\nType 'SEND' to send SMS or 'TEST' to test again");
      return;
    }
    
    if (!waitingForNumber && !waitingForMessage) {
      // Waiting for SEND command
      if (input.equalsIgnoreCase("SEND")) {
        if (!moduleReady) {
          Serial.println("\n✗ Module not ready! Run 'TEST' first.");
          return;
        }
        waitingForNumber = true;
        phoneNumber = "";
        message = "";
        Serial.println("\nEnter recipient phone number");
        Serial.println("(with country code, e.g., +639123456789):");
      } else if (input != "") {
        Serial.println("Type 'SEND' to send SMS or 'TEST' to test module");
      }
    }
    else if (waitingForNumber) {
      // Got phone number
      phoneNumber = input;
      Serial.print("✓ Phone number: ");
      Serial.println(phoneNumber);
      Serial.println("\nEnter your message text:");
      Serial.println("(You can type anything you want)");
      waitingForNumber = false;
      waitingForMessage = true;
    }
    else if (waitingForMessage) {
      // Got message
      message = input;
      Serial.print("✓ Message: ");
      Serial.println(message);
      Serial.println("\n--- Ready to Send ---");
      Serial.println("Press ENTER to send");
      Serial.println("Type 'CANCEL' to abort");
      Serial.println("---------------------");
      waitingForMessage = false;
    }
    else if (input.equalsIgnoreCase("CANCEL")) {
      Serial.println("\n✗ SMS cancelled.");
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

bool testModule() {
  bool testPassed = true;
  
  // Clear serial buffer
  while(sim800.available()) sim800.read();
  
  // Test 1: Basic AT command
  Serial.print("Test 1: Module response... ");
  sim800.println("AT");
  delay(1000);
  String response = waitForResponse(2000);
  if (response.indexOf("OK") >= 0) {
    Serial.println("✓ PASS");
  } else {
    Serial.println("✗ FAIL");
    testPassed = false;
  }
  
  // Test 2: SIM card status
  Serial.print("Test 2: SIM card... ");
  sim800.println("AT+CPIN?");
  delay(1000);
  response = waitForResponse(2000);
  if (response.indexOf("READY") >= 0) {
    Serial.println("✓ PASS");
  } else {
    Serial.println("✗ FAIL (SIM not ready)");
    testPassed = false;
  }
  
  // Test 3: Network registration
  Serial.print("Test 3: Network registration... ");
  sim800.println("AT+CREG?");
  delay(1000);
  response = waitForResponse(2000);
  if (response.indexOf("+CREG: 0,1") >= 0 || response.indexOf("+CREG: 0,5") >= 0) {
    Serial.println("✓ PASS");
  } else {
    Serial.println("✗ FAIL (Not registered)");
    testPassed = false;
  }
  
  // Test 4: Signal strength
  Serial.print("Test 4: Signal strength... ");
  sim800.println("AT+CSQ");
  delay(1000);
  response = waitForResponse(2000);
  if (response.indexOf("+CSQ:") >= 0) {
    Serial.println("✓ PASS");
    Serial.print("   Signal: ");
    Serial.println(response);
  } else {
    Serial.println("✗ FAIL");
    testPassed = false;
  }
  
  // Test 5: SMS mode
  Serial.print("Test 5: SMS configuration... ");
  sim800.println("AT+CMGF=1");
  delay(1000);
  response = waitForResponse(2000);
  if (response.indexOf("OK") >= 0) {
    Serial.println("✓ PASS");
    // Set character set
    sim800.println("AT+CSCS=\"GSM\"");
    delay(1000);
  } else {
    Serial.println("✗ FAIL");
    testPassed = false;
  }
  
  return testPassed;
}

String waitForResponse(unsigned long timeout) {
  String response = "";
  unsigned long startTime = millis();
  
  while (millis() - startTime < timeout) {
    while (sim800.available()) {
      char c = sim800.read();
      response += c;
    }
  }
  
  return response;
}

void sendSMS(String number, String text) {
  Serial.println("\n========== SENDING SMS ==========");
  Serial.print("To: ");
  Serial.println(number);
  Serial.print("Message: ");
  Serial.println(text);
  Serial.println("=================================");
  Serial.println("Please wait...");
  
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
  
  // Wait for response
  Serial.println("Waiting for confirmation...");
  String response = waitForResponse(10000);
  
  // Check if SMS was sent successfully
  if (response.indexOf("OK") >= 0 || response.indexOf("+CMGS:") >= 0) {
    Serial.println("\n✓✓✓ SMS SENT SUCCESSFULLY! ✓✓✓");
  } else if (response.indexOf("ERROR") >= 0) {
    Serial.println("\n✗✗✗ SMS FAILED TO SEND ✗✗✗");
    Serial.println("Error details:");
    Serial.println(response);
  } else {
    Serial.println("\n? SMS status unknown");
    Serial.println("Response:");
    Serial.println(response);
  }
  
  Serial.println("\n=================================");
  Serial.println("Type 'SEND' to send another SMS");
  Serial.println("Type 'TEST' to test module");
  Serial.println("=================================\n");
}
