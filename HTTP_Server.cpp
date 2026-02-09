#include <WiFi.h>
#include <WebServer.h>
#include <string>  
using namespace std;

#include "HTTP_Server.h" 
#include "MCAL/ripple.h"
#include "MCAL/EEP.h"
#include "Alexa/SimpleJson.h"
#include "WiFi_utilities.h"

unsigned long currentTime = 0;
unsigned long previousTime = 0;
const long timeout = 2000;

// Auxiliar variables to store the current output state
GlobalParameters_struct GlobalParameters;



boolean manualFireRipple = 0;
unsigned int currentSelectedProfile = 0;
unsigned int currentLoadedProfile = -1;

void setupDefaultProfileParameters(RippleProfile_struct* RippleProfile)
{
  /* set up default profile parameters in GlobalParameters struct */
  RippleProfile->Active = 1;
  RippleProfile->Behavior = BEHAVIOR_DEFAULT;
  RippleProfile->Direction = DIRECTION_DEFAULT; /* all directions */ 
  RippleProfile->DelayBetweenRipples_ms = DELAYBETWEENRIPPLES_DEFAULT;
  
  RippleProfile->RippleLifeSpan = RIPPLELIFESPAN_DEFAULT;
  RippleProfile->RippleSpeed = RIPPLESPEED_DEFAULT;
  RippleProfile->RainbowDeltaPerTick = RAINBOWDELTAPERTICK_DEFAULT;
  RippleProfile->NumberOfColors = NUMBEROFCOLORS_DEFAULT;
  for (int i = 0; i < RippleProfile->NumberOfColors; i++)
  {
    RippleProfile->Colors[0] = 0xFF0000; /* red */
    RippleProfile->Colors[1] = 0xFF7F00; /* orange */
    RippleProfile->Colors[2] = 0xFFFF00; /* yellow */
    RippleProfile->Colors[3] = 0x00FF00; /* green */
    RippleProfile->Colors[4] = 0x0000FF; /* blue */
    RippleProfile->Colors[5] = 0x4B0082; /* indigo */
    RippleProfile->Colors[6] = 0x8B00FF; /* violet */

  }
  RippleProfile->CurrentColor = 0;
  RippleProfile->TimeLastRippleFired_ms = 0;
  RippleProfile->ActiveNodes[starburstNode] = 1; /* activate profile on starburst node */
  strncpy(GlobalParameters.RippleProfiles[0].ProfileName, "New Profile", MAX_PROFILE_NAME_LEN);
}

