#include <WiFi.h>
#include <WebServer.h>
#include <string>  
using namespace std;

#include "HTTP_Server.h" 
#include "ripple.h"
#include "EEP.h"
#include "SimpleJson.h"


// WiFi stuff - CHANGE FOR YOUR OWN NETWORK!
const char* ssid = "TP-Link-150";
const char* password = "Cenote#150";

const IPAddress ip(192, 168, 1, 241);  // IP address that THIS DEVICE should request
const IPAddress gateway(192, 168, 1, 250);  // Your router
const IPAddress subnet(255, 255, 255, 0);  // Your subnet mask (find it from your router's admin panel)

#define server hueBridge.webServer //Open port number 80 (HTTP)

unsigned long currentTime = 0;
unsigned long previousTime = 0;
const long timeout = 2000;

// Auxiliar variables to store the current output state
GlobalParameters_struct GlobalParameters = { 
  .loop_MasterFireRippleEnabled = 1,
  .currentStartingNode = HTTP_CURRENTSTARTINGNODE_DEFAULT,
  .currentBehavior = HTTP_CURRENTBEHAVIOR_DEFAULT,
  .currentDirection = HTTP_CURRENTDIRECTION_DEFAULT,
  .currentDelayBetweenRipples = HTTP_CURRENTDELAYBETWEENRIPPLES_DEFAULT, /* in milliseconds */
  .currentRippleLifeSpan = HTTP_CURRENTRIPPLELIFESPAN_DEFAULT, /* in milliseconds */
  .currentRippleSpeed = HTTP_CURRENTRIPPLESPEED_DEFAULT, 
  .currentDecay = HTTP_CURRENTDECAY_DEFAULT,  // Multiply all LED's by this amount each tick to create fancy fading tails 0.972 good value for rainbow
  .currentColor = HTTP_CURRENTCOLOR_DEFAULT,
  .currentRainbowDeltaPerPeriod = HTTP_CURRENTRAINBOWDELTAPERPERIOD_DEFAULT, /* units: hue */
  .currentRainbowDeltaPerTick = HTTP_CURRENTRAINBOWDELTAPERTICK_DEFAULT, /* units: hue */
  .currentNumberofColors = HTTP_CURRENTNUMBEROFCOLORS_DEFAULT
};
boolean manualFireRipple = 0;
unsigned int currentSelectedProfile = 0;
unsigned int currentLoadedProfile = -1;


