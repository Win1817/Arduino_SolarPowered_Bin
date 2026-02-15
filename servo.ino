#include <Servo.h>

// Servo setup
Servo servoBio;
Servo servoNon;

const int servoBioPin = 6;       // BIO bin servo signal
const int servoNonPin = 7;       // NON-BIO bin servo signal

// Swap the angles: now 0 = unlocked, 90 = locked
const int SERVO_UNLOCKED = 0;    // Servo fully open
const int SERVO_LOCKED = 90;     // Servo fully locked

void setup() {
  Serial.begin(9600);

  // Attach servos
  servoBio.attach(servoBioPin);
  servoNon.attach(servoNonPin);

  // Start both unlocked
  servoBio.write(SERVO_UNLOCKED);
  servoNon.write(SERVO_UNLOCKED);

  Serial.println("Dual Servo Test Ready!");
  Serial.println("Commands:");
  Serial.println("LOCKBIO / UNLOCKBIO");
  Serial.println("LOCKNON / UNLOCKNON");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Remove whitespace

    if (command.equalsIgnoreCase("LOCKBIO")) {
      servoBio.write(SERVO_LOCKED);
      Serial.println("BIO Servo LOCKED");
    } 
    else if (command.equalsIgnoreCase("UNLOCKBIO")) {
      servoBio.write(SERVO_UNLOCKED);
      Serial.println("BIO Servo UNLOCKED");
    } 
    else if (command.equalsIgnoreCase("LOCKNON")) {
      servoNon.write(SERVO_LOCKED);
      Serial.println("NON-BIO Servo LOCKED");
    } 
    else if (command.equalsIgnoreCase("UNLOCKNON")) {
      servoNon.write(SERVO_UNLOCKED);
      Serial.println("NON-BIO Servo UNLOCKED");
    } 
    else {
      Serial.println("Unknown command. Use LOCKBIO, UNLOCKBIO, LOCKNON, UNLOCKNON");
    }
  }
}
