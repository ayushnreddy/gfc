#include <ESP32Servo.h>
#define ParachuteServo 1 //ADD NUMBER

Servo servoP; //Initialize parachut servo class
void setup() {
  ESP32PWM::allocateTimer(0);
  servoP.setPeriodHertz(50); 
  servoP.attach(ParachuteServo, 500, 1000); // or 500 2400

}

void loop() {
  // put your main code here, to run repeatedly:
  servoP.write(0);
  delay(1000);
  servoP.write(90);
  delay(1000);
  servoP.write(180);
  delay(1000);
}