/* HANDLER FUNCTIONS */
void handle_getInternalVariables(AsyncWebServerRequest *request) {
  String response = "{\n";
  response += "  \"currentStartingNode\": " + String(GlobalParameters.currentStartingNode) + " ,\n";
// Enter the desired behavior:
  response += "  \"currentBehavior\": " + String(GlobalParameters.currentBehavior) + " ,\n";
// Enter ripple direction:
  response += "  \"currentDirection\": " + String(GlobalParameters.currentDirection) + " ,\n";
// Enter delay between ripples in ms [1 - 60000]:
  response += "  \"currentDelayBetweenRipples\": " + String(GlobalParameters.currentDelayBetweenRipples) + " ,\n";
  response += "  \"currentDelayBetweenRipplesMin\": " + String(HTTP_CURRENTDELAYBETWEENRIPPLES_MIN) + " ,\n";
  response += "  \"currentDelayBetweenRipplesMax\": " + String(HTTP_CURRENTDELAYBETWEENRIPPLES_MAX) + " ,\n";
// Enter ripple life span in ms [1 - 60000]:
  response += "  \"currentRippleLifeSpan\": " + String(GlobalParameters.currentRippleLifeSpan) + " ,\n";
  response += "  \"currentRippleLifeSpanMin\": " + String(HTTP_CURRENTRIPPLELIFESPAN_MIN) + " ,\n";
  response += "  \"currentRippleLifeSpanMax\": " + String(HTTP_CURRENTRIPPLELIFESPAN_MAX) + " ,\n";
// Enter ripple speed [0.01 - 5]:
  response += "  \"currentRippleSpeed\": " + String(GlobalParameters.currentRippleSpeed) + " ,\n";
  response += "  \"currentRippleSpeedMin\": " + String(HTTP_CURRENTRIPPLESPEED_MIN) + " ,\n";
  response += "  \"currentRippleSpeedMax\": " + String(HTTP_CURRENTRIPPLESPEED_MAX) + " ,\n";
// Enter decay per tick [0.90 - 1]:
  response += "  \"currentDecay\": " + String(GlobalParameters.currentDecay) + " ,\n";
  response += "  \"currentDecayMin\": " + String(HTTP_CURRENTDECAY_MIN) + " ,\n";
  response += "  \"currentDecayMax\": " + String(HTTP_CURRENTDECAY_MAX) + " ,\n";
// Starting Color:
  response += "  \"currentColor\": " + String(GlobalParameters.currentColor) + " ,\n";
// Enter hue delta per period [0 - 60000]:
  response += "  \"currentRainbowDeltaPerPeriod\": " + String(GlobalParameters.currentRainbowDeltaPerPeriod) + " ,\n";
  response += "  \"currentRainbowDeltaPerPeriodMin\": " + String(HTTP_CURRENTRAINBOWDELTAPERPERIOD_MIN) + " ,\n";
  response += "  \"currentRainbowDeltaPerPeriodMax\": " + String(HTTP_CURRENTRAINBOWDELTAPERPERIOD_MAX) + " ,\n";
// Enter hue delta per tick [0 - 500]:
  response += "  \"currentRainbowDeltaPerTick\": " + String(GlobalParameters.currentRainbowDeltaPerTick) + " ,\n";
  response += "  \"currentRainbowDeltaPerTickMin\": " + String(HTTP_CURRENTRAINBOWDELTAPERTICK_MIN) + " ,\n";
  response += "  \"currentRainbowDeltaPerTickMax\": " + String(HTTP_CURRENTRAINBOWDELTAPERTICK_MAX) + " ,\n";
// Enter the desired number of colors [1 - 64]:
  response += "  \"currentNumberofColors\": " + String(GlobalParameters.currentNumberofColors) + " ,\n";
  response += "  \"currentNumberofColorsMin\": " + String(HTTP_CURRENTNUMBEROFCOLORS_MIN) + " ,\n";
  response += "  \"currentNumberofColorsMax\": " + String(HTTP_CURRENTNUMBEROFCOLORS_MAX) + " \n";
  response += "}";
  
  const int length = response.length(); 
  // declaring character array (+1 for null terminator) 
  char* char_array = new char[length + 1]; 
  strcpy(char_array, response.c_str()); 
  request->send_P(200, "application/json", char_array);
  delete[] char_array;
}


