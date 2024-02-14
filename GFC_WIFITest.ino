#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#define LED_BUILTIN 2

const char *ssid = "NotARocket";
const char *password = "xX_JerrySIMP_Xx";

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

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

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
        Serial.print(getState(0));
        Serial.println();

      }
    }
    client.stop();
  }
}
