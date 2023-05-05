#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <string>  
using namespace std;

#include "HTTP_Server.h" 
#include "ripple.h"


// WiFi stuff - CHANGE FOR YOUR OWN NETWORK!
const char* ssid = "TP-Link-150";
const char* password = "Cenote#150";

const IPAddress ip(192, 168, 0, 241);  // IP address that THIS DEVICE should request
const IPAddress gateway(192, 168, 0, 1);  // Your router
const IPAddress subnet(255, 255, 255, 0);  // Your subnet mask (find it from your router's admin panel)

WebServer server(80); //Open port number 80 (HTTP)

unsigned long currentTime = 0;
unsigned long previousTime = 0;
const long timeout = 2000;

// Auxiliar variables to store the current output state
int loopFireRippleEnabled = 1;
int manualFireRipple = 0;
int currentNumberofRipples = NUMBER_OF_RIPPLES;
int currentNumberofColors = 7;
short currentDelayBetweenRipples = 3000; /* in milliseconds */
unsigned long currentRippleLifeSpan = 3000; /* in milliseconds */
float currentDecay = 0.985;  // Multiply all LED's by this amount each tick to create fancy fading tails 0.972 good value for rainbow

String SendHTML(void) {
  // Display the HTML web page
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
  ptr += "<link rel=\"icon\" href=\"data:,\">\n";
  // CSS to style the on/off buttons 
  // Feel free to change the background-color and font-size attributes to fit your preferences
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;\n";
  ptr += "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\n";
  ptr += "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\n";
  ptr += ".button2 {background-color: #555555;}\n";
  ptr += "form div {\n";
  ptr += " display: flex;\n";
  ptr += " justify-content: space-between;\n";
  ptr += " margin: 10px 0;\n";
  ptr += "}\n";
  ptr += "form div label {\n";
  ptr += " flex: 1;\n";
  ptr += " text-align: left;\n";
  ptr += "}\n";
  ptr += "form div input {\n";
  ptr += " flex: 1;\n";
  ptr += " text-align: right;\n";
  ptr += "}\n";
  ptr += "</style></head>\n";
  
  /* Web Page Heading */
  ptr += "<body><h1>ESP32 Web Server</h1>\n";
  
  /* Fire manual Ripple button */
  ptr += "<p> Fire Manual Ripple </p>\n";
  ptr += "<p><a href=\"/ManualRipple\"><button class=\"button\">Fire!</button></a></p>\n";
  
  /* Display ON/OFF checkbox for loopFireRippleEnabled */
  if (loopFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"auto-ripple-checkbox\" name=\"auto-ripple\" value=\"1\" onchange=\"toggleAutoRipple(this)\" checked><label for=\"auto-ripple-checkbox\">Automatic Ripples</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"auto-ripple-checkbox\" name=\"auto-ripple\" value=\"1\" onchange=\"toggleAutoRipple(this)\"><label for=\"auto-ripple-checkbox\">Automatic Ripples</label></p>\n";
  }
  
  /* Text input for number of ripples */
  ptr += "<form action=\"/updateInternalVariables\" method=\"post\">\n";
  ptr += "<div><label for=\"NumberofRipples\">Enter number of ripples [1 - 99]:</label>\n";
  ptr += "<input id=\"NumberofRipples\" name=\"NumberofRipples\" value=\"" + String(currentNumberofRipples) + "\"></div>\n";
  ptr += "<div><label for=\"currentDelayBetweenRipples\">Enter delay between ripples in ms [5 - 20000]:</label>\n";
  ptr += "<input id=\"currentDelayBetweenRipples\" name=\"currentDelayBetweenRipples\" value=\"" + String(currentDelayBetweenRipples) + "\"></div>\n";
  ptr += "<div><label for=\"currentRippleLifeSpan\">Enter ripple life span in ms [500 - 20000]:</label>\n";
  ptr += "<input id=\"currentRippleLifeSpan\" name=\"currentRippleLifeSpan\" value=\"" + String(currentRippleLifeSpan) + "\"></div>\n";
  ptr += "<div><label for=\"currentNumberofColors\">Enter desired number of colors [3 - 25]:</label>\n";
  ptr += "<input id=\"currentNumberofColors\" name=\"currentNumberofColors\" value=\"" + String(currentNumberofColors) + "\"></div>\n";
  ptr += "<div><label for=\"currentDecay\">Enter decay per tick [0.75 - 0.995]:</label>\n";
  ptr += "<input id=\"currentDecay\" name=\"currentDecay\" value=\"" + String(currentDecay, 3) + "\"></div>\n";
  ptr += "<div><button type=\"submit\">Submit</button></div>\n";
  ptr += "</form>\n";
  
  /* Javascript functions */
  ptr += "<script> function toggleAutoRipple(checkbox)\n";
    ptr += "{if (checkbox.checked){\n";
    // checkbox is checked, turn on automatic ripples
    ptr += "fetch('/FireRippleEnabled/on');}\n";
    ptr += "else{\n";
    // checkbox is unchecked, turn off automatic ripples
    ptr += "fetch('/FireRippleEnabled/off');}\n";
  ptr += "}</script>\n";
  
  ptr += "</body></html>\n";
  /* The HTTP response ends with another blank line */
  ptr += "\n";
  return ptr;
}