void handle_UpdateInternalVariables(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    //String body = request->arg("plain");
    DEBUG_MSG_HUE("Received the following contents via HTTP Post Request in handle_UpdateInternalVariables:");
    DEBUG_MSG_HUE((const char*) data);

    DynamicJsonDocument bodyJSON(1024);
    DeserializationError error = deserializeJson(bodyJSON, data, len);

    if (error) {
        DEBUG_MSG_HUE("Failed to parse JSON");
        return;
    }

    short DelayBetweenRipples = bodyJSON.containsKey("currentDelayBetweenRipples") ? bodyJSON["currentDelayBetweenRipples"] : GlobalParameters.currentDelayBetweenRipples;
    short RainbowDeltaPerTick = bodyJSON.containsKey("currentRainbowDeltaPerTick") ? bodyJSON["currentRainbowDeltaPerTick"] : GlobalParameters.currentRainbowDeltaPerTick;
    unsigned long RippleLifeSpan = bodyJSON.containsKey("currentRippleLifeSpan") ? bodyJSON["currentRippleLifeSpan"] : GlobalParameters.currentRippleLifeSpan;
    float RippleSpeed = bodyJSON.containsKey("currentRippleSpeed") ? (float) bodyJSON["currentRippleSpeed"] : GlobalParameters.currentRippleSpeed;
    RippleSpeed = RippleSpeed/100; //scaling for proper display on HTML webpage
    int NumberofColors = bodyJSON.containsKey("currentNumberofColors") ? bodyJSON["currentNumberofColors"] : GlobalParameters.currentNumberofColors;
    int Behavior = bodyJSON.containsKey("currentBehavior") ? bodyJSON["currentBehavior"] : GlobalParameters.currentBehavior;
    int Direction = bodyJSON.containsKey("currentDirection") ? bodyJSON["currentDirection"] : GlobalParameters.currentDirection;
    float Decay = bodyJSON.containsKey("currentDecay") ? (float) bodyJSON["currentDecay"] : GlobalParameters.currentDecay;
    Decay = Decay/1000; //scaling for proper display on HTML webpage

    if(DelayBetweenRipples != GlobalParameters.currentDelayBetweenRipples){
      DEBUG_MSG_HUE("received new DelayBetweenRipples from POST request: %d", DelayBetweenRipples);
      if(DelayBetweenRipples >= HTTP_CURRENTDELAYBETWEENRIPPLES_MIN && DelayBetweenRipples <= HTTP_CURRENTDELAYBETWEENRIPPLES_MAX){ /* new value received */
        DEBUG_MSG_HUE("New value accepted. Previous value: %d", GlobalParameters.currentDelayBetweenRipples);
        GlobalParameters.currentDelayBetweenRipples = DelayBetweenRipples;
      } else {
        DEBUG_MSG_HUE(". New DelayBetweenRipples not valid; discarded.");
      }
    }

    if(RainbowDeltaPerTick != GlobalParameters.currentRainbowDeltaPerTick){
      DEBUG_MSG_HUE("received new RainbowDeltaPerTick from POST request: %d", RainbowDeltaPerTick);
      if(RainbowDeltaPerTick >= HTTP_CURRENTRAINBOWDELTAPERTICK_MIN && RainbowDeltaPerTick <= HTTP_CURRENTRAINBOWDELTAPERTICK_MAX){ /* new value received */
        DEBUG_MSG_HUE("New value accepted. Previous value: %d", GlobalParameters.currentRainbowDeltaPerTick);
        GlobalParameters.currentRainbowDeltaPerTick = RainbowDeltaPerTick;
      } else {
        DEBUG_MSG_HUE(". New RainbowDeltaPerTick not valid; discarded.");
      }
    }

    if(RippleLifeSpan != GlobalParameters.currentRippleLifeSpan){
      DEBUG_MSG_HUE("received new RippleLifeSpan from POST request: %d", RippleLifeSpan);
      if(RippleLifeSpan >= HTTP_CURRENTRIPPLELIFESPAN_MIN && RippleLifeSpan <= HTTP_CURRENTRIPPLELIFESPAN_MAX){ /* new value received */
        DEBUG_MSG_HUE("New value accepted. Previous value: %d", GlobalParameters.currentRippleLifeSpan);
        GlobalParameters.currentRippleLifeSpan = RippleLifeSpan;
      } else {
        DEBUG_MSG_HUE(". New RippleLifeSpan not valid; discarded.");
      }
    }

    if(RippleSpeed != GlobalParameters.currentRippleSpeed){
      DEBUG_MSG_HUE("received new Ripple Speed from POST request: %.2f", RippleSpeed);
      if(RippleSpeed >= HTTP_CURRENTRIPPLESPEED_MIN && RippleSpeed <= HTTP_CURRENTRIPPLESPEED_MAX){ /* new value received */
        DEBUG_MSG_HUE("New value accepted. Previous value: %.2f", GlobalParameters.currentRippleSpeed);
        GlobalParameters.currentRippleSpeed = RippleSpeed;
      } else {
        DEBUG_MSG_HUE(". New Ripple Speed not valid; discarded.");
      }
    }

    if(NumberofColors != GlobalParameters.currentNumberofColors){
      DEBUG_MSG_HUE("received new NumberofColors from POST request: %d", NumberofColors);
      if(NumberofColors >= HTTP_CURRENTNUMBEROFCOLORS_MIN && NumberofColors <= HTTP_CURRENTNUMBEROFCOLORS_MAX){ /* new value received */
        DEBUG_MSG_HUE("New value accepted. Previous value: %d", GlobalParameters.currentNumberofColors);
        GlobalParameters.currentNumberofColors = NumberofColors;
      } else {
        DEBUG_MSG_HUE(". New NumberofColors not valid; discarded.");
      }
    }

    if(Behavior != GlobalParameters.currentBehavior){
      DEBUG_MSG_HUE("received new Behavior from POST request: %d", Behavior);
      if(Behavior >= 0 && Behavior <= 4){ /* new value received */
        DEBUG_MSG_HUE("New value accepted. Previous value: %d", GlobalParameters.currentBehavior);
        GlobalParameters.currentBehavior = Behavior;
      } else {
        DEBUG_MSG_HUE(". New Behavior not valid; discarded.");
      }
    }

    if(Direction != GlobalParameters.currentDirection){
      DEBUG_MSG_HUE("received new Direction from POST request: %d", Direction);
      if(Direction >= -1 && Direction <= 6){ /* new value received */
        DEBUG_MSG_HUE("New value accepted. Previous value: %d", GlobalParameters.currentDirection);
        GlobalParameters.currentDirection = Direction;
      } else {
        DEBUG_MSG_HUE(". New Direction not valid; discarded.");
      }
    }

    if(Decay != GlobalParameters.currentDecay){
      DEBUG_MSG_HUE("received new Decay factor from POST request: %.3f", Decay);
      if(Decay >= HTTP_CURRENTDECAY_MIN && Decay <= HTTP_CURRENTDECAY_MAX){ /* new value received */
        DEBUG_MSG_HUE("New value accepted. Previous value: %.3f", GlobalParameters.currentDecay);
        GlobalParameters.currentDecay = Decay;
      } else {
        DEBUG_MSG_HUE(". New Decay not valid; discarded.");
      }
    }

    request->send_P(200, "application/json", "{}");
}


