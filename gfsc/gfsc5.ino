/***************************************************************************
  This is a library for the BMP3XX temperature & pressure sensor

  Designed specifically to work with the Adafruit BMP388 Breakout
  ----> http://www.adafruit.com/products/3966

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>
#include "Adafruit_BMP3XX.h"
#include <Wire.h>

#include <FS.h>
#include "SPIFFS.h"
#include "SPI.h"
#include <ESP32Servo.h>

#define BMP_SCK 48
#define BMP_MISO 47
#define BMP_MOSI 38
#define BMP_CS 21

#define PServo 4
#define AServo 1

#define seapressure (1013.25)

float topHeight;
float currentHeight;
bool launched = false;
unsigned long currentTime;
unsigned long launchTime = 0;

Servo ParachuteServo;
Servo AirbrakeServo;
Adafruit_BMP3XX bmp;
Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);

  Wire.setPins(17, 18);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  //initialize
  if (! bmp.begin_SPI(BMP_CS, BMP_SCK, BMP_MISO, BMP_MOSI)) { //software spi
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");
    while (1);
  }
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_16X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_200_HZ);

  Serial.println();


  ESP32PWM::allocateTimer(0);
  ParachuteServo.setPeriodHertz(50); 
  ParachuteServo.attach(PServo);
  ParachuteServo.attach(PServo, 500, 2400);
  AirbrakeServo.setPeriodHertz(50); 
  AirbrakeServo.attach(AServo);
  AirbrakeServo.attach(AServo, 500, 2400);

  for(int i = 0; i<=15; i++){
    Serial.println(bmp.readAltitude(seapressure));
    bmp.readAltitude(seapressure);
  }
  currentHeight = bmp.readAltitude(seapressure) + 1000;
  AirbrakeServo.write(0);
}

void loop() {
  currentTime = millis() - launchTime;
  if(bmp.readAltitude(seapressure)+1000<currentHeight+70&&bmp.readAltitude(seapressure)+1000>currentHeight-70){
    currentHeight = bmp.readAltitude(seapressure)+1000;    
  }
  Serial.println(currentHeight);
  Serial.println(topHeight);

  
  if(currentHeight+7<topHeight){
    ParachuteServo.write(270);
    AirbrakeServo.write(100);

    Serial.println("deployed");
    
  }else if(currentHeight>topHeight){
    topHeight = currentHeight;
  }
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  
  if (sq(a.acceleration.x)+sq(a.acceleration.y)+sq(a.acceleration.z)>3400&&!launched){
    launchTime = millis();
    launched = true;
  }
  delay(50);
}