/* HANDLER FUNCTIONS */
void handle_getCurrentProfiles(AsyncWebServerRequest *request) {
  udp_printf("Received getCurrentProfiles request");

  String response = "{\n";
  response += "\"MasterFireRippleEnabled\": " + String(GlobalParameters.MasterFireRippleEnabled) + ",\n";
  response += "\"Decay\": " + String(GlobalParameters.Decay) + ",\n";
  response += "\"NumberOfActiveProfiles\": " + String(GlobalParameters.NumberOfActiveProfiles) + ",\n";
  response += "\"Profiles\": [\n"; 
  for(int i = 0; i < GlobalParameters.NumberOfActiveProfiles; i++){
    response += "  {\n";
    response += "    \"ProfileIndex\": " + String(i) + ",\n";
    response += "    \"ProfileName\": \"" + String(GlobalParameters.RippleProfiles[i].ProfileName) + "\",\n";
    response += "    \"Active\": " + String(GlobalParameters.RippleProfiles[i].Active) + ",\n";
    response += "    \"ActiveNodes\": [";
    for(int j = 0; j < NUMBER_OF_NODES; j++){
      response += String(GlobalParameters.RippleProfiles[i].ActiveNodes[j]);
      if(j < NUMBER_OF_NODES - 1){
        response += ", ";
      }
    }
    response += "],\n";
    response += "    \"Behavior\": " + String(GlobalParameters.RippleProfiles[i].Behavior) + ",\n";
    response += "    \"Direction\": " + String(GlobalParameters.RippleProfiles[i].Direction) + ",\n";
    response += "    \"RippleLifeSpan\": " + String(GlobalParameters.RippleProfiles[i].RippleLifeSpan) + ",\n";
    response += "    \"DelayBetweenRipples_ms\": " + String(GlobalParameters.RippleProfiles[i].DelayBetweenRipples_ms) + ",\n";
    response += "    \"RippleSpeed\": " + String(GlobalParameters.RippleProfiles[i].RippleSpeed) + ",\n";
    response += "    \"RainbowDeltaPerTick\": " + String(GlobalParameters.RippleProfiles[i].RainbowDeltaPerTick) + ",\n";
    response += "    \"NumberOfColors\": " + String(GlobalParameters.RippleProfiles[i].NumberOfColors) + ",\n";
    response += "    \"Colors\": [";
    for(int k = 0; k < GlobalParameters.RippleProfiles[i].NumberOfColors; k++){
      char colorString[8];
      sprintf(colorString, "\"#%06X\"", GlobalParameters.RippleProfiles[i].Colors[k]);
      response += String(colorString);
      if(k < GlobalParameters.RippleProfiles[i].NumberOfColors - 1){
        response += ", ";
      }
    }
    response += "],\n";
    response += "    \"delayBetweenRipples_ms\": " + String(GlobalParameters.RippleProfiles[i].DelayBetweenRipples_ms) + "\n";

    response += "  }";
    if(i < GlobalParameters.NumberOfActiveProfiles - 1){
      response += ",\n";
    } else {
      response += "\n";
    }
  }
  response += "]\n";
  response += "}\n";

  udp_printf("handle_getCurrentProfiles response: %s", response.c_str());

  const int length = response.length(); 
  // declaring character array (+1 for null terminator) 
  char* char_array = new char[length + 1]; 
  strcpy(char_array, response.c_str()); 
  request->send_P(200, "application/json", char_array);
  delete[] char_array;
}


