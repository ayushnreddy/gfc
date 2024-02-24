#include <Adafruit_ICM20X.h>
#include <Adafruit_ICM20948.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include <Wire.h>

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <ESP32Servo.h>
#define ParachuteServo 1 //ADD NUMBER

Servo servoP;

Adafruit_ICM20948 icm;
Adafruit_BMP3XX bmp;
uint16_t measurement_delay_us = 65535; // Delay between measurements for testing

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

int c = 0;

SPIClass spi = SPIClass(HSPI);

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
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void setup() {


  Serial.begin(115200);
  spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  while (!Serial)
  delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit ICM20948 test!");
  //initialize
  if (! bmp.begin_SPI(BMP_CS, BMP_SCK, BMP_MISO, BMP_MOSI)) { //software spi
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");
    while (1);
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
    while (1) {
      delay(10);
    }
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

  //Serial.print("Accelerometer data rate divisor set to: ");
  //Serial.println(accel_divisor);
  //Serial.print("Accelerometer data rate (Hz) is approximately: ");
  //Serial.println(accel_rate);

  // icm.setGyroRateDivisor(255);
  uint8_t gyro_divisor = icm.getGyroRateDivisor();
  float gyro_rate = 1100 / (1.0 + gyro_divisor);

  //Serial.print("Gyro data rate divisor set to: ");
  //Serial.println(gyro_divisor);
  //Serial.print("Gyro data rate (Hz) is approximately: ");
  //Serial.println(gyro_rate);

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
  writeFile(SD, "/test.txt", "test\n");

  // to calculate IMU error
  // calculate_IMU_error();
  // return;
  ESP32PWM::allocateTimer(0);
  servoP.setPeriodHertz(50); 
  servoP.attach(ParachuteServo, 500, 2400);

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
  while (c < 200) {

    AccX = accel.acceleration.x;
    AccY = accel.acceleration.y;
    AccZ = accel.acceleration.z;

    // Sum all readings
    AccErrorX = AccErrorX + ((atan((AccY) / sqrt(pow((AccX), 2) + pow((AccZ), 2))) * 180 / PI));
    AccErrorY = AccErrorY + ((atan(-1 * (AccX) / sqrt(pow((AccY), 2) + pow((AccZ), 2))) * 180 / PI));
    c++;
  }
  //Divide the sum by 200 to get the error value
  AccErrorX = AccErrorX / 200;
  AccErrorY = AccErrorY / 200;
  c = 0;
  // Read gyro values 200 times
  while (c < 200) {

    GyroX = gyro.gyro.x;
    GyroY = gyro.gyro.y;
    GyroZ = gyro.gyro.z;

    // Sum all readings
    GyroErrorX = GyroErrorX + (GyroX / 131.0);
    GyroErrorY = GyroErrorY + (GyroY / 131.0);
    GyroErrorZ = GyroErrorZ + (GyroZ / 131.0);
    c++;
  }
  //Divide the sum by 200 to get the error value
  GyroErrorX = GyroErrorX / 200;
  GyroErrorY = GyroErrorY / 200;
  GyroErrorZ = GyroErrorZ / 200;
  // Print the error values on the Serial Monitor
  Serial.print("AccErrorX: ");
  Serial.println(AccErrorX);
  Serial.print("AccErrorY: ");
  Serial.println(AccErrorY);
  Serial.print("GyroErrorX: ");
  Serial.println(GyroErrorX);
  Serial.print("GyroErrorY: ");
  Serial.println(GyroErrorY);
  Serial.print("GyroErrorZ: ");
  Serial.println(GyroErrorZ);
}

//////////////////////////// loop ////////////////////////

void loop() {

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
  Serial.print("Gyro X Error: ");
  Serial.print(GyroX);
  Serial.print("\t");
  Serial.print("Gyro Y Error: ");
  Serial.print(GyroY);
  Serial.print("\t");
  Serial.print("Gyro Z Error: ");
  Serial.println(GyroZ);

  // Currently the raw values are in degrees per seconds, deg/s, so we need to multiply by sendonds (s) to get the angle in degrees
  gyroAngleX = gyroAngleX + GyroX * elapsedTime; // deg/s * s = deg
  gyroAngleY = gyroAngleY + GyroY * elapsedTime;
  // Fehler =???
  gyroAngleZ = gyroAngleZ + GyroZ * elapsedTime;
  // Complementary filter - combine acceleromter and gyro angle values
  roll = 0.96 * gyroAngleX + 0.04 * accAngleX;
  pitch = 0.96 * gyroAngleY + 0.04 * accAngleY;

  // Print the values on the serial monitor
  // Serial.print(roll);
  // Serial.print(" ");
  // Serial.print(pitch);
  // Serial.print(" ");
  // Serial.println(yaw);
  //Serial.println(GyroZ);

  Serial.print("roll: ");
  Serial.print(roll);
  Serial.print("\t\t");
  Serial.print("pitch: ");
  Serial.println(pitch);

  // Display the results (acceleration is measured in m/s^2)
  //
  Serial.print("Accel\tX:");
  //Serial.print(int(accel.acceleration.x * 100));
  Serial.print(accel.acceleration.x);
  Serial.print("\t\t");
  Serial.print("Y:");
  //Serial.print(int(accel.acceleration.y * 100));
  Serial.print(accel.acceleration.y + 0.2);
  Serial.print("\t\t");
  Serial.print("Z:");
  //Serial.print(int(accel.acceleration.z * 100));
  Serial.print(accel.acceleration.z +0.3);
  Serial.print("\t\t");
  Serial.println("m/s^2");
  Serial.print("Mag\tX:");
  //Serial.print(int(mag.magnetic.x * 100));
  Serial.print(mag.magnetic.x);
  Serial.print("\t\t");
  Serial.print("Y:");
  //Serial.print(int(mag.magnetic.y * 100));
  Serial.print(mag.magnetic.y);
  Serial.print("\t\t");
  Serial.print("Z:");
  //Serial.print(int(mag.magnetic.z * 100));
  Serial.print(mag.magnetic.z);
  Serial.print("\t\t");
  Serial.println("uT");

  //Display the results (acceleration is measured in m/s^2)
  Serial.print("Gyro\tX: ");
  //Serial.print(int(gyro.gyro.x * 100));
  Serial.print(gyro.gyro.x);
  Serial.print("\t\t");
  Serial.print("Y: ");
  //Serial.print(int(gyro.gyro.y * 100));
  Serial.print(gyro.gyro.y);
  Serial.print("\t\t");
  Serial.print("Z: ");
  //Serial.print(int(gyro.gyro.z * 100));
  Serial.print(gyro.gyro.z);
  Serial.print("\t\t");
  Serial.println(" radians/s ");

  //Serial.println();

  // Serial.print("Temperature");
  // Serial.print(int(temp.temperature * 100));
  //Serial.print(temp.temperature);
  //Serial.print(" ");
  // Serial.println("degC");

  //Serial.println();

  delay(50);
  //  delay(100);

  // Serial.print(temp.temperature);
  //
  // Serial.print(",");
  //
  Serial.print(accel.acceleration.x);
  Serial.print(","); Serial.print(accel.acceleration.y);
  Serial.print(","); Serial.print(accel.acceleration.z);
  
  Serial.print(",");
  Serial.print(gyro.gyro.x);
  Serial.print(","); Serial.print(gyro.gyro.y);
  Serial.print(","); Serial.print(gyro.gyro.z);
  
  // Serial.print(",");
  // Serial.print(mag.magnetic.x);
  // Serial.print(","); Serial.print(mag.magnetic.y);
  // Serial.print(","); Serial.print(mag.magnetic.z);

  Serial.println("\n");
  appendFile(SD, "/test.txt", std::to_string(accel.acceleration.x).c_str());
  appendFile(SD, "/test.txt", ",");
  appendFile(SD, "/test.txt", std::to_string(accel.acceleration.y + 0.2).c_str());
  appendFile(SD, "/test.txt", ",");
  appendFile(SD, "/test.txt", std::to_string(accel.acceleration.z + 0.3).c_str());
  appendFile(SD, "/test.txt", ",");
  appendFile(SD, "/test.txt", std::to_string(gyro.gyro.x).c_str());
  appendFile(SD, "/test.txt", ",");
  appendFile(SD, "/test.txt", std::to_string(gyro.gyro.y).c_str());
  appendFile(SD, "/test.txt", ",");
  appendFile(SD, "/test.txt", std::to_string(gyro.gyro.z).c_str());
  appendFile(SD, "/test.txt", ",");
  appendFile(SD, "/test.txt", std::to_string(bmp.temperature).c_str());
  appendFile(SD, "/test.txt", ",");
  appendFile(SD, "/test.txt", std::to_string(bmp.pressure).c_str());
  appendFile(SD, "/test.txt", ",");
  appendFile(SD, "/test.txt", std::to_string(bmp.readAltitude(seapressure)).c_str());
  appendFile(SD, "/test.txt", "\n");

  if(bmp.readAltitude(seapressure)+1<topHeight){
    servo.write(300);
  }
  
  delayMicroseconds(measurement_delay_us);
}

/*
*
void loop() {

  // Get a new normalized sensor event
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t mag;
  sensors_event_t temp;
  icm.getEvent(&accel, &gyro, &temp, &mag);

  Serial.print("\t\tTemperature ");
  Serial.print(temp.temperature);
  Serial.println(" deg C");

  // Display the results (acceleration is measured in m/s^2)
  //Serial.print("\t\tAccel_X: ");
  Serial.print("\t\tAccel_X: ");
  Serial.print(accel.acceleration.x);
  Serial.print(" \tY: ");
  Serial.print(accel.acceleration.y);
  Serial.print(" \tZ: ");
  Serial.print(accel.acceleration.z);
  Serial.println(" m/s^2 ");

  Serial.print("\t\tMag X: ");
  Serial.print(mag.magnetic.x);
  Serial.print(" \tY: ");
  Serial.print(mag.magnetic.y);
  Serial.print(" \tZ: ");
  Serial.print(mag.magnetic.z);
  Serial.println(" uT");

  //Display the results (acceleration is measured in m/s^2)
  Serial.print("\t\tGyro X: ");
  Serial.print(gyro.gyro.x);
  Serial.print(" \tY: ");
  Serial.print(gyro.gyro.y);
  Serial.print(" \tZ: ");
  Serial.print(gyro.gyro.z);
  Serial.println(" radians/s ");
  Serial.println();

  //delay(500);
  delay(100);

  // Serial.print(temp.temperature);
  //
  // Serial.print(",");
  //
  // Serial.print(accel.acceleration.x);
  // Serial.print(","); Serial.print(accel.acceleration.y);
  // Serial.print(","); Serial.print(accel.acceleration.z);
  //
  // Serial.print(",");
  // Serial.print(gyro.gyro.x);
  // Serial.print(","); Serial.print(gyro.gyro.y);
  // Serial.print(","); Serial.print(gyro.gyro.z);
  //
  // Serial.print(",");
  // Serial.print(mag.magnetic.x);
  // Serial.print(","); Serial.print(mag.magnetic.y);
  // Serial.print(","); Serial.print(mag.magnetic.z);

  // Serial.println();
  //
  // delayMicroseconds(measurement_delay_us);
}
*/