/* deprecated; for reference for profilemanagement*/
void handle_OnConnect(AsyncWebServerRequest *request) {
  DEBUG_MSG_HUE("New client connected!");

  // Display the HTML web page
String ptr = "<!DOCTYPE html> <html>\n";
ptr += "<html>\n";
ptr += "<head>\n";
ptr += "  <meta http-equiv='content-type' content='text/html; charset=UTF-8'>\n";
ptr += "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n";
ptr += "  <meta http-equiv='Cache-Control' content='no-cache, no-store, must-revalidate'>\n";
ptr += "  <meta http-equiv='Pragma' content='no-cache'>\n";
ptr += "  <meta http-equiv='Expires' content='0'>\n";
ptr += "  <link rel='icon' href='data:,'>\n";

/****************** BEGINNING OF CSS STYLE ******************/
/****************** END OF CSS STYLE ******************/

ptr += "</head>\n";
ptr += "<body>\n";
ptr += "  <h1>ESP32 Web Server</h1>\n";
  ptr += "<section id=\"checkboxes\"\n>";
  /* Display ON/OFF checkbox for loop_MasterFireRippleEnabled */
  if (GlobalParameters.loop_MasterFireRippleEnabled) {
    ptr += "<p><input type=\"checkbox\" id=\"master-auto-ripple-checkbox\" name=\"master-auto-ripple\" value=\"1\" onchange=\"toggleMasterAutoRipple(this)\" checked><label for=\"master-auto-ripple-checkbox\">Master Enable Automatic Ripples</label></p>\n";
  } else {
    ptr += "<p><input type=\"checkbox\" id=\"master-auto-ripple-checkbox\" name=\"master-auto-ripple\" value=\"1\" onchange=\"toggleMasterAutoRipple(this)\"><label for=\"master-auto-ripple-checkbox\">Master Enable Automatic Ripples</label></p>\n";
  }
  ptr += "</section\n>";
/****************** BEGINNING OF PROFILE MANAGEMENT ******************/
  ptr += "<section id=\"profileManagement\"\n>";
  ptr += "    <h1>User Profiles</h1>\n";
  ptr += "<p> Current profile selected: " + String(currentSelectedProfile + 1) + "</p>\n";
  if(currentLoadedProfile == -1) ptr += "<p> Last profile loaded: N/A </p\n>";
  else ptr += "<p> Last profile loaded: " + String(currentLoadedProfile + 1) + "</p>\n";
  ptr += "<div\n>";
  ptr += "<div id=\"profiles\"\n>";
  /* button 1*/
  ptr += "<button onclick='selectProfile(0)' class='Profile ";
  if (ProfilesAvailable[0] == 1U) ptr += "ActiveProfile ";
  else ptr += "InactiveProfile ";
  if (currentLoadedProfile == 0) ptr += "LoadedProfile ";
  if (currentSelectedProfile == 0) ptr += "SelectedProfile ";
  ptr += "'>#1</button></a>\n";
  /* button 2*/
  ptr += "<button onclick='selectProfile(1)' class='Profile ";
  if (ProfilesAvailable[1] == 1U) ptr += "ActiveProfile ";
  else ptr += "InactiveProfile ";
  if (currentLoadedProfile == 1) ptr += "LoadedProfile ";
  if (currentSelectedProfile == 1) ptr += "SelectedProfile ";
  ptr += "'>#2</button></a>\n";
  /* button 3*/
  ptr += "<button onclick='selectProfile(2)' class='Profile ";
  if (ProfilesAvailable[2] == 1U) ptr += "ActiveProfile ";
  else ptr += "InactiveProfile ";
  if (currentLoadedProfile == 2) ptr += "LoadedProfile ";
  if (currentSelectedProfile == 2) ptr += "SelectedProfile ";
  ptr += "'>#3</button></a>\n";
  /* button 4*/
  ptr += "<button onclick='selectProfile(3)' class='Profile ";
  if (ProfilesAvailable[3] == 1U) ptr += "ActiveProfile ";
  else ptr += "InactiveProfile ";
  if (currentLoadedProfile == 3) ptr += "LoadedProfile ";
  if (currentSelectedProfile == 3) ptr += "SelectedProfile ";
  ptr += "'>#4</button></a>\n";
  /* button 5*/
  ptr += "<button onclick='selectProfile(4)' class='Profile ";
  if (ProfilesAvailable[4] == 1U) ptr += "ActiveProfile ";
  else ptr += "InactiveProfile ";
  if (currentLoadedProfile == 4) ptr += "LoadedProfile ";
  if (currentSelectedProfile == 4) ptr += "SelectedProfile ";
  ptr += "'>#5</button></a>\n";
  ptr += "</div\n>";
  ptr += "      <div>\n";
  ptr += "          <button class='button' onclick = 'saveCurrentSelectedProfile()'> Save </button>\n";
  ptr += "          <button class='button' onclick = 'loadCurrentSelectedProfile()'> Load </button>\n";
  ptr += "          <button class='button' style='background-color: red;' onclick = 'deleteCurrentSelectedProfile()'> Delete </button>\n";
  ptr += "      </div>\n";
  ptr += "</div\n>";
    ptr += "    <h1> Presets</ h1>\n ";
  ptr += "      <div>\n";
  ptr += "        <button onclick='selectPreset(1)' class='preset'>Restore default settings</button>\n";
  ptr += "        <button onclick='selectPreset(2)' class='preset'>TBD</button>\n";
  ptr += "        <button onclick='selectPreset(3)' class='preset'>TBD</button>\n";
  ptr += "        <button onclick='selectPreset(0)' class='preset'>TBD</button>\n";
  ptr += "        <button onclick='selectPreset(0)' class='preset'>TBD</button>\n";
  ptr += "      </div>\n";
  ptr += "</section\n>";
  /****************** END OF PROFILE MANAGEMENT ******************/
  /****************** BEGINNING USER INPUT ******************/
/****************** END OF USER INPUT ******************/
  ptr += "</body>\n";
  ptr += "</html>\n";
  ptr += "\n";
  const char* buffer = ptr.c_str();
  request->send_P(200, "text/html", buffer);
/****************** BEGINNING OF JAVASCRIPT ******************/

//  ptr += HTTP_SERVER_JAVASCRIPT;
/****************** END OF JAVASCRIPT ******************/
}