/* expected JSON format example for POST request made to handle_UpdateProfile:
Remember ActiveNodes is an array of 19 elements, one for each node
{
  "ProfileIndex": 0,
  "ProfileName": "My Profile",
  "Active": true,
  "ActiveNodes": [0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0],
  "Behavior": 1,
  "Direction": 0,
  "RippleLifeSpan": 5000,
  "DelayBetweenRipples_ms": 1000,
  "RippleSpeed": 1.0,
  "RainbowDeltaPerTick": 100,
  "NumberOfColors": 3,
  "Colors": ["#FF0000", "#00FF00", "#0000FF"]
}
*/
void handle_UpdateProfile(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    String body = String((const char*)data, len);
    udp_printf("Received the following contents via HTTP Post Request in handle_UpdateProfile:");
    udp_printf("%s", body.c_str());

    DynamicJsonDocument bodyJSON(1024);
    DeserializationError error = deserializeJson(bodyJSON, body);
    if (error) {
        udp_printf("JSON parse error: %s", error.c_str());
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}\n");
        return;
    }

    // extract profile index from JSON request to see which profile is being updated
    int ProfileIndex = bodyJSON["ProfileIndex"];
    if (ProfileIndex < 0 || ProfileIndex > GlobalParameters.NumberOfActiveProfiles) { // must be an existing profile or the next one to be created
        udp_printf("Invalid ProfileIndex: %d", ProfileIndex);
        request->send(400, "application/json", "{\"error\":\"Invalid ProfileIndex\"}\n");
        return;
    }
  
    if(bodyJSON.containsKey("DeleteProfile") && bodyJSON["DeleteProfile"] == true){
      if(GlobalParameters.NumberOfActiveProfiles <= 1){
        udp_printf("Cannot delete the only active profile");
        request->send(400, "application/json", "{\"error\":\"Cannot delete the only active profile\"}\n");
        return;
      }
      // delete this profile by shifting all subsequent profiles down by one
      udp_printf(" - Deleting profile at index %d", ProfileIndex);
      for(int i = ProfileIndex; i < GlobalParameters.NumberOfActiveProfiles - 1; i++){
        GlobalParameters.RippleProfiles[i] = GlobalParameters.RippleProfiles[i + 1];
      }
      GlobalParameters.NumberOfActiveProfiles--;
      udp_printf(" - Profile deleted. Total number of active profiles is now %d", GlobalParameters.NumberOfActiveProfiles);
      request->send_P(200, "application/json", "{}");
      return;
    }

    if(ProfileIndex > GlobalParameters.NumberOfActiveProfiles){
      // this is a new profile, so increment the number of active profiles
      GlobalParameters.NumberOfActiveProfiles++;
      udp_printf("Creating new profile at index %d. Total number of active profiles is now %d", ProfileIndex, GlobalParameters.NumberOfActiveProfiles);
      setupDefaultProfileParameters(&GlobalParameters.RippleProfiles[ProfileIndex]);
      udp_printf(" - Default parameters set for new profile at index %d", ProfileIndex);
    } else {
      udp_printf("Updating existing profile with name \"%s\" at index %d", GlobalParameters.RippleProfiles[ProfileIndex].ProfileName, ProfileIndex);
    }

    RippleProfile_struct* profile = &GlobalParameters.RippleProfiles[ProfileIndex];

    // Now update the profile fields based on the JSON input
    //first extract all variables from JSON into local copies
    udp_printf(" - Extracting parameters from JSON body");
    String ProfileNameStr = bodyJSON["ProfileName"] | String(profile->ProfileName);
    const char* ProfileName = ProfileNameStr.c_str();
    boolean ActiveNodes[NUMBER_OF_NODES];
    if (bodyJSON.containsKey("ActiveNodes")) {
        JsonArray nodesArray = bodyJSON["ActiveNodes"].as<JsonArray>();
        for (int i = 0; i < NUMBER_OF_NODES; i++) {
            ActiveNodes[i] = nodesArray[i].as<int>(); // direct assignment, don't use | fallback
        }
    } else {
        for (int i = 0; i < NUMBER_OF_NODES; i++) {
            ActiveNodes[i] = profile->ActiveNodes[i];
        }
    }
    rippleBehavior Behavior = bodyJSON.containsKey("Behavior") ? static_cast<rippleBehavior>(bodyJSON["Behavior"].as<int>()) : profile->Behavior;
    signed char Direction = bodyJSON.containsKey("Direction") ? bodyJSON["Direction"].as<signed char>() : profile->Direction;
    unsigned long RippleLifeSpan = bodyJSON.containsKey("RippleLifeSpan") ? bodyJSON["RippleLifeSpan"].as<unsigned long>() : profile->RippleLifeSpan;
    float RippleSpeed = bodyJSON.containsKey("RippleSpeed") ? bodyJSON["RippleSpeed"].as<float>() : profile->RippleSpeed;
    short RainbowDeltaPerTick = bodyJSON.containsKey("RainbowDeltaPerTick") ? bodyJSON["RainbowDeltaPerTick"].as<short>() : profile->RainbowDeltaPerTick;
    unsigned int Colors[16];
    unsigned int NumberOfColors = bodyJSON.containsKey("NumberOfColors") ? bodyJSON["NumberOfColors"].as<unsigned int>() : profile->NumberOfColors;
    if (bodyJSON.containsKey("Colors")) {
        JsonArray colorsArray = bodyJSON["Colors"].as<JsonArray>();
        for (int i = 0; i < NumberOfColors && i < 16; i++) {
            String colorStr = colorsArray[i] | String("#000000");
            Colors[i] = (unsigned int) strtol(colorStr.c_str() + 1, NULL, 16); // convert hex string to unsigned int
        }
    } else {
        for (int i = 0; i < profile->NumberOfColors; i++) {
            Colors[i] = profile->Colors[i];
        }
    }
    short DelayBetweenRipples_ms = bodyJSON.containsKey("DelayBetweenRipples_ms") ? bodyJSON["DelayBetweenRipples_ms"].as<short>() : profile->DelayBetweenRipples_ms;
    // Now apply the updates to the profile only if parameters are new and valid
    udp_printf(" - Finished extraction from JSON, applying parameter updates to profile at index %d", ProfileIndex);
    if(strcmp(profile->ProfileName, ProfileName) != 0){
      strncpy(profile->ProfileName, ProfileName, sizeof(profile->ProfileName) - 1);
      profile->ProfileName[sizeof(profile->ProfileName) - 1] = '\0'; // ensure termination
    } else{
      udp_printf(" - ProfileName remains %s", profile->ProfileName);
    }
    for(int i = 0; i < NUMBER_OF_NODES; i++){
      if(profile->ActiveNodes[i] != ActiveNodes[i]){
        profile->ActiveNodes[i] = ActiveNodes[i];
        udp_printf(" - Updated ActiveNodes[%d] to %d", i, profile->ActiveNodes[i]);
      } else{
        udp_printf(" - ActiveNodes[%d] remains %d", i, profile->ActiveNodes[i]);
      }
    }
    if(profile->Behavior != Behavior){
      profile->Behavior = Behavior;
      udp_printf(" - Updated Behavior to %d", profile->Behavior);
    } else{
      udp_printf(" - Behavior remains %d", profile->Behavior);
    }
    if(profile->Direction != Direction){
      profile->Direction = Direction;
      udp_printf(" - Updated Direction to %d", profile->Direction);
    } else{
      udp_printf(" - Direction remains %d", profile->Direction);
    }
    if(profile->RippleLifeSpan != RippleLifeSpan){
      profile->RippleLifeSpan = RippleLifeSpan;
      udp_printf(" - Updated RippleLifeSpan to %lu", profile->RippleLifeSpan);
    } else{
      udp_printf(" - RippleLifeSpan remains %lu", profile->RippleLifeSpan);
    }
    if(profile->RippleSpeed != RippleSpeed){
      profile->RippleSpeed = RippleSpeed;
      udp_printf(" - Updated RippleSpeed to %.2f", profile->RippleSpeed);
    } else{
      udp_printf(" - RippleSpeed remains %.2f", profile->RippleSpeed);
    }
    if(profile->RainbowDeltaPerTick != RainbowDeltaPerTick){
      profile->RainbowDeltaPerTick = RainbowDeltaPerTick;
      udp_printf(" - Updated RainbowDeltaPerTick to %d", profile->RainbowDeltaPerTick);
    } else{
      udp_printf(" - RainbowDeltaPerTick remains %d", profile->RainbowDeltaPerTick);
    }
    boolean colorsChanged = false;
    if(profile->NumberOfColors != NumberOfColors && NumberOfColors > 0 && NumberOfColors <= 16){
      profile->NumberOfColors = NumberOfColors;
      colorsChanged = true;
      udp_printf(" - Updated NumberOfColors to %d", profile->NumberOfColors);
    } else{
      udp_printf(" - NumberOfColors remains %d", profile->NumberOfColors);
    }
    for(int i = 0; i < profile->NumberOfColors; i++){
      if(profile->Colors[i] != Colors[i]){
        profile->Colors[i] = Colors[i];
        colorsChanged = true;
        udp_printf(" - Updated Colors[%d] to #%06X", i, profile->Colors[i]);
      } else{
        udp_printf(" - Colors[%d] remains #%06X", i, profile->Colors[i]);
      }
    }
    if(colorsChanged){
      profile->CurrentColor = 0;
      udp_printf(" - Reset CurrentColor to 0");
    }
    if(profile->DelayBetweenRipples_ms != DelayBetweenRipples_ms){
      profile->DelayBetweenRipples_ms = DelayBetweenRipples_ms;
      udp_printf(" - Updated DelayBetweenRipples_ms to %d", profile->DelayBetweenRipples_ms);
    } else{
      udp_printf(" - DelayBetweenRipples_ms remains %d", profile->DelayBetweenRipples_ms);
    }
    if (bodyJSON.containsKey("Active")) {
      boolean Active = bodyJSON["Active"];
      if (profile->Active != Active) {
        profile->Active = Active;
        udp_printf(" - Updated Active to %d", profile->Active);
      } else {
        udp_printf(" - Active remains %d", profile->Active);
      }
    }

    request->send_P(200, "application/json", "{}");
}

