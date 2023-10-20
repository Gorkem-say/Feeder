#include <AccelStepper.h>

// Define the stepper motor connections (ULN2003 driver)
const int motorPin1 = 3; // IN1
const int motorPin2 = 4; // IN2
const int motorPin3 = 5; // IN3
const int motorPin4 = 6; // IN4

// Create an instance of the AccelStepper class
AccelStepper stepper(AccelStepper::FULL4WIRE, motorPin1, motorPin3, motorPin2, motorPin4);

const int forwardDuration = 500; // 2 seconds forward
const int reverseDuration = 3000; // 1 second reverse
unsigned long lastChangeTime = 0;
bool forward = true;

void setup() {
  // Set the maximum speed and acceleration
  stepper.setMaxSpeed(500); // Adjust this to your motor's maximum speed
  stepper.setAcceleration(100); // Adjust this to your desired acceleration

  // Set the initial direction to forward
  stepper.setSpeed(500); // Adjust the speed as needed for forward direction
  forward = true;
}

void loop() {
  unsigned long currentTime = millis();

  switch (forward) {
    case true:
      if (currentTime - lastChangeTime >= forwardDuration) {
        stepper.setSpeed(-1000); // Adjust the speed as needed for reverse direction
        forward = false;
        lastChangeTime = currentTime;
      }
      break;

    case false:
      if (currentTime - lastChangeTime >= reverseDuration) {
        stepper.setSpeed(1000); // Adjust the speed as needed for forward direction
        forward = true;
        lastChangeTime = currentTime;
      }
      break;
  }

  // Move the motor by one step
  stepper.runSpeed();
}