void handle_ManualRipple(AsyncWebServerRequest *request) {
  DEBUG_MSG_HUE("Received manual ripple request");
  manualFireRipple = 1;
  //request->send_P(200, "text/html", SendHTML_Dashboard());
}

void handle_profileManagement(AsyncWebServerRequest *request) {
  DEBUG_MSG_HUE("received POST request for profile management");
  String body = request->arg("plain");
  DEBUG_MSG_HUE(body.c_str());

  /* local variables*/
  presetType preset = no_preset;
  bool deleteProfileRequest = 0U;
  bool saveProfileRequest = 0U;
  bool loadProfileRequest = 0U;
  bool loadProfileRequest_return = 0U;

  if (body.length() == 0){
      char response[strlen_P(HUE_ERROR_TEMPLATE) + request->url().length() + 40];
      snprintf_P(
          response, sizeof(response),
          HUE_ERROR_TEMPLATE,
          5,
          request->url().c_str(),
          "invalid/missing parameters in body");        
      request->send_P(400, "application/json", response);
  }else{
    SimpleJson json;
    json.parse(body);
    /* USER PROFILE MANAGEMENT */
    if(json.hasPropery("selectProfile")){
      if( (json["selectProfile"].getInt()) >= 0 && (json["selectProfile"].getInt() < EEPROM_SUPPORTED_PROFILES) ){
       currentSelectedProfile = json["selectProfile"].getInt();
        DEBUG_MSG_HUE("new profile selected: ");
        Serial.println(currentSelectedProfile);
      }else{
        DEBUG_MSG_HUE("received new INVALID profile: ");
        Serial.print(json["selectProfile"].getInt());
        DEBUG_MSG_HUE(". Discarding - current profile is still ");
        Serial.println(currentSelectedProfile);
      }
    }
    
    if(json.hasPropery("deleteProfile")){
      deleteProfileRequest = (bool) json["deleteProfile"].getInt();
      if(deleteProfileRequest == 1U){
        DEBUG_MSG_HUE("received request to delete current profile");
        EEPROM_InvalidateProfile(currentSelectedProfile); 
        if(currentLoadedProfile == currentSelectedProfile) currentLoadedProfile = -1; /*loaded profile was the one we just deleted! */
      }
    }

    if(json.hasPropery("loadProfile")){
      loadProfileRequest = (bool) json["loadProfile"].getInt();
      if(loadProfileRequest == 1U){
        DEBUG_MSG_HUE("received request to load current profile");
        loadProfileRequest_return = EEPROM_RestoreProfile(currentSelectedProfile);
        if(loadProfileRequest_return) currentLoadedProfile = currentSelectedProfile;
      }
    }

    if(json.hasPropery("saveProfile")){
      saveProfileRequest = (bool) json["saveProfile"].getInt();
      if(saveProfileRequest == 1U){
        DEBUG_MSG_HUE("received request to save current profile");
        EEPROM_StoreProfile(currentSelectedProfile);
        currentLoadedProfile = currentSelectedProfile;
      }
    }

    /* PRESET MANAGEMENT */
    if(json.hasPropery("selectPreset")){
      preset = (presetType) json["selectPreset"].getInt();
        DEBUG_MSG_HUE("new preset selected: ");
        Serial.println(preset);

      switch(preset){
      case default_preset:
        DEBUG_MSG_HUE("received request for preset Rainbow Trails");
        GlobalParameters.currentNumberofColors = HTTP_CURRENTNUMBEROFCOLORS_DEFAULT;
        GlobalParameters.currentBehavior = HTTP_CURRENTBEHAVIOR_DEFAULT;
        GlobalParameters.currentDirection = HTTP_CURRENTDIRECTION_DEFAULT;
        GlobalParameters.currentDelayBetweenRipples = HTTP_CURRENTDELAYBETWEENRIPPLES_DEFAULT; /* in milliseconds */
        GlobalParameters.currentRainbowDeltaPerTick = HTTP_CURRENTRAINBOWDELTAPERTICK_DEFAULT; /* units: hue */
        GlobalParameters.currentRippleLifeSpan = HTTP_CURRENTRIPPLELIFESPAN_DEFAULT;           /* in milliseconds */
        GlobalParameters.currentRippleSpeed = HTTP_CURRENTRIPPLESPEED_DEFAULT;
        GlobalParameters.currentDecay = HTTP_CURRENTDECAY_DEFAULT;
        break;
      case RainbowTrails:
        DEBUG_MSG_HUE("received request for preset Rainbow Trails");
        break;
      case LongTrails:
        DEBUG_MSG_HUE("received request for  preset Long Trails");
        break;
      }
    }
  }
  //request->send_P(200, "text/html", SendHTML_Dashboard());
}