/* Update global parameters like MasterFireRippleEnabled and Decay */
/* expected JSON format:
{
  "MasterFireRippleEnabled": true,
  "Decay": 0.985
}
*/
void handle_UpdateGlobalParameters(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    String body = String((const char*)data, len);
    udp_printf("Received the following contents via HTTP Post Request in handle_UpdateProfile:");
    udp_printf("%s", body.c_str());

    DynamicJsonDocument bodyJSON(1024);
    DeserializationError error = deserializeJson(bodyJSON, body);
    if (error) {
        udp_printf("JSON parse error: %s", error.c_str());
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}\n");
        return;
    }
    // extract global parameters from JSON request to see which parameters are being updated
    if (bodyJSON.containsKey("MasterFireRippleEnabled")) {
        GlobalParameters.MasterFireRippleEnabled = bodyJSON["MasterFireRippleEnabled"];
        udp_printf(" - Updated MasterFireRippleEnabled to %d", GlobalParameters.MasterFireRippleEnabled);
    } else {
        udp_printf(" - MasterFireRippleEnabled remains %d", GlobalParameters.MasterFireRippleEnabled);
    }
    if (bodyJSON.containsKey("Decay")) {
        GlobalParameters.Decay = bodyJSON["Decay"];
        udp_printf(" - Updated Decay to %.3f", GlobalParameters.Decay);
    } else {
        udp_printf(" - Decay remains %.3f", GlobalParameters.Decay);
    }

    request->send_P(200, "application/json", "{}");
}

