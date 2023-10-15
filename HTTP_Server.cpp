#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <string>  
using namespace std;

#include "HTTP_Server.h" 
#include "ripple.h"
#include "EEP.h"


// WiFi stuff - CHANGE FOR YOUR OWN NETWORK!
const char* ssid = "TP-Link-150";
const char* password = "Cenote#150";

const IPAddress ip(192, 168, 0, 241);  // IP address that THIS DEVICE should request
const IPAddress gateway(192, 168, 0, 1);  // Your router
const IPAddress subnet(255, 255, 255, 0);  // Your subnet mask (find it from your router's admin panel)

#define server hueBridge.webServer //Open port number 80 (HTTP)

unsigned long currentTime = 0;
unsigned long previousTime = 0;
const long timeout = 2000;

// Auxiliar variables to store the current output state
GlobalParameters_struct GlobalParameters = { 
  .loop_MasterFireRippleEnabled = 1,
  .loop_CenterFireRippleEnabled = 0,
  .loop_CubeFireRippleEnabled = 0,
  .loop_QuadFireRippleEnabled = 0,
  .loop_BorderFireRippleEnabled = 0,
  .loop_RandomEffectEnabled = 1,
  .currentNumberofRipples = NUMBER_OF_RIPPLES,
  .currentNumberofColors = 7,
  .currentBehavior = feisty,
  .currentDirection = ALL_DIRECTIONS,
  .currentDelayBetweenRipples = 3000, /* in milliseconds */
  .currentRainbowDeltaPerTick = 200, /* units: hue */
  .currentRippleLifeSpan = 3000, /* in milliseconds */
  .currentRippleSpeed = 0.5, 
  .currentDecay = 0.985,  // Multiply all LED's by this amount each tick to create fancy fading tails 0.972 good value for rainbow
};
boolean manualFireRipple = 0;
unsigned int currentProfile = 0;

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
  ptr += ".InactiveProfile { background-color: gray; font-size: 30px; color: white; padding: 25px 25px; margin: 5px; border: none; cursor: pointer;}\n";
  ptr += ".ActiveProfile { background-color: #4CAF50; font-size: 30px; color: white; padding: 25px 25px; margin: 5px; border: none; cursor: pointer;}\n";
  ptr += ".SelectedProfile { background-color: #green; font-size: 30px; color: white; padding: 30px 30px; margin: 5px; border: none; cursor: pointer;}\n";
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
  
  /****************** BEGINNING OF CHECKBOXES ******************/
  ptr += "<section id=\"checkboxes\"\n>";
  /* Display ON/OFF checkbox for loop_MasterFireRippleEnabled */
  if (GlobalParameters.loop_MasterFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"master-auto-ripple-checkbox\" name=\"master-auto-ripple\" value=\"1\" onchange=\"toggleMasterAutoRipple(this)\" checked><label for=\"master-auto-ripple-checkbox\">Master Enable Automatic Ripples</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"master-auto-ripple-checkbox\" name=\"master-auto-ripple\" value=\"1\" onchange=\"toggleMasterAutoRipple(this)\"><label for=\"master-auto-ripple-checkbox\">Master Enable Automatic Ripples</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_CenterFireRippleEnabled */
  if (GlobalParameters.loop_CenterFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"center-auto-ripple-checkbox\" name=\"center-auto-ripple\" value=\"1\" onchange=\"toggleCenterAutoRipple(this)\" checked><label for=\"center-auto-ripple-checkbox\">Enable Ripples in Center Node</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"center-auto-ripple-checkbox\" name=\"center-auto-ripple\" value=\"1\" onchange=\"toggleCenterAutoRipple(this)\"><label for=\"center-auto-ripple-checkbox\">Enable Ripples in Center Node</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_CubeFireRippleEnabled */
  if (GlobalParameters.loop_CubeFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"cube-auto-ripple-checkbox\" name=\"cube-auto-ripple\" value=\"1\" onchange=\"toggleCubeAutoRipple(this)\" checked><label for=\"cube-auto-ripple-checkbox\">Enable Ripples in Cube Nodes</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"cube-auto-ripple-checkbox\" name=\"cube-auto-ripple\" value=\"1\" onchange=\"toggleCubeAutoRipple(this)\"><label for=\"cube-auto-ripple-checkbox\">Enable Ripples in Cube Nodes</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_QuadFireRippleEnabled */
  if (GlobalParameters.loop_QuadFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"quad-auto-ripple-checkbox\" name=\"quad-auto-ripple\" value=\"1\" onchange=\"toggleQuadAutoRipple(this)\" checked><label for=\"quad-auto-ripple-checkbox\">Enable Ripples in Quad Nodes</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"quad-auto-ripple-checkbox\" name=\"quad-auto-ripple\" value=\"1\" onchange=\"toggleQuadAutoRipple(this)\"><label for=\"quad-auto-ripple-checkbox\">Enable Ripples in Quad Nodes</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_BorderFireRippleEnabled */
  if (GlobalParameters.loop_BorderFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"border-auto-ripple-checkbox\" name=\"border-auto-ripple\" value=\"1\" onchange=\"toggleBorderAutoRipple(this)\" checked><label for=\"border-auto-ripple-checkbox\">Enable Ripples in Border Nodes</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"border-auto-ripple-checkbox\" name=\"border-auto-ripple\" value=\"1\" onchange=\"toggleBorderAutoRipple(this)\"><label for=\"border-auto-ripple-checkbox\">Enable Ripples in Border Nodes</label></p>\n";
  }
  /* Display ON/OFF checkbox for loop_RandomEffectEnabled */
  if (GlobalParameters.loop_RandomEffectEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"Random-Effect-checkbox\" name=\"Random-ripple\" value=\"1\" onchange=\"toggleRandomEffect(this)\" checked><label for=\"Random-Effect-checkbox\">Enable Random effect</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"Random-Effect-checkbox\" name=\"Random-ripple\" value=\"1\" onchange=\"toggleRandomEffect(this)\"><label for=\"Random-Effect-checkbox\">Enable Random effect</label></p>\n";
  }
  ptr += "</section\n>";
  /****************** END OF CHECKBOXES ******************/
  

  /****************** BEGINNING OF USER INPUT ******************/
  ptr += "<section id=\"userinput\"\n>";
  /* Text input for number of ripples */
  ptr += "<form action=\"/updateInternalVariables\" method=\"post\">\n";
  ptr += "<div><label for=\"NumberofRipples\">Enter number of ripples [1 - 99]:</label>\n";
  ptr += "<input id=\"NumberofRipples\" name=\"NumberofRipples\" value=\"" + String((int) GlobalParameters.currentNumberofRipples) + "\"></div>\n";
  ptr += "<div><label for=\"currentDelayBetweenRipples\">Enter delay between ripples in ms [1 - 20000]:</label>\n";
  ptr += "<input id=\"currentDelayBetweenRipples\" name=\"currentDelayBetweenRipples\" value=\"" + String(GlobalParameters.currentDelayBetweenRipples) + "\"></div>\n";
  ptr += "<div><label for=\"currentRainbowDeltaPerTick\">Enter rainbow delta per tick in hue [1 - 20000]:</label>\n";
  ptr += "<input id=\"currentRainbowDeltaPerTick\" name=\"currentRainbowDeltaPerTick\" value=\"" + String(GlobalParameters.currentRainbowDeltaPerTick) + "\"></div>\n";
  ptr += "<div><label for=\"currentRippleLifeSpan\">Enter ripple life span in ms [1 - 20000]:</label>\n";
  ptr += "<input id=\"currentRippleLifeSpan\" name=\"currentRippleLifeSpan\" value=\"" + String(GlobalParameters.currentRippleLifeSpan) + "\"></div>\n";
  ptr += "<div><label for=\"currentRippleSpeed\">Enter ripple speed (float) [0.01 - 10]:</label>\n";
  ptr += "<input id=\"currentRippleSpeed\" name=\"currentRippleSpeed\" value=\"" + String(GlobalParameters.currentRippleSpeed, 2) + "\"></div>\n";
  ptr += "<div><label for=\"currentNumberofColors\">Enter desired number of colors [1 - 25]:</label>\n";
  ptr += "<input id=\"currentNumberofColors\" name=\"currentNumberofColors\" value=\"" + String((int) GlobalParameters.currentNumberofColors) + "\"></div>\n";
  ptr += "<div><label for=\"currentBehavior\">Enter desired behavior [0-2 = less to more aggro; 3 = alwaysRight; 4 = alwaysLeft]:</label>\n";
  ptr += "<input id=\"currentBehavior\" name=\"currentBehavior\" value=\"" + String((int) GlobalParameters.currentBehavior) + "\"></div>\n";
  ptr += "<div><label for=\"currentDirection\">Enter ripple direction [-1 = All directions; 0-5 = direction clockwise starting at 12:00; 6 = random direction]:</label>\n";
  ptr += "<input id=\"currentDirection\" name=\"currentDirection\" value=\"" + String(GlobalParameters.currentDirection) + "\"></div>\n";
  ptr += "<div><label for=\"currentDecay\">Enter decay per tick [0.5 - 1]:</label>\n";
  ptr += "<input id=\"currentDecay\" name=\"currentDecay\" value=\"" + String(GlobalParameters.currentDecay, 3) + "\"></div>\n";
  ptr += "<div>\n"; /*begin buttons */
  ptr += "<button type=\"submit\">Submit</button>\n";
  ptr += "</div>\n"; /* end buttons */
  ptr += "</form>\n"; 
  ptr += "</section\n>";
  /****************** END OF USER INPUT ******************/


  /****************** BEGINNING OF PROFILE MANAGEMENT ******************/
  ptr += "<section id=\"userinput\"\n>";
  ptr += "<h1> Profiles </h1\n>";
    ptr += "<p> Current profile: " + String(currentProfile) + "</p\n>";
    ptr += "<div\n>";
      ptr += "<div id=\"profiles\"\n>";
        /* button 1*/
        ptr += "<a href=\"http://192.168.0.241/RestoreProfile_1\"><button ";
        if( (currentProfile == 0) && (ProfilesAvailable[0] == 1U)) ptr += "class=\"SelectedProfile\"";
        else if (ProfilesAvailable[0] == 1U) ptr += "class=\"ActiveProfile\"";
        else ptr += "class=\"InactiveProfile\"";
        ptr += ">#1</button></a>\n";
        /* button 2*/
        ptr += "<a href=\"http://192.168.0.241/RestoreProfile_2\"><button ";
        if( (currentProfile == 1) && (ProfilesAvailable[1] == 1U)) ptr += "class=\"SelectedProfile\"";
        else if (ProfilesAvailable[1] == 1U) ptr += "class=\"ActiveProfile\"";
        else ptr += "class=\"InactiveProfile\"";
        ptr += ">#2</button></a>\n";
        /* button 3*/
        ptr += "<a href=\"http://192.168.0.241/RestoreProfile_3\"><button ";
        if( (currentProfile == 2) && (ProfilesAvailable[2] == 1U)) ptr += "class=\"SelectedProfile\"";
        else if (ProfilesAvailable[2] == 1U) ptr += "class=\"ActiveProfile\"";
        else ptr += "class=\"InactiveProfile\"";
        ptr += ">#3</button></a>\n";
        /* button 4*/
        ptr += "<a href=\"http://192.168.0.241/RestoreProfile_4\"><button ";
        if( (currentProfile == 3) && (ProfilesAvailable[3] == 1U)) ptr += "class=\"SelectedProfile\"";
        else if (ProfilesAvailable[3] == 1U) ptr += "class=\"ActiveProfile\"";
        else ptr += "class=\"InactiveProfile\"";
        ptr += ">#4</button></a>\n";
        /* button 5*/
        ptr += "<a href=\"http://192.168.0.241/RestoreProfile_5\"><button ";
        if( (currentProfile == 4) && (ProfilesAvailable[4] == 1U)) ptr += "class=\"SelectedProfile\"";
        else if (ProfilesAvailable[4] == 1U) ptr += "class=\"ActiveProfile\"";
        else ptr += "class=\"InactiveProfile\"";
        ptr += ">#5</button></a>\n";
      ptr += "</div\n>";
      ptr += "<div\n>";
        ptr += "<a href=\"http://192.168.0.241/StoreProfile\"><button class=\"button\">Store Profile</button></a\n>";
        ptr += "<a href=\"http://192.168.0.241/DeleteProfile\"><button class=\"button\">Delete Profile</button></a\n>";
      ptr += "</div\n>";
    ptr += "</div\n>";
  ptr += "</section\n>";
  /****************** END OF PROFILE MANAGEMENT ******************/
  ptr += "<p><a href=\"/SWreset\"><button class=\"button\">Software reset</button></a></p>\n";
  
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

   /* for Random effect Enable */
  ptr += "<script> function toggleRandomEffect(checkbox)\n";
    ptr += "{if (checkbox.checked){\n";
    // checkbox is checked, turn on automatic ripples
    ptr += "fetch('/RandomEffectEnabled/on');}\n";
    ptr += "else{\n";
    // checkbox is unchecked, turn off automatic ripples
    ptr += "fetch('/RandomEffectEnabled/off');}\n";
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
    short RainbowDeltaPerTick = server.arg(2).toInt();
    unsigned long RippleLifeSpan = server.arg(3).toInt();
    float RippleSpeed = server.arg(4).toFloat();
    int NumberofColors = server.arg(5).toInt();
    int Behavior = server.arg(6).toInt();
    int Direction = server.arg(7).toInt();
    float Decay = server.arg(8).toFloat();
    
    if(NumberofRipples > 0 && NumberofRipples <= NUMBER_OF_RIPPLES){ /* new value received */
      Serial.print("received new NumberofRipples from POST request: ");
      Serial.print(NumberofRipples);
      Serial.print(". Previous value: ");
      Serial.println(GlobalParameters.currentNumberofRipples);
      GlobalParameters.currentNumberofRipples = NumberofRipples;
    } else {
      Serial.println("new NumberofRipples not valid; discarded.");
    }
    
    if(DelayBetweenRipples >= 1 && DelayBetweenRipples <= 20000){ /* new value received */
      Serial.print("received new DelayBetweenRipples from POST request: ");
      Serial.print(DelayBetweenRipples);
      Serial.print(". Previous value: ");
      Serial.println(GlobalParameters.currentDelayBetweenRipples);
      GlobalParameters.currentDelayBetweenRipples = DelayBetweenRipples;
    } else {
      Serial.println("new DelayBetweenRipples not valid; discarded.");
    }

    if(RainbowDeltaPerTick >= 1 && RainbowDeltaPerTick <= 20000){ /* new value received */
      Serial.print("received new RainbowDeltaPerTick from POST request: ");
      Serial.print(RainbowDeltaPerTick);
      Serial.print(". Previous value: ");
      Serial.println(GlobalParameters.currentRainbowDeltaPerTick);
      GlobalParameters.currentRainbowDeltaPerTick = RainbowDeltaPerTick;
    } else {
      Serial.println("new RainbowDeltaPerTick not valid; discarded.");
    }
    
    if(RippleLifeSpan >= 1 && RippleLifeSpan <= 20000){ /* new value received */
      Serial.print("received new RippleLifeSpan from POST request: ");
      Serial.print(RippleLifeSpan);
      Serial.print(". Previous value: ");
      Serial.println(GlobalParameters.currentRippleLifeSpan);
      GlobalParameters.currentRippleLifeSpan = RippleLifeSpan;
    } else {
      Serial.println("new RippleLifeSpan not valid; discarded.");
    }

    if(RippleSpeed >= 0.01 && RippleSpeed <= 10){ /* new value received */
      Serial.print("received new Ripple Speed from POST request: ");
      Serial.print(String(RippleSpeed, 2));
      Serial.print(". Previous value: ");
      Serial.println(String(GlobalParameters.currentRippleSpeed, 2));
      GlobalParameters.currentRippleSpeed = RippleSpeed;
    } else {
      Serial.println("new Ripple Speed not valid; discarded.");
    }

    if(NumberofColors >= 1 && NumberofColors <= 25){ /* new value received */
      Serial.print("received new NumberofColors from POST request: ");
      Serial.print(NumberofColors);
      Serial.print(". Previous value: ");
      Serial.println(GlobalParameters.currentNumberofColors);
      GlobalParameters.currentNumberofColors = NumberofColors;
    } else {
      Serial.println("new NumberofColors not valid; discarded.");
    }

    if(Behavior >= 0 && Behavior <= 4){ /* new value received */
      Serial.print("received new Behavior from POST request: ");
      Serial.print(Behavior);
      Serial.print(". Previous value: ");
      Serial.println(GlobalParameters.currentBehavior);
      GlobalParameters.currentBehavior = Behavior;
    } else {
      Serial.println("new Behavior not valid; discarded.");
    }

    if(Direction >= -1 && Direction <= 6){ /* new value received */
      Serial.print("received new Direction from POST request: ");
      Serial.print(Direction);
      Serial.print(". Previous value: ");
      Serial.println(GlobalParameters.currentDirection);
      GlobalParameters.currentDirection = Direction;
    } else {
      Serial.println("new Direction not valid; discarded.");
    }

    if(Decay >= 0.5 && Decay <= 1){ /* new value received */
      Serial.print("received new Decay factor from POST request: ");
      Serial.print(String(Decay, 3));
      Serial.print(". Previous value: ");
      Serial.println(String(GlobalParameters.currentDecay, 3));
      GlobalParameters.currentDecay = Decay;
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

void handle_RestoreProfile_1() {
  boolean EEP_return = 0U;
  currentProfile = 0;
  EEP_return = EEPROM_RestoreProfile(currentProfile);
  if(EEP_return == 1U){ 
    /* profile found */
  } else{

  }
  server.send(500, "text/html", SendHTML());
}

void handle_RestoreProfile_2() {
  boolean EEP_return = 0U;
  currentProfile = 1;
  EEP_return = EEPROM_RestoreProfile(currentProfile);
  if(EEP_return == 1U){ 
    /* profile found */
  } else{

  }
  server.send(500, "text/html", SendHTML());
}

void handle_RestoreProfile_3() {
  boolean EEP_return = 0U;
  currentProfile = 2;
  EEP_return = EEPROM_RestoreProfile(currentProfile);
  if(EEP_return == 1U){ 
    /* profile found */
  } else{

  }
  server.send(500, "text/html", SendHTML());
}

void handle_RestoreProfile_4() {
  boolean EEP_return = 0U;
  currentProfile = 3;
  EEP_return = EEPROM_RestoreProfile(currentProfile);
  if(EEP_return == 1U){ 
    /* profile found */
  } else{

  }
  server.send(500, "text/html", SendHTML());
}

void handle_RestoreProfile_5() {
  boolean EEP_return = 0U;
  currentProfile = 4;
  EEP_return = EEPROM_RestoreProfile(currentProfile);
  if(EEP_return == 1U){ 
    /* profile found */
  } else{

  }
  server.send(500, "text/html", SendHTML());
}

void handle_StoreProfile() {
  EEPROM_StoreProfile(0U);
  server.send(500, "text/html", SendHTML());
}

void handle_DeleteProfile() {
  //EEPROM_RestoreProfile(0U);
  server.send(500, "text/html", SendHTML());
}

void handle_SWreset() {
  ESP.restart();
}

void handle_MasterFireRippleEnabled_On() {
  Serial.println("Master Automatic ripples: ON");
  GlobalParameters.loop_MasterFireRippleEnabled = 1;
}

void handle_MasterFireRippleEnabled_Off() {
  Serial.println("Master Automatic ripples: OFF");
  GlobalParameters.loop_MasterFireRippleEnabled = 0;
}

void handle_CenterFireRippleEnabled_On() {
  Serial.println("Center Automatic ripples: ON");
  GlobalParameters.loop_CenterFireRippleEnabled = 1;
}

void handle_CenterFireRippleEnabled_Off() {
  Serial.println("Center Automatic ripples: OFF");
  GlobalParameters.loop_CenterFireRippleEnabled = 0;
}

void handle_CubeFireRippleEnabled_On() {
  Serial.println("Cube Automatic ripples: ON");
  GlobalParameters.loop_CubeFireRippleEnabled = 1;
}

void handle_CubeFireRippleEnabled_Off() {
  Serial.println("Cube Automatic ripples: OFF");
  GlobalParameters.loop_CubeFireRippleEnabled = 0;
}

void handle_QuadFireRippleEnabled_On() {
  Serial.println("Quad Automatic ripples: ON");
  GlobalParameters.loop_QuadFireRippleEnabled = 1;
}

void handle_QuadFireRippleEnabled_Off() {
  Serial.println("Quad Automatic ripples: OFF");
  GlobalParameters.loop_QuadFireRippleEnabled = 0;
}

void handle_BorderFireRippleEnabled_On() {
  Serial.println("Border Automatic ripples: ON");
  GlobalParameters.loop_BorderFireRippleEnabled = 1;
}

void handle_BorderFireRippleEnabled_Off() {
  Serial.println("Border Automatic ripples: OFF");
  GlobalParameters.loop_BorderFireRippleEnabled = 0;
}

void handle_RandomEffectEnabled_On() {
  Serial.println("Border Automatic ripples: ON");
  GlobalParameters.loop_RandomEffectEnabled = 1;
}

void handle_RandomEffectEnabled_Off() {
  Serial.println("Border Automatic ripples: OFF");
  GlobalParameters.loop_RandomEffectEnabled = 0;
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
  server.on("/dashboard", handle_OnConnect);
  server.on("/ManualRipple", handle_ManualRipple);
  server.on("/RestoreProfile_1", handle_RestoreProfile_1);
  server.on("/RestoreProfile_2", handle_RestoreProfile_2);
  server.on("/RestoreProfile_3", handle_RestoreProfile_3);
  server.on("/RestoreProfile_4", handle_RestoreProfile_4);
  server.on("/RestoreProfile_5", handle_RestoreProfile_5);
  server.on("/StoreProfile", handle_StoreProfile);
  server.on("/DeleteProfile", handle_DeleteProfile);
  server.on("/SWreset", handle_SWreset);
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
  server.on("/RandomEffectEnabled/off", handle_RandomEffectEnabled_Off);
  server.on("/RandomEffectEnabled/on", handle_RandomEffectEnabled_On);
  server.on("/updateInternalVariables", HTTP_POST, handle_PostRequest); 
  
  /* server already begun by hueBrdige */
  //server.begin();
  Serial.printf("Wifi connected, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  //Serial.println(WiFi.localIP());

  // Setup Multicast DNS https://en.wikipedia.org/wiki/Multicast_DNS 
  // You can open http://hexagono.local in Chrome on a desktop
  Serial.println("Setup MDNS for http://hexagono.local");
  if (!MDNS.begin("hexagono"))
  {
    Serial.println("Error setting up MDNS responder!");
  }
}
