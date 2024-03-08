
int fileNum = 0;
#include <string>
#include <sstream>
std::string fileName;
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
const char *ssid = "NotARocket";
const char *password = "xX_JerrySIMP_Xx";
// IP: 192.168.4.1
using namespace std;

#include <Adafruit_ICM20X.h>
#include <Adafruit_ICM20948.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include <Wire.h>

#include <ESP32Servo.h>
const int ParachuteServo = 17; //ADD NUMBER
Servo servoP;
const int AirbrakeServo = 16;
Servo servoAB;
#include "FS.h"
#include "SD.h"
#include "SPI.h"

Adafruit_ICM20948 icm;
Adafruit_BMP3XX bmp;


#define ICM_20948_USE_DMP
// For SPI mode, we need a CS pin
#define ICM_CS 26
// For software-SPI mode we need SCK/MOSI/MISO pins
#define ICM_SCK 33
#define ICM_MISO 25
#define ICM_MOSI 32

#define SD_SCK  14
#define SD_MISO  12
#define SD_MOSI  13
#define SD_CS  15

#define BMP_SCK 18
#define BMP_MISO 19
#define BMP_MOSI 23
#define BMP_CS 5

#define seapressure (1013.25)

float GyroErrorX, GyroErrorY, GyroErrorZ;
float AccErrorX, AccErrorY;
float GyroX, GyroY, GyroZ;
float AccX, AccY, AccZ;
float accAngleX, accAngleY;
unsigned long elapsedTime, currentTime, previousTime;
float gyroAngleX, gyroAngleY, gyroAngleZ;
float roll, pitch, yaw;

float topAltitude = 0;
const float deploymentAltitudeDrop = 1; // 15 feet in meters
float startAltitude;
const float WifiCutoff = 0.5; // 2 meter increase causes wifi to stop
const float AirbrakeDeployAltitude = 92; // 300 ft

int errorCount = 0;

SPIClass spi = SPIClass(HSPI);







// 8 boolean global variables for the rocket
uint8_t stateHolder = 0; // Holds up to 8 states, each bit represents a different state


WiFiServer server(80);


