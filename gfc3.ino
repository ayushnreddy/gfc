#include <Wire.h>
#include <Adafruit_BMP3XX.h>
#include <SPI.h>
#include <SD.h>

// Define the pin configuration for the BMP388
#define BMP_CS_PIN 5 // Example GPIO pin for BMP388 chip select

// Define the pin configuration for the microSD card
#define SD_CS_PIN 27

// Create an instance of the BMP388 sensor
Adafruit_BMP3XX bmp;

// Define the file on the microSD card
File dataFile;

void setup() {
  // Start the serial communication
  Serial.begin(115200);

  // Initialize the SPI bus for BMP388
  SPI.begin(18, 19, 23, BMP_CS_PIN); // SCK, MISO, MOSI, SS

  // Initialize the BMP388 sensor
 if (! bmp.begin_SPI(BMP_CS_PIN, 18, 19, 23)) {
    Serial.println("Could not find a valid BMP388 sensor, check wiring!");
    while (1);
  }

  // Initialize the SPI bus for the microSD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Could not initialize SD card, check connections!");
    while (1);
  }

  // Create a new file on the microSD card
  dataFile = SD.open("data.txt", FILE_WRITE);

  // Check if the file was opened successfully
  if (dataFile) {
    Serial.println("Data file opened successfully");
    dataFile.println("Time(ms),Pressure(hPa)"); // Header line
    dataFile.close(); // Close the file
  } else {
    Serial.println("Error opening data file");
  }
}

void loop() {
  // Read pressure from BMP388
  float pressure = bmp.readPressure() / 100.0;

  // Get the current time in milliseconds
  unsigned long currentTime = millis();

  // Open the data file in append mode
  dataFile = SD.open("data.txt", FILE_WRITE);
  
  // Check if the file was opened successfully
  if (dataFile) {
    // Print the data to the file
    dataFile.print(currentTime);
    dataFile.print(",");
    dataFile.println(pressure);
    
    // Close the file
    dataFile.close();
    
    // Print the data to the Serial Monitor
    Serial.print("Time(ms): ");
    Serial.print(currentTime);
    Serial.print(", Pressure(hPa): ");
    Serial.println(pressure);
  } else {
    Serial.println("Error opening data file");
  }

  // Delay for a certain interval (e.g., 1 second)
  delay(1000);
}
