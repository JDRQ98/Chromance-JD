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
int loop_MasterFireRippleEnabled = 1;
int loop_CenterFireRippleEnabled = 1;
int loop_CubeFireRippleEnabled = 0;
int loop_QuadFireRippleEnabled = 0;
int loop_BorderFireRippleEnabled = 0;
int manualFireRipple = 0;
int currentNumberofRipples = NUMBER_OF_RIPPLES;
int currentNumberofColors = 7;
int currentBehavior = feisty;
short currentDelayBetweenRipples = 3000; /* in milliseconds */
unsigned long currentRippleLifeSpan = 3000; /* in milliseconds */
float currentRippleSpeed = 0.25; 
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
  
  /* Display ON/OFF checkbox for loop_MasterFireRippleEnabled */
  if (loop_MasterFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"master-auto-ripple-checkbox\" name=\"master-auto-ripple\" value=\"1\" onchange=\"toggleMasterAutoRipple(this)\" checked><label for=\"master-auto-ripple-checkbox\">Master Enable Automatic Ripples</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"master-auto-ripple-checkbox\" name=\"master-auto-ripple\" value=\"1\" onchange=\"toggleMasterAutoRipple(this)\"><label for=\"master-auto-ripple-checkbox\">Master Enable Automatic Ripples</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_CenterFireRippleEnabled */
  if (loop_CenterFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"center-auto-ripple-checkbox\" name=\"center-auto-ripple\" value=\"1\" onchange=\"toggleCenterAutoRipple(this)\" checked><label for=\"center-auto-ripple-checkbox\">Enable Ripples in Center Node</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"center-auto-ripple-checkbox\" name=\"center-auto-ripple\" value=\"1\" onchange=\"toggleCenterAutoRipple(this)\"><label for=\"center-auto-ripple-checkbox\">Enable Ripples in Center Node</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_CubeFireRippleEnabled */
  if (loop_CubeFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"cube-auto-ripple-checkbox\" name=\"cube-auto-ripple\" value=\"1\" onchange=\"toggleCubeAutoRipple(this)\" checked><label for=\"cube-auto-ripple-checkbox\">Enable Ripples in Cube Nodes</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"cube-auto-ripple-checkbox\" name=\"cube-auto-ripple\" value=\"1\" onchange=\"toggleCubeAutoRipple(this)\"><label for=\"cube-auto-ripple-checkbox\">Enable Ripples in Cube Nodes</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_QuadFireRippleEnabled */
  if (loop_QuadFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"quad-auto-ripple-checkbox\" name=\"quad-auto-ripple\" value=\"1\" onchange=\"toggleQuadAutoRipple(this)\" checked><label for=\"quad-auto-ripple-checkbox\">Enable Ripples in Quad Nodes</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"quad-auto-ripple-checkbox\" name=\"quad-auto-ripple\" value=\"1\" onchange=\"toggleQuadAutoRipple(this)\"><label for=\"quad-auto-ripple-checkbox\">Enable Ripples in Quad Nodes</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_BorderFireRippleEnabled */
  if (loop_BorderFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"border-auto-ripple-checkbox\" name=\"border-auto-ripple\" value=\"1\" onchange=\"toggleBorderAutoRipple(this)\" checked><label for=\"border-auto-ripple-checkbox\">Enable Ripples in Border Nodes</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"border-auto-ripple-checkbox\" name=\"border-auto-ripple\" value=\"1\" onchange=\"toggleBorderAutoRipple(this)\"><label for=\"border-auto-ripple-checkbox\">Enable Ripples in Border Nodes</label></p>\n";
  }
  
  /* Text input for number of ripples */
  ptr += "<form action=\"/updateInternalVariables\" method=\"post\">\n";
  ptr += "<div><label for=\"NumberofRipples\">Enter number of ripples [1 - 99]:</label>\n";
  ptr += "<input id=\"NumberofRipples\" name=\"NumberofRipples\" value=\"" + String(currentNumberofRipples) + "\"></div>\n";
  ptr += "<div><label for=\"currentDelayBetweenRipples\">Enter delay between ripples in ms [1 - 20000]:</label>\n";
  ptr += "<input id=\"currentDelayBetweenRipples\" name=\"currentDelayBetweenRipples\" value=\"" + String(currentDelayBetweenRipples) + "\"></div>\n";
  ptr += "<div><label for=\"currentRippleLifeSpan\">Enter ripple life span in ms [1 - 20000]:</label>\n";
  ptr += "<input id=\"currentRippleLifeSpan\" name=\"currentRippleLifeSpan\" value=\"" + String(currentRippleLifeSpan) + "\"></div>\n";
  ptr += "<div><label for=\"currentRippleSpeed\">Enter ripple speed (float) [0.01 - 10]:</label>\n";
  ptr += "<input id=\"currentRippleSpeed\" name=\"currentRippleSpeed\" value=\"" + String(currentRippleSpeed, 2) + "\"></div>\n";
  ptr += "<div><label for=\"currentNumberofColors\">Enter desired number of colors [3 - 25]:</label>\n";
  ptr += "<input id=\"currentNumberofColors\" name=\"currentNumberofColors\" value=\"" + String(currentNumberofColors) + "\"></div>\n";
  ptr += "<div><label for=\"currentBehavior\">Enter desired behavior [0-2 = less to more aggro; 3 = alwaysRight; 4 = alwaysLeft]:</label>\n";
  ptr += "<input id=\"currentBehavior\" name=\"currentBehavior\" value=\"" + String(currentBehavior) + "\"></div>\n";
  ptr += "<div><label for=\"currentDecay\">Enter decay per tick [0.5 - 0.995]:</label>\n";
  ptr += "<input id=\"currentDecay\" name=\"currentDecay\" value=\"" + String(currentDecay, 3) + "\"></div>\n";
  ptr += "<div><button type=\"submit\">Submit</button></div>\n";
  ptr += "</form>\n";
  
  /* Javascript functions */
  /* for Master Enable */
  ptr += "<script> function toggleMasterAutoRipple(checkbox)\n";
    ptr += "{if (checkbox.checked){\n";
    // checkbox is checked, turn on automatic ripples
    ptr += "fetch('/MasterFireRippleEnabled/on');}\n";
    ptr += "else{\n";
    // checkbox is unchecked, turn off automatic ripples
    ptr += "fetch('/MasterFireRippleEnabled/off');}\n";
  ptr += "}</script>\n";

  /* for Center Enable */
  ptr += "<script> function toggleCenterAutoRipple(checkbox)\n";
    ptr += "{if (checkbox.checked){\n";
    // checkbox is checked, turn on automatic ripples
    ptr += "fetch('/CenterFireRippleEnabled/on');}\n";
    ptr += "else{\n";
    // checkbox is unchecked, turn off automatic ripples
    ptr += "fetch('/CenterFireRippleEnabled/off');}\n";
  ptr += "}</script>\n";

  /* for Cube Enable */
  ptr += "<script> function toggleCubeAutoRipple(checkbox)\n";
    ptr += "{if (checkbox.checked){\n";
    // checkbox is checked, turn on automatic ripples
    ptr += "fetch('/CubeFireRippleEnabled/on');}\n";
    ptr += "else{\n";
    // checkbox is unchecked, turn off automatic ripples
    ptr += "fetch('/CubeFireRippleEnabled/off');}\n";
  ptr += "}</script>\n";

  /* for Quad Enable */
  ptr += "<script> function toggleQuadAutoRipple(checkbox)\n";
    ptr += "{if (checkbox.checked){\n";
    // checkbox is checked, turn on automatic ripples
    ptr += "fetch('/QuadFireRippleEnabled/on');}\n";
    ptr += "else{\n";
    // checkbox is unchecked, turn off automatic ripples
    ptr += "fetch('/QuadFireRippleEnabled/off');}\n";
  ptr += "}</script>\n";

  /* for Border Enable */
  ptr += "<script> function toggleBorderAutoRipple(checkbox)\n";
    ptr += "{if (checkbox.checked){\n";
    // checkbox is checked, turn on automatic ripples
    ptr += "fetch('/BorderFireRippleEnabled/on');}\n";
    ptr += "else{\n";
    // checkbox is unchecked, turn off automatic ripples
    ptr += "fetch('/BorderFireRippleEnabled/off');}\n";
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
    float RippleSpeed = server.arg(3).toFloat();
    int NumberofColors = server.arg(4).toInt();
    int Behavior = server.arg(5).toInt();
    float Decay = server.arg(6).toFloat();
    
    if(NumberofRipples > 0 && NumberofRipples <= NUMBER_OF_RIPPLES){ /* new value received */
      Serial.print("received new NumberofRipples from POST request: ");
      Serial.print(NumberofRipples);
      Serial.print(". Previous value: ");
      Serial.println(currentNumberofRipples);
      currentNumberofRipples = NumberofRipples;
    } else {
      Serial.println("new NumberofRipples not valid; discarded.");
    }
    
    if(DelayBetweenRipples >= 1 && DelayBetweenRipples <= 20000){ /* new value received */
      Serial.print("received new DelayBetweenRipples from POST request: ");
      Serial.print(DelayBetweenRipples);
      Serial.print(". Previous value: ");
      Serial.println(currentDelayBetweenRipples);
      currentDelayBetweenRipples = DelayBetweenRipples;
    } else {
      Serial.println("new DelayBetweenRipples not valid; discarded.");
    }
    
    if(RippleLifeSpan >= 1 && RippleLifeSpan <= 20000){ /* new value received */
      Serial.print("received new RippleLifeSpan from POST request: ");
      Serial.print(RippleLifeSpan);
      Serial.print(". Previous value: ");
      Serial.println(currentRippleLifeSpan);
      currentRippleLifeSpan = RippleLifeSpan;
    } else {
      Serial.println("new RippleLifeSpan not valid; discarded.");
    }

    if(RippleSpeed >= 0.01 && RippleSpeed <= 10){ /* new value received */
      Serial.print("received new Ripple Speed from POST request: ");
      Serial.print(String(RippleSpeed, 2));
      Serial.print(". Previous value: ");
      Serial.println(String(currentRippleSpeed, 2));
      currentRippleSpeed = RippleSpeed;
    } else {
      Serial.println("new Ripple Speed not valid; discarded.");
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

    if(Behavior >= 0 && Behavior <= 4){ /* new value received */
      Serial.print("received new Behavior from POST request: ");
      Serial.print(Behavior);
      Serial.print(". Previous value: ");
      Serial.println(currentBehavior);
      currentBehavior = Behavior;
    } else {
      Serial.println("new Behavior not valid; discarded.");
    }

    if(Decay >= 0.5 && Decay <= 0.996){ /* new value received */
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

void handle_MasterFireRippleEnabled_On() {
  Serial.println("Master Automatic ripples: ON");
  loop_MasterFireRippleEnabled = 1;
}

void handle_MasterFireRippleEnabled_Off() {
  Serial.println("Master Automatic ripples: OFF");
  loop_MasterFireRippleEnabled = 0;
}

void handle_CenterFireRippleEnabled_On() {
  Serial.println("Center Automatic ripples: ON");
  loop_CenterFireRippleEnabled = 1;
}

void handle_CenterFireRippleEnabled_Off() {
  Serial.println("Center Automatic ripples: OFF");
  loop_CenterFireRippleEnabled = 0;
}

void handle_CubeFireRippleEnabled_On() {
  Serial.println("Cube Automatic ripples: ON");
  loop_CubeFireRippleEnabled = 1;
}

void handle_CubeFireRippleEnabled_Off() {
  Serial.println("Cube Automatic ripples: OFF");
  loop_CubeFireRippleEnabled = 0;
}

void handle_QuadFireRippleEnabled_On() {
  Serial.println("Quad Automatic ripples: ON");
  loop_QuadFireRippleEnabled = 1;
}

void handle_QuadFireRippleEnabled_Off() {
  Serial.println("Quad Automatic ripples: OFF");
  loop_QuadFireRippleEnabled = 0;
}

void handle_BorderFireRippleEnabled_On() {
  Serial.println("Border Automatic ripples: ON");
  loop_BorderFireRippleEnabled = 1;
}

void handle_BorderFireRippleEnabled_Off() {
  Serial.println("Border Automatic ripples: OFF");
  loop_BorderFireRippleEnabled = 0;
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
  server.on("/MasterFireRippleEnabled/on", handle_MasterFireRippleEnabled_On);
  server.on("/MasterFireRippleEnabled/off", handle_MasterFireRippleEnabled_Off);
  server.on("/CenterFireRippleEnabled/off", handle_CenterFireRippleEnabled_Off);
  server.on("/CenterFireRippleEnabled/on", handle_CenterFireRippleEnabled_On);
  server.on("/CubeFireRippleEnabled/off", handle_CubeFireRippleEnabled_Off);
  server.on("/CubeFireRippleEnabled/on", handle_CubeFireRippleEnabled_On);
  server.on("/QuadFireRippleEnabled/off", handle_QuadFireRippleEnabled_Off);
  server.on("/QuadFireRippleEnabled/on", handle_QuadFireRippleEnabled_On);
  server.on("/BorderFireRippleEnabled/off", handle_BorderFireRippleEnabled_Off);
  server.on("/BorderFireRippleEnabled/on", handle_BorderFireRippleEnabled_On);
  server.on("/updateInternalVariables", HTTP_POST, handle_PostRequest); 
  
  /* Begin Server */
  server.begin();
  Serial.print("WiFi connected! IP = ");
  Serial.println(WiFi.localIP());
}