void handle_SWreset(AsyncWebServerRequest *request) {
  ESP.restart();
}

/* checkbox handling */
void handle_MasterFireRippleEnabled_On(AsyncWebServerRequest *request) {
  DEBUG_MSG_HUE("Master Automatic ripples: ON");
  GlobalParameters.loop_MasterFireRippleEnabled = 1;
}

void handle_MasterFireRippleEnabled_Off(AsyncWebServerRequest *request) {
  DEBUG_MSG_HUE("Master Automatic ripples: OFF");
  GlobalParameters.loop_MasterFireRippleEnabled = 0;
}
/* end of checkbox handling*/

/* to be called periodically inside loop() */
void WiFi_MainFunction(void){
  //server.handleClient();
}

void handle_SendDashboard(AsyncWebServerRequest *request){
  request->send(SPIFFS, "/oneindex.html", String(), false, nullptr);
}

/* to be called once at startup */
void WiFi_init(void){
  /* Setup WiFi network */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    DEBUG_MSG_HUE("Connection Failed! Rebooting...");
    delay(1000);
    ESP.restart();
  }

  /* Setup REST API Handlers */
  //server.on("/dashboard", handle_OnConnect);
  // Route to set GPIO to HIGH
  server.on("/dashboard", HTTP_GET, handle_SendDashboard);
  server.on("/ManualRipple", handle_ManualRipple);
  server.on("/profileManagement", handle_profileManagement);
  server.on("/SWreset", handle_SWreset);
  server.on("/MasterFireRippleEnabled/on", handle_MasterFireRippleEnabled_On);
  server.on("/MasterFireRippleEnabled/off", handle_MasterFireRippleEnabled_Off);
  server.on("/getInternalVariables", handle_getInternalVariables); 
  server.on("/updateInternalVariables", HTTP_POST, handle_SendDashboard, nullptr, handle_UpdateInternalVariables); 
  
  /* server already begun by hueBrdige */
  //server.begin();
  //server.enableDelay(false); /* refer to comment from scottchiefbaker in https://github.com/espressif/arduino-esp32/issues/7708*/
  Serial.printf("Wifi connected, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  //Serial.println(WiFi.localIP());

  // Setup Multicast DNS https://en.wikipedia.org/wiki/Multicast_DNS 
  // You can open http://hexagono.local in Chrome on a desktop
  DEBUG_MSG_HUE("Setup MDNS for http://hexagono.local");
  if (!MDNS.begin("hexagono"))
  {
    DEBUG_MSG_HUE("Error setting up MDNS responder!");
  }
}
