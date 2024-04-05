
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include <Wire.h>

#include <ESP32Servo.h>
const int ParachuteServo = 17; //ADD NUMBER
Servo servoP;

#include "FS.h"
#include "SPI.h"

Adafruit_BMP3XX bmp;

#define BMP_SCK 18
#define BMP_MISO 19
#define BMP_MOSI 23
#define BMP_CS 5

#define seapressure (1013.25)


float topAltitude = 0;
const float deploymentAltitudeDrop = 0.5; // 15 feet in meters
bool Deployed = 0;

int errorCount = 0;

SPIClass spi = SPIClass(HSPI);







// 8 boolean global variables for the rocket




void setup() {

  


  if (! bmp.begin_SPI(BMP_CS, BMP_SCK, BMP_MISO, BMP_MOSI)) { //software spi
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");
    
  }
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);



  Serial.println();

  // to calculate IMU error
  // calculate_IMU_error();
  // return;
  ESP32PWM::allocateTimer(0);
  servoP.setPeriodHertz(50); 
  servoP.attach(ParachuteServo, 500, 2500); // or 500 2400
  servoP.write(180);


}

void loop() {



     if (bmp.readAltitude(seapressure) > topAltitude) {
       topAltitude = bmp.readAltitude(seapressure);
        Serial.println("Height Increase");
      }
      Serial.println("\n \n");
      Serial.println(topAltitude);
      Serial.println(bmp.readAltitude(seapressure));

      if (((topAltitude - bmp.readAltitude(seapressure)) >= deploymentAltitudeDrop) && !Deployed) {
        servoP.write(0); // Deploy parachute
        Deployed = 1;
        Serial.println("Parachute deployed due to altitude drop below threshold.");
      }
    delay(50);
  }


  
  