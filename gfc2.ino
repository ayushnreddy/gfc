
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"

#define BMP_SCK 18
#define BMP_MISO 19
#define BMP_MOSI 23
#define BMP_CS 5

#define seapressure (1013.25)
String logs;
int i, k;
int logging [3][3] = {{},{},{}};

Adafruit_BMP3XX bmp;

void setup() {
  Serial.begin(115200);
  Serial.println("initialized");
  logs = "";
  i = 0;
  k = 0;
  while (!Serial);
  //if (! bmp.begin_SPI(BMP_CS)) { //hardwar spi
  if (! bmp.begin_SPI(BMP_CS, BMP_SCK, BMP_MISO, BMP_MOSI)) { //software spi
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");
    while (1);
  }
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);
}

void loop() {
  if (! bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
  if(i<3){
    
    logging[0][i]=bmp.temperature;
    logging[1][i]=bmp.pressure;
    logging[2][i]=bmp.readAltitude(seapressure);
    delay(3000);
    i++;
    
  }
  Serial.println("iterated");
  if(i>=3){
    for(int k = 0; k<3; k++){
      Serial.print(logging[0][k] + "");
    }
    Serial.println("");
    for(int k = 0; k<3; k++){
      Serial.print(logging[1][k] + "");
    }
    Serial.println("");
    for(int k = 0; k<3; k++){
      Serial.print(logging[2][k] + "");
    }
    
  }
  
  
  // Serial.print("Temperature = ");
  // Serial.print(bmp.temperature);
  // Serial.println(" *C");

  // Serial.print("Pressure = ");
  // Serial.print(bmp.pressure / 100.0);
  // Serial.println(" hPa");

  // Serial.print("Approx. Altitude = ");
  // Serial.print(bmp.readAltitude(seapressure));
  // Serial.println(" m");

  // Serial.println();
  // delay(2000);
}
