#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>
#include "Adafruit_BMP3XX.h"
#include <Wire.h>

#include <FS.h>
#include "SPIFFS.h"
#include "SPI.h"
#include <ESP32Servo.h>
#include <WiFi.h>
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
unsigned long launchTime = 0;
char* nowTime = "";
char* nowHeight = "";

const char* ssid     = "Willy's Wonky Willy";
const char* password = "gfsc2025";
//192.168.4.1

WiFiServer server(80);

String header;

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}
void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed");
  }
  file.close();
}


void setup() {

  

  Serial.begin(115200);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  Serial.println("Configuring access point...");
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    while (1);
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  Serial.println("Server started");


  // Serial.print("Connecting to ");
  // Serial.println(ssid);
  // WiFi.softAP(ssid, password);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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
  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // UI for the client to control motors and LEDs
            //client.print("<a href=\"/ARM\">ARM</a><br>");
            //if (getState(0)) {
            //  client.print("<a href=\"/LAUNCH\">LAUNCH</a><br>");
            //}
            //if (!getState(4)) {
            //  client.print("<a href=\"/CLEAR_LAST_RECORDING\">CLEAR LAST RECORDING</a><br>");
            //}
            //if (getState(4)) {
            //  client.print("<a href=\"/CLEAR_VERIFICATION\">ARE YOU SURE?</a><br>");
            //}
            
            client.print("<a href=\"/START_RECORDING\">START RECORDING</a><br>");
            
            
            client.print("<a href=\"/END_RECORDING\">END RECORDING</a><br>");
            
            
            client.print("<a href=\"/DEACTIVATE\">FLIGHT READY </a><br>");
            
           
            client.print("<a href=\"/ACTIVATE\">SYSTEMS OFFLINE</a><br>");
            

            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }


      
        
        

      }
    }
    client.stop();
  }




  currentTime = millis() - launchTime;
  currentHeight = bmp.readAltitude(seapressure);
  dtostrf(currentTime, 5, 2, nowTime);
  dtostrf(currentHeight, 5, 2, nowHeight);
  appendFile(SPIFFS, "/test.txt", nowTime);
  appendFile(SPIFFS, "/test.txt", ",");
  appendFile(SPIFFS, "/test.txt", nowHeight);

  Serial.println(topHeight);
  Serial.println(currentHeight);
  
  Serial.println();
  if(currentHeight+3<topHeight && launched){
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
