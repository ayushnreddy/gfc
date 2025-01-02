#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>
#include "Adafruit_BMP3XX.h"
#include <Wire.h>

#include <FS.h>
#include "SPIFFS.h"
#include "SPI.h"
#include <ESP32Servo.h>
//#include <ESP32Servo.h>
#define ParachuteServo 27 //ADD NUMBER

Servo servoP;

Adafruit_BMP3XX bmp;
Adafruit_MPU6050 mpu;

//bmp
#define BMP_SCK 18
#define BMP_MISO 19
#define BMP_MOSI 23
#define BMP_CS 5

#define seapressure (1013.25)

float topHeight;
float currentHeight;
bool launched = false;
unsigned long currentTime;

const char* ssid     = "Willy's Wonky Willy";
const char* password = "gfsc2025";

WiFiServer server(80);

String header;

void writeFile(fs::FS &fs, const char *path, const char *message) {

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    return;
  }
  if (file.print(message)) {
    Serial.println("w");
  }
  file.close();
  File file = fs.open(path, FILE_APPEND);
}
void appendFile(fs::FS &fs, const char *path, const char *message) {


  if (!file) {
    return;
  }
  if (file.print(message)) {
    Serial.println("a");
  }

}


void setup() {


  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());
  server.begin();
  while (!Serial)
  delay(10); // will pause Zero, Leonardo, etc until serial console opens

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
  servoP.setPeriodHertz(50); 
  servoP.attach(ParachuteServo);
  servoP.attach(ParachuteServo, 500, 2400);
  for(int i = 0; i<=5; i++){
    Serial.println(bmp.readAltitude(seapressure));
    bmp.readAltitude(seapressure);
  }

  writeFile(SPIFFS, "/test.txt", "start");

}



//////////////////////////// loop ////////////////////////

void loop() {
  WiFiClient client = server.available();
  currentTime = millis() - launchTime;
  currentHeight = bmp.readAltitude(seapressure);
  appendFile(SPIFFS, "/test.txt", currentTime);
  appendFile(SPIFFS, "/test.txt", ",");
  appendFile(SPIFFS, "/test.txt", currentHeight);

  Serial.println(topHeight);
  Serial.println(currentHeight);
  
  Serial.println();
  if(currentHeight+5<topHeight && launched){
    servoP.write(300);
    Serial.println("deployed");
    
  }else if(currentHeight>topHeight){
    topHeight = bmp.readAltitude(seapressure);
  }
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  
  if (sq(a.acceleration.x)+sq(a.acceleration.y)+sq(a.acceleration.z)>3400){
    launchTime = millis();
    
    Serial.println("launched");
    launched = true;
  }
  delay(500);
}
