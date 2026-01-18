#include <Servo.h>

// Create servo objects
Servo servo1;
Servo servo2;

// Define servo pins
const int SERVO1_PIN = 9;
const int SERVO2_PIN = 10;

void setup() {
  // Attach servos to their pins
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  
  // Initialize servos to center position (90 degrees)
  servo1.write(90);
  servo2.write(90);
  
  // Optional: Start serial communication for debugging
  Serial.begin(9600);
  Serial.println("Two Servo Control Ready");
}

void loop() {
  // Example 1: Sweep both servos together from 0 to 180 degrees
  for (int pos = 0; pos <= 180; pos++) {
    servo1.write(pos);
    servo2.write(pos);
    delay(15);
  }
  
  delay(500);
  
  // Sweep back from 180 to 0 degrees
  for (int pos = 180; pos >= 0; pos--) {
    servo1.write(pos);
    servo2.write(pos);
    delay(15);
  }
  
  delay(500);
  
  // Example 2: Move servos in opposite directions
  for (int pos = 0; pos <= 180; pos++) {
    servo1.write(pos);           // Servo 1 goes 0 to 180
    servo2.write(180 - pos);     // Servo 2 goes 180 to 0
    delay(15);
  }
  
  delay(500);
  
  // Return to center
  servo1.write(90);
  servo2.write(90);
  delay(1000);
}