void setup() {
  Serial.begin(115200);
  Serial.println("setup");
  

  // WiFi Setup
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

  setState(0, false); // Active launch State + Wifi
  setState(1, false); // Parachute Deployed
  setState(2, false); // Flight ready state
  setState(3, false); // Recording State
  setState(4, false); // Airbrake Servo State

  spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  

  Serial.println("Adafruit ICM20948 test!");
  //initialize
  if (! bmp.begin_SPI(BMP_CS, BMP_SCK, BMP_MISO, BMP_MOSI)) { //software spi
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");
    
  }
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);
  if (!SD.begin(SD_CS,spi,80000000)) {
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  if (!icm.begin_SPI(ICM_CS, ICM_SCK, ICM_MISO, ICM_MOSI)) {

    Serial.println("Failed to find ICM20948 chip");
    
  }
  
  icm.setAccelRange(ICM20948_ACCEL_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (icm.getAccelRange()) {
    case ICM20948_ACCEL_RANGE_2_G:
      Serial.println("+-2G");
      break;
    case ICM20948_ACCEL_RANGE_4_G:
      Serial.println("+-4G");
      break;
    case ICM20948_ACCEL_RANGE_8_G:
      Serial.println("+-8G");
      break;
    case ICM20948_ACCEL_RANGE_16_G:
      Serial.println("+-16G");
      break;
  }
  //Serial.println("OK");

  icm.setGyroRange(ICM20948_GYRO_RANGE_2000_DPS);
  Serial.print("Gyro range set to: ");
  switch (icm.getGyroRange()) {
    case ICM20948_GYRO_RANGE_250_DPS:
      Serial.println("250 degrees/s");
      break;
    case ICM20948_GYRO_RANGE_500_DPS:
      Serial.println("500 degrees/s");
      break;
    case ICM20948_GYRO_RANGE_1000_DPS:
      Serial.println("1000 degrees/s");
      break;
    case ICM20948_GYRO_RANGE_2000_DPS:
      Serial.println("2000 degrees/s");
      break;
  }

  // icm.setAccelRateDivisor(4095);
  uint16_t accel_divisor = icm.getAccelRateDivisor();
  float accel_rate = 1125 / (1.0 + accel_divisor);


  // icm.setGyroRateDivisor(255);
  uint8_t gyro_divisor = icm.getGyroRateDivisor();
  float gyro_rate = 1100 / (1.0 + gyro_divisor);



  icm.setMagDataRate(AK09916_MAG_DATARATE_100_HZ);
  Serial.print("Magnetometer data rate set to: ");
  switch (icm.getMagDataRate()) {
    case AK09916_MAG_DATARATE_SHUTDOWN:
    //Serial.println("Shutdown");
    break;
    case AK09916_MAG_DATARATE_SINGLE:
    //Serial.println("Single/One shot");
    break;
    case AK09916_MAG_DATARATE_10_HZ:
    //Serial.println("10 Hz");
    break;
    case AK09916_MAG_DATARATE_20_HZ:
    //Serial.println("20 Hz");
    break;
    case AK09916_MAG_DATARATE_50_HZ:
    //Serial.println("50 Hz");
    break;
    case AK09916_MAG_DATARATE_100_HZ:
    //Serial.println("100 Hz");
    break;
  }
  Serial.println();

  // to calculate IMU error
  // calculate_IMU_error();
  // return;
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  servoP.setPeriodHertz(50); 
  servoP.attach(ParachuteServo, 500, 2400); // or 500 2400
  servoP.write(180);

  servoAB.setPeriodHertz(50); 
  servoAB.attach(AirbrakeServo, 500, 2400); // or 500 2400
  servoAB.write(180);
}

void loop() {
  if(!getState(0)) {
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
  
              if (!getState(3)) {
              client.print("<a href=\"/START_RECORDING\">START RECORDING</a><br>");
              }
              if (getState(3)) {
                client.print("<a href=\"/END_RECORDING\">END RECORDING</a><br>");
              }
              if (getState(2)) {
                client.print("<a href=\"/DEACTIVATE\">FLIGHT READY </a><br>");
              }
              if (!getState(2)) {
                client.print("<a href=\"/ACTIVATE\">SYSTEMS OFFLINE</a><br>");
              }
              if (!getState(1)) {
                client.print("<a href=\"/P_DEPLOY\">DEPLOY PARACHUTE</a><br>");
              }
              if (getState(1)) {
                client.print("<a href=\"/P_LOCK\">LOCK PARACHUTE</a><br>");
              }
              if (!getState(4)) {
                client.print("<a href=\"/AB_OPEN\">OPEN AIRBRAKES</a><br>");
              }
              if (getState(4)) {
                client.print("<a href=\"/AB_CLOSE\">CLOSE AIRBRAKES</a><br>");
              }
              client.print("<a href=\"/REFRESH\">REFRESH</a><br>");
  
              break;
            } else {
              currentLine = "";
            }
          } else if (c != '\r') {
            currentLine += c;
          }
  
  
  
          if (currentLine.endsWith("GET /START_RECORDING")) {
            Serial.print("START RECORDING");
            toggle(3);
            fileNum += 1;
            fileName = "/Recording" + std::to_string(fileNum) + ".txt";
            writeFile(SD, fileName.c_str(), "");
          } else if (currentLine.endsWith("GET /END_RECORDING")) {
            Serial.print("END RECORDING");
            toggle(3);
          } else if (currentLine.endsWith("GET /ACTIVATE")) {
            toggle(2);
            setState(1, false);
            servoP.write(180);
            setState(4, false);
            servoAB.write(180);
            topAltitude = 0;
            startAltitude = bmp.readAltitude(seapressure);
          } else if (currentLine.endsWith("GET /DEACTIVATE")) {
            toggle(2);
          } else if (currentLine.endsWith("GET /P_LOCK")) {
            toggle(1);
            servoP.write(180);
          } else if (currentLine.endsWith("GET /P_DEPLOY")) {
            toggle(1);
            servoP.write(0);
          } else if (currentLine.endsWith("GET /AB_OPEN")) {
            toggle(4);
            servoAB.write(0);
          } else if (currentLine.endsWith("GET /AB_CLOSE")) {
            toggle(4);
            servoAB.write(180);
          } 
          
          
  
        }
      }
      client.stop();
    }
    }
      // Get a new normalized sensor event
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t mag;
    sensors_event_t temp;
    icm.getEvent(&accel, &gyro, &temp, &mag);
    calculate_IMU_error();
    AccX = accel.acceleration.x;
    AccY = accel.acceleration.y;
    AccZ = accel.acceleration.z;
  
      // Sum all readings
    accAngleX = (atan(AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 / PI) - 0.58;
    accAngleY = (atan(-1 * AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / PI) + 1.58;
  
    // === Read gyroscope data === //
    previousTime = currentTime; // Previous time is stored before the actual time read
    currentTime = millis(); // Current time actual time read
    elapsedTime = (currentTime - previousTime) / 1000; // Divide by 1000 to get seconds
  
    // correct the gyro.gyro.x, y and z values
    // take Gyro Error from IMU Error
    GyroX = (gyro.gyro.x + 0.56); // GyroError x
    GyroY = (gyro.gyro.y - 2 ); // GyroError y
    GyroZ = (gyro.gyro.z + 0.79); // GyroError z
  
    // Currently the raw values are in degrees per seconds, deg/s, so we need to multiply by sendonds (s) to get the angle in degrees
    gyroAngleX = gyroAngleX + GyroX * elapsedTime; // deg/s * s = deg
    gyroAngleY = gyroAngleY + GyroY * elapsedTime;
    // Fehler =???
    gyroAngleZ = gyroAngleZ + GyroZ * elapsedTime;
    // Complementary filter - combine acceleromter and gyro angle values
    roll = 0.96 * gyroAngleX + 0.04 * accAngleX;
    pitch = 0.96 * gyroAngleY + 0.04 * accAngleY;
  
  
    if (getState(3)) {
  
      Serial.println("Recording");
      std::string dataToAppend = std::to_string(accel.acceleration.x) + "," + std::to_string(accel.acceleration.y) + "," + std::to_string(accel.acceleration.z) + "," + std::to_string(gyro.gyro.x) + "," + std::to_string(gyro.gyro.y) + "," + std::to_string(gyro.gyro.z) + "," + std::to_string(bmp.temperature) + "," + std::to_string(bmp.pressure) + "," + std::to_string(bmp.readAltitude(seapressure)) + "\n";
      appendFile(SD, fileName.c_str(), dataToAppend.c_str()); 
  
    }
    if(getState(2)) {
      Serial.println(topAltitude);
     if (bmp.readAltitude(seapressure) > topAltitude) {
       topAltitude = bmp.readAltitude(seapressure);
        Serial.println("Height Increase");
      }

      if (bmp.readAltitude(seapressure) > (startAltitude + WifiCutoff) && !getState(0)) {
        setState(0, true);
        Serial.println("Wifi Cutoff");
      }

      if (bmp.readAltitude(seapressure) > (startAltitude + AirbrakeDeployAltitude) && !getState(4)) {
        setState(4, true);
        servoAB.write(0);
        Serial.println("Airbrakes Deployed");
      }

      if (((topAltitude - bmp.readAltitude(seapressure)) >= deploymentAltitudeDrop) && !getState(1)) {
        servoP.write(0); // Deploy parachute
        servoAB.write(180); // Close Airbrakes
        setState(1, true); // update parachute var
        setState(0, false); // update launched var, reenables wifi
        setState(2, false); // End flight ready
        setState(4, false); // Close airbrakes
        Serial.println("Parachute deployed due to altitude drop below threshold.");
      }
  }
    
    delay(500);
    Serial.println("logged");
  }
  
  
  void setState(int var, bool state) {
    if (state) {
      stateHolder |= 1 << var; // Set bit var to 1
    } else {
      stateHolder &= ~(1 << var); // Set bit var to 0
    }
  }
  
  // Function to get the state of the bit (bit 0)
  bool getState(int var) {
    return (stateHolder & (1 << var)) != 0;
  }
  void toggle(int var) {
    if (getState(var)) {
      setState(var, false);
    }
    else {
      setState(var, true);
    }
  }
  
  void writeFile(fs::FS &fs, const char * path, const char * message) {
    Serial.printf("Writing file: %s\n", path);
  
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }
    if (file.print(message)) {
      Serial.println("File written");
    } else {
      Serial.println("Write failed");
    }
    file.close();
  }
  
  void appendFile(fs::FS &fs, const char * path, const char * message) {
    //Serial.printf("Appending to file: %s\n", path);
  
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
      //Serial.println("Failed to open file for appending");
      return;
    }
    if (file.print(message)) {
      //Serial.println("Message appended");
    } else {
     // Serial.println("Append failed");
    }
    file.close();
  }
  
  void calculate_IMU_error() {
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t mag;
    sensors_event_t temp;
    icm.getEvent(&accel, &gyro, &temp, &mag);
  
    // We can call this funtion in the setup section to calculate the accelerometer and gyro data error. From here we will get the error values used in the above equations printed on the Serial Monitor.
    // Note that we should place the IMU flat in order to get the proper values, so that we then can the correct values
    // Read accelerometer values 200 times
    while (errorCount < 200) {
  
      AccX = accel.acceleration.x;
      AccY = accel.acceleration.y;
      AccZ = accel.acceleration.z;
  
      // Sum all readings
      AccErrorX = AccErrorX + ((atan((AccY) / sqrt(pow((AccX), 2) + pow((AccZ), 2))) * 180 / PI));
      AccErrorY = AccErrorY + ((atan(-1 * (AccX) / sqrt(pow((AccY), 2) + pow((AccZ), 2))) * 180 / PI));
      errorCount++;
    }
    //Divide the sum by 200 to get the error value
    AccErrorX = AccErrorX / 200;
    AccErrorY = AccErrorY / 200;
    errorCount = 0;
    // Read gyro values 200 times
    while (errorCount < 200) {
  
      GyroX = gyro.gyro.x;
      GyroY = gyro.gyro.y;
      GyroZ = gyro.gyro.z;
  
      // Sum all readings
      GyroErrorX = GyroErrorX + (GyroX / 131.0);
      GyroErrorY = GyroErrorY + (GyroY / 131.0);
      GyroErrorZ = GyroErrorZ + (GyroZ / 131.0);
      errorCount++;
    }
    //Divide the sum by 200 to get the error value
    GyroErrorX = GyroErrorX / 200;
    GyroErrorY = GyroErrorY / 200;
    GyroErrorZ = GyroErrorZ / 200;
    // Print the error values on the Serial Monitor
    // Serial.print("AccErrorX: ");
    // Serial.println(AccErrorX);
    // Serial.print("AccErrorY: ");
    // Serial.println(AccErrorY);
    // Serial.print("GyroErrorX: ");
    // Serial.println(GyroErrorX);
    // Serial.print("GyroErrorY: ");
    // Serial.println(GyroErrorY);
    // Serial.print("GyroErrorZ: ");
    // Serial.println(GyroErrorZ);
}