void handle_ManualRipple(AsyncWebServerRequest *request) {
  udp_printf("Received manual ripple request");
  manualFireRipple = 1;
  request->send(LittleFS, "/oneindex.html", String(), false, nullptr);
}

/* checkbox handling */
void handle_MasterFireRippleEnabled_On(AsyncWebServerRequest *request) {
  udp_printf("Master Automatic ripples: ON");
  GlobalParameters.MasterFireRippleEnabled = 1;
}

void handle_MasterFireRippleEnabled_Off(AsyncWebServerRequest *request) {
  udp_printf("Master Automatic ripples: OFF");
  GlobalParameters.MasterFireRippleEnabled = 0;
}
/* end of checkbox handling*/

/* to be called once at startup */
void HTTP_backend_init(void){
  /* Setup REST API Handlers */
  server.on("/ManualRipple", handle_ManualRipple);
  server.on("/MasterFireRippleEnabled/on", handle_MasterFireRippleEnabled_On);
  server.on("/MasterFireRippleEnabled/off", handle_MasterFireRippleEnabled_Off);
  server.on("/getCurrentProfiles", handle_getCurrentProfiles); 
  server.on("/updateProfile", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, handle_UpdateProfile);
  server.on("/updateGlobalParameters", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, handle_UpdateGlobalParameters);
  server.on("/resetMicrocontroller", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              udp_printf("Received resetMicrocontroller request via HTTP");
              request->send(200, "text/plain", "Resetting microcontroller...\n");
              delay(1500);
              ESP.restart();
            });
  server.on("/clearEEPROM", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              udp_printf("Received clearEEPROM request via HTTP");
              request->send(200, "text/plain", "Clearing EEPROM and resetting microcontroller...\n");
              delay(1500);
              EEPROM_Clear();
              ESP.restart();
            });
}
