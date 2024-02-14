#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#define LED_BUILTIN 2

const char *ssid = "NotARocket";
const char *password = "jerryismydad";

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
}

void loop() {
  int Armed = 0;
  int Clear = 0;
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
            if (Armed == 1) {
              client.print("<a href=\"/LAUNCH\">LAUNCH</a><br>");
            }

            client.print("<a href=\"/CLEAR_LAST_RECORDING\">CLEAR LAST RECORDING</a><br>");
            if (Clear == 1) {
              client.print("<a href=\"/CLEAR_VERIFICATION\">ARE YOU SURE?</a><br>");
            }
            client.print("<a href=\"/START_RECORDING\">START RECORDING</a><br>");
            client.print("<a href=\"/END_RECORDING\">END RECORDING</a><br>");


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
          if (Armed == 1) {
            Armed = 0;
          }
          else {
            Armed = 1;
          }
        } else if (currentLine.endsWith("GET /LAUNCH")) {
          Serial.print("LAUNCH");
        } else if (currentLine.endsWith("GET /CLEAR_LAST_RECORDING")) {
          Serial.print("CLEAR LAST RECORDING");
          if (Clear == 1) {
            Clear = 0;
          }
          else {
            Clear = 1;
          }
        } else if (currentLine.endsWith("GET /CLEAR_VERIFICATION")) {
          Serial.print("Clear Verification");
        } else if (currentLine.endsWith("GET /START_RECORDING")) {
          Serial.print("START RECORDING");
        } else if (currentLine.endsWith("GET /SERVO_180")) {
          Serial.print("END RECORDING");
        }
        Serial.print(Armed);
        Serial.println();

      }
    }
    client.stop();
  }
}