/* HANDLER FUNCTIONS */

void handle_PostRequest() {
  if (server.method() == HTTP_POST){
    Serial.println("received new POST request!");

    String message = "POST form contents:\n";
    for (uint8_t i = 0; i < server.args(); i++) { message += "" + server.argName(i) + ": " + server.arg(i) + "\n "; }
    Serial.println(message);

    int NumberofRipples = server.arg(0).toInt();
    short DelayBetweenRipples = server.arg(1).toInt();
    unsigned long RippleLifeSpan = server.arg(2).toInt();
    int NumberofColors = server.arg(3).toInt();
    float Decay = server.arg(4).toFloat();
    
    if(NumberofRipples > 0 && NumberofRipples <= NUMBER_OF_RIPPLES){ /* new value received */
      Serial.print("received new NumberofRipples from POST request: ");
      Serial.print(NumberofRipples);
      Serial.print(". Previous value: ");
      Serial.println(currentNumberofRipples);
      currentNumberofRipples = NumberofRipples;
    } else {
      Serial.println("new NumberofRipples not valid; discarded.");
    }
    
    if(DelayBetweenRipples >= 5 && DelayBetweenRipples <= 20000){ /* new value received */
      Serial.print("received new DelayBetweenRipples from POST request: ");
      Serial.print(DelayBetweenRipples);
      Serial.print(". Previous value: ");
      Serial.println(currentDelayBetweenRipples);
      currentDelayBetweenRipples = DelayBetweenRipples;
    } else {
      Serial.println("new DelayBetweenRipples not valid; discarded.");
    }
    
    if(RippleLifeSpan >= 500 && RippleLifeSpan <= 20000){ /* new value received */
      Serial.print("received new RippleLifeSpan from POST request: ");
      Serial.print(RippleLifeSpan);
      Serial.print(". Previous value: ");
      Serial.println(currentRippleLifeSpan);
      currentRippleLifeSpan = RippleLifeSpan;
    } else {
      Serial.println("new RippleLifeSpan not valid; discarded.");
    }

    if(NumberofColors >= 3 && NumberofColors <= 25){ /* new value received */
      Serial.print("received new NumberofColors from POST request: ");
      Serial.print(NumberofColors);
      Serial.print(". Previous value: ");
      Serial.println(currentNumberofColors);
      currentNumberofColors = NumberofColors;
    } else {
      Serial.println("new NumberofColors not valid; discarded.");
    }

    if(Decay >= 0.75 && Decay <= 0.996){ /* new value received */
      Serial.print("received new Decay factor from POST request: ");
      Serial.print(String(Decay, 3));
      Serial.print(". Previous value: ");
      Serial.println(String(currentDecay, 3));
      currentDecay = Decay;
    } else {
      Serial.println("new Decay not valid; discarded.");
    }
    
    server.send(500, "text/html", SendHTML());
    server.send(200, "application/json", "{}");
  }
}

void handle_OnConnect() {
  Serial.println("New client connected!");
  server.send(500, "text/html", SendHTML());
}

void handle_ManualRipple() {
  Serial.println("Received manual ripple request");
  manualFireRipple = 1;
  server.send(500, "text/html", SendHTML());
}

void handle_FireRippleEnabled_On() {
  Serial.println("Automatic ripples: ON");
  loopFireRippleEnabled = 1;
}

void handle_FireRippleEnabled_Off() {
  Serial.println("Automatic ripples: OFF");
  loopFireRippleEnabled = 0;
}

/* to be called periodically inside loop() */
void WiFi_MainFunction(void){
  server.handleClient();
}

/* to be called once at startup */
void WiFi_init(void){
  /* Setup WiFi network */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(1000);
    ESP.restart();
  }
  
  /* Setup REST API Handlers */
  server.on("/", handle_OnConnect);
  server.on("/ManualRipple", handle_ManualRipple);
  server.on("/FireRippleEnabled/on", handle_FireRippleEnabled_On);
  server.on("/FireRippleEnabled/off", handle_FireRippleEnabled_Off);
  server.on("/updateInternalVariables", HTTP_POST, handle_PostRequest); 
  
  /* Begin Server */
  server.begin();
  Serial.print("WiFi connected! IP = ");
  Serial.println(WiFi.localIP());
}
