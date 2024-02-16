

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"

#include <string>
#include <sstream>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
const char *ssid = "NotARocket";
const char *password = "xX_JerrySIMP_Xx";

#define SD_SCK  14
#define SD_MISO  12
#define SD_MOSI  13
#define SD_CS  15

#define BMP_SCK 18
#define BMP_MISO 19
#define BMP_MOSI 23
#define BMP_CS 5

#define seapressure (1013.25)

using namespace std;
Adafruit_BMP3XX bmp;


SPIClass spi = SPIClass(HSPI);

string tempstring = "";

const char * convertChar(double inputValue){
  std::string str = std::to_string(inputValue);
  char *cstr = new char[str.length()+1];
  str.copy(cstr, str.length());
  cstr[str.length()] = '\0';
  return cstr;
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char * path){
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path)){
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char * path){
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if(file){
    len = file.size();
    size_t flen = len;
    start = millis();
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for(i=0; i<2048; i++){
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}



// 8 boolean global variables for the rocket
uint8_t stateHolder = 0; // Holds up to 8 states, each bit represents a different state

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
void toggle(int var){
  if (getState(var)){
    setState(var, false);
  }
  else{
    setState(var, true);
  }
  }

WiFiServer server(80);

void newRecording(int num){
  writeFile(SD, "/pres.txt", "Pressure hPa \n");
  writeFile(SD, "/temp.txt", "Temperature *C \n");
  writeFile(SD, "/alt.txt", "Altitude m \n");
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}
void setup(){
  Serial.begin(115200);
  Serial.println("setup");
  spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS,spi,80000000)) {
    Serial.println("Card Mount Failed");
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
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
  while (!Serial);
  //if (! bmp.begin_SPI(BMP_CS)) { //hardwar spi
  if (! bmp.begin_SPI(BMP_CS, BMP_SCK, BMP_MISO, BMP_MOSI)) { //software spi
    Serial.println("Could not find a valid BMP3 sensor, check wiring!");

  }
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  // listDir(SD, "/", 0);
  // createDir(SD, "/mydir");
  // listDir(SD, "/", 0);
  // removeDir(SD, "/mydir");
  // listDir(SD, "/", 2);
  // writeFile(SD, "/hello.txt", "Hello ");
  // appendFile(SD, "/hello.txt", "World!\n");
  // readFile(SD, "/hello.txt");
  // deleteFile(SD, "/foo.txt");
  // renameFile(SD, "/hello.txt", "/foo.txt");
  // readFile(SD, "/foo.txt");
  // testFileIO(SD, "/test.txt");


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

  setState(0, false); // Armed state
  setState(1, false); // Launch State
  setState(2, false); // Flight ready state
  setState(3, false); // Recording State
  setState(4, false); // Data clear verification state

}

void loop(){

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
            client.print("<a href=\"/ARM\">ARM</a><br>");
            if (getState(0)) {
              client.print("<a href=\"/LAUNCH\">LAUNCH</a><br>");
            }
            if (!getState(4)){
              client.print("<a href=\"/CLEAR_LAST_RECORDING\">CLEAR LAST RECORDING</a><br>");
            }
            if (getState(4)) {
              client.print("<a href=\"/CLEAR_VERIFICATION\">ARE YOU SURE?</a><br>");
            }
            if (!getState(3)){
              client.print("<a href=\"/START_RECORDING\">START RECORDING</a><br>");
            }
            if(getState(3)){
              client.print("<a href=\"/END_RECORDING\">END RECORDING</a><br>");
            }

            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }


        // Check client's request and act
        if (currentLine.endsWith("GET /ARM")) {
          Serial.print("ARM");
          toggle(0);
        } else if (currentLine.endsWith("GET /LAUNCH")) {
          Serial.print("LAUNCH");
          toggle(1);
        } else if (currentLine.endsWith("GET /CLEAR_LAST_RECORDING")) {
          Serial.print("CLEAR LAST RECORDING");
          toggle(4);
        } else if (currentLine.endsWith("GET /CLEAR_VERIFICATION")) {
          Serial.print("Clear Verification");
          toggle(4);
        } else if (currentLine.endsWith("GET /START_RECORDING")) {
          Serial.print("START RECORDING");
          toggle(3);
        } else if (currentLine.endsWith("GET /END_RECORDING")) {
          Serial.print("END RECORDING");
          toggle(3);
        }

      }
    }
    client.stop();
  }
    
  if (getState(3)) {
  appendFile(SD, "/temp.txt", convertChar(bmp.temperature));
  appendFile(SD, "/temp.txt", "\n");
  // dtostrf(bmp.temperature, 7, 0, tempstring);
  appendFile(SD, "/pres.txt", convertChar(bmp.pressure));
  appendFile(SD, "/pres.txt", "\n");
  // dtostrf(bmp.temperature, 7, 0, tempstring);
  appendFile(SD, "/alt.txt", convertChar(bmp.readAltitude(seapressure)));
  appendFile(SD, "/alt.txt", "\n");
  }
  delay(500);
  Serial.println("logged");
  // dtostrf(bmp.temperature, 7, 0, tempstring);
  // Serial.println(tempstring);
 }
