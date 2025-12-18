#include "Servo.h"

#define SERVO_PIN 12
#define SERVO_TIME 3000

Servo servinho;

int servoState;
unsigned long servoTimer;

void setup() {
  servinho.attach(SERVO_PIN);

  servoState = 0;
}

void loop() {
  if (!servoState){
    servinho.write(1);
    servoState = 1;
    servoTimer = millis();
  } else if ((millis() - servoTimer) > SERVO_TIME){
    servinho.write(0);
    servoState = 0;
  }
}
