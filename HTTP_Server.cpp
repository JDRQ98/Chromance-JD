#include <WiFi.h>
#include <ArduinoJson.h>
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

/* Convert an RGB hex value (0xRRGGBB) to a 16-bit HSV hue (0-65535).
   Saturation and value are assumed to be max (pure color). */
static unsigned int rgbHexToHue16(unsigned int rgb) {
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = rgb & 0xFF;

  uint8_t maxC = max(r, max(g, b));
  uint8_t minC = min(r, min(g, b));

  if (maxC == minC) return 0; // achromatic (gray/white/black) → red hue

  float hue;
  float delta = maxC - minC;

  if (maxC == r) {
    hue = (float)(g - b) / delta;
    if (hue < 0) hue += 6.0f;
  } else if (maxC == g) {
    hue = 2.0f + (float)(b - r) / delta;
  } else {
    hue = 4.0f + (float)(r - g) / delta;
  }

  return (unsigned int)(hue * 65536.0f / 6.0f); // scale 0-6 → 0-65535
}

/* Convert a 16-bit HSV hue (0-65535) back to RGB hex (0xRRGGBB).
   Uses full saturation and value. */
static unsigned int hue16ToRgbHex(unsigned int hue16) {
  // Use Adafruit NeoPixel's ColorHSV which returns a packed GRB uint32_t
  // but we need an external strip reference — use a simpler approach instead.
  // HSV to RGB with S=255, V=255:
  uint8_t region = hue16 / 10923; // 65536/6 ≈ 10922.67
  uint16_t remainder = (hue16 - (region * 10923)) * 6; // scale remainder to 0-65535

  uint8_t q = 255 - ((255 * remainder) >> 16); // falling
  uint8_t t = 255 - ((255 * (65535 - remainder)) >> 16); // rising

  uint8_t r, g, b;
  switch (region % 6) {
    case 0:  r = 255; g = t;   b = 0;   break;
    case 1:  r = q;   g = 255; b = 0;   break;
    case 2:  r = 0;   g = 255; b = t;   break;
    case 3:  r = 0;   g = q;   b = 255; break;
    case 4:  r = t;   g = 0;   b = 255; break;
    default: r = 255; g = 0;   b = q;   break;
  }

  return ((unsigned int)r << 16) | ((unsigned int)g << 8) | b;
}



boolean manualFireRipple = 0;
unsigned int currentSelectedProfile = 0;
unsigned int currentLoadedProfile = -1;

SemaphoreHandle_t gParamsMutex = nullptr;
volatile int pendingKillProfileIndex = -1;

void setupDefaultProfileParameters(RippleProfile_struct* RippleProfile)
{
  /* set up default profile parameters in GlobalParameters struct */
  memset(RippleProfile, 0, sizeof(RippleProfile_struct));
  RippleProfile->Active = 1;
  RippleProfile->ProfilePeriod_ms = PROFILEPERIOD_DEFAULT;
  RippleProfile->NumberOfColors = NUMBEROFCOLORS_DEFAULT;
  RippleProfile->Colors[0] = 0;      /* red (hue 0°) */
  RippleProfile->Colors[1] = 5461;   /* orange (hue ~30°) */
  RippleProfile->Colors[2] = 10922;  /* yellow (hue ~60°) */
  RippleProfile->Colors[3] = 21845;  /* green (hue ~120°) */
  RippleProfile->Colors[4] = 43690;  /* blue (hue ~240°) */
  RippleProfile->Colors[5] = 48497;  /* indigo (hue ~266°) */
  RippleProfile->Colors[6] = 51425;  /* violet (hue ~282°) */
  RippleProfile->CurrentColor = 0;
  RippleProfile->PeriodStartTime_ms = 0;

  /* Set up default event t0 */
  RippleProfile->Events[0].Enabled = true;
  RippleProfile->Events[0].TimeOffset_ms = 0;
  RippleProfile->Events[0].RippleLifeSpan = RIPPLELIFESPAN_DEFAULT;
  RippleProfile->Events[0].RippleType = RIPPLETYPE_SINGLE;
  RippleProfile->Events[0].Behavior = BEHAVIOR_DEFAULT;
  RippleProfile->Events[0].RippleSpeed = RIPPLESPEED_DEFAULT;
  RippleProfile->Events[0].RainbowDeltaPerTick = RAINBOWDELTAPERTICK_DEFAULT;
  RippleProfile->Events[0].Direction = DIRECTION_DEFAULT;
  RippleProfile->Events[0].ActiveNodes[starburstNode] = 1;
  /* Events[1-4] are zeroed by memset (Enabled = false) */

  strncpy(RippleProfile->ProfileName, "New Profile", MAX_PROFILE_NAME_LEN);
}

static void setupDefaultStableColorParameters(void) {
  GlobalParameters.StableColorMode = false;
  GlobalParameters.StableColorHue  = 43690;  /* blue ~240° */
  GlobalParameters.StableColorSat  = 255;
  GlobalParameters.PulseFrequency  = 0.3f;
  GlobalParameters.PulseDepth      = 0.4f;
  memset(GlobalParameters.StableColorSegments, true, NUMBER_OF_SEGMENTS);
}

/* HANDLER FUNCTIONS */
void handle_getCurrentProfiles(AsyncWebServerRequest *request) {
  udp_printf("Received getCurrentProfiles request");

  String response = "{\n";
  response += "\"MasterFireRippleEnabled\": " + String(GlobalParameters.MasterFireRippleEnabled) + ",\n";
  response += "\"Decay\": " + String(GlobalParameters.Decay) + ",\n";
  response += "\"Brightness\": " + String(GlobalParameters.Brightness) + ",\n";
  response += "\"NumberOfActiveProfiles\": " + String(GlobalParameters.NumberOfActiveProfiles) + ",\n";
  response += "\"SequencerEnabled\": " + String(GlobalParameters.SequencerEnabled ? "true" : "false") + ",\n";
  response += "\"SequencerMode\": " + String(GlobalParameters.SequencerMode) + ",\n";
  response += "\"SequencerDwellTime_s\": " + String(GlobalParameters.SequencerDwellTime_s) + ",\n";
  response += "\"SequencerCurrentProfile\": " + String(GlobalParameters.SequencerCurrentProfile) + ",\n";
  response += "\"GlobalBPM\": " + String(GlobalParameters.GlobalBPM, 1) + ",\n";
  /* Stable color mode fields */
  response += "\"StableColorMode\": " + String(GlobalParameters.StableColorMode ? "true" : "false") + ",\n";
  {
    char scHexStr[12];
    sprintf(scHexStr, "\"#%06X\"", hue16ToRgbHex(GlobalParameters.StableColorHue));
    response += "\"StableColorHue\": " + String(scHexStr) + ",\n";
  }
  response += "\"PulseFrequency\": " + String(GlobalParameters.PulseFrequency, 2) + ",\n";
  response += "\"PulseDepth\": " + String(GlobalParameters.PulseDepth, 2) + ",\n";
  response += "\"StableColorSegments\": [";
  for (int i = 0; i < NUMBER_OF_SEGMENTS; i++) {
    response += String(GlobalParameters.StableColorSegments[i] ? 1 : 0);
    if (i < NUMBER_OF_SEGMENTS - 1) response += ",";
  }
  response += "],\n";
  response += "\"Profiles\": [\n"; 
  for(int i = 0; i < GlobalParameters.NumberOfActiveProfiles; i++){
    response += "  {\n";
    response += "    \"ProfileIndex\": " + String(i) + ",\n";
    response += "    \"ProfileName\": \"" + String(GlobalParameters.RippleProfiles[i].ProfileName) + "\",\n";
    response += "    \"Active\": " + String(GlobalParameters.RippleProfiles[i].Active) + ",\n";
    response += "    \"ProfilePeriod_ms\": " + String(GlobalParameters.RippleProfiles[i].ProfilePeriod_ms) + ",\n";
    response += "    \"NumberOfColors\": " + String(GlobalParameters.RippleProfiles[i].NumberOfColors) + ",\n";
    response += "    \"Colors\": [";
    for(int k = 0; k < GlobalParameters.RippleProfiles[i].NumberOfColors; k++){
      char colorString[12];
      unsigned int rgbColor = hue16ToRgbHex(GlobalParameters.RippleProfiles[i].Colors[k]);
      sprintf(colorString, "\"#%06X\"", rgbColor);
      response += String(colorString);
      if(k < GlobalParameters.RippleProfiles[i].NumberOfColors - 1){
        response += ", ";
      }
    }
    response += "],\n";
    response += "    \"Events\": [\n";
    for(int e = 0; e < MAX_EVENTS_PER_PROFILE; e++){
      TimeEvent_struct* evt = &GlobalParameters.RippleProfiles[i].Events[e];
      response += "      {";
      response += "\"Enabled\":" + String(evt->Enabled ? "true" : "false") + ",";
      response += "\"TimeOffset_ms\":" + String(evt->TimeOffset_ms) + ",";
      response += "\"RippleLifeSpan\":" + String(evt->RippleLifeSpan) + ",";
      response += "\"RippleType\":" + String(evt->RippleType) + ",";
      response += "\"Behavior\":" + String(evt->Behavior) + ",";
      response += "\"RippleSpeed\":" + String(evt->RippleSpeed) + ",";
      response += "\"RainbowDeltaPerTick\":" + String(evt->RainbowDeltaPerTick) + ",";
      response += "\"Direction\":" + String(evt->Direction) + ",";
      response += "\"ActiveNodes\":[";
      for(int j = 0; j < NUMBER_OF_NODES; j++){
        response += String(evt->ActiveNodes[j]);
        if(j < NUMBER_OF_NODES - 1) response += ",";
      }
      response += "]}";
      if(e < MAX_EVENTS_PER_PROFILE - 1) response += ",";
      response += "\n";
    }
    response += "    ]\n";
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

  request->send(200, "application/json", response);
}


/* expected JSON format example for POST request made to handle_UpdateProfile:
{
  "ProfileIndex": 0,
  "ProfileName": "My Profile",
  "Active": true,
  "ProfilePeriod_ms": 5000,
  "NumberOfColors": 3,
  "Colors": ["#FF0000", "#00FF00", "#0000FF"],
  "Events": [
    {"Enabled":true,"TimeOffset_ms":0,"RippleLifeSpan":5000,"RippleType":0,"Behavior":1,"RippleSpeed":0.5,"RainbowDeltaPerTick":200,"Direction":-1,"ActiveNodes":[0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0]},
    {"Enabled":false},{"Enabled":false},{"Enabled":false},{"Enabled":false}
  ]
}
*/
void handle_UpdateProfile(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    String body = String((const char*)data, len);
    udp_printf("Received the following contents via HTTP Post Request in handle_UpdateProfile:");
    udp_printf("%s", body.c_str());

    JsonDocument bodyJSON;
    DeserializationError error = deserializeJson(bodyJSON, body);
    if (error) {
        udp_printf("JSON parse error: %s", error.c_str());
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}\n");
        return;
    }

    int ProfileIndex = bodyJSON["ProfileIndex"];
    if (ProfileIndex < 0 || ProfileIndex > (int)GlobalParameters.NumberOfActiveProfiles) {
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
      udp_printf(" - Deleting profile at index %d", ProfileIndex);
      for(int i = ProfileIndex; i < (int)GlobalParameters.NumberOfActiveProfiles - 1; i++){
        GlobalParameters.RippleProfiles[i] = GlobalParameters.RippleProfiles[i + 1];
      }
      GlobalParameters.NumberOfActiveProfiles--;
      udp_printf(" - Profile deleted. Total number of active profiles is now %d", GlobalParameters.NumberOfActiveProfiles);
      EEPROM_MarkDirty();
      request->send(200, "application/json", "{}");
      return;
    }

    if(ProfileIndex >= (int)GlobalParameters.NumberOfActiveProfiles){
      GlobalParameters.NumberOfActiveProfiles = ProfileIndex + 1;
      udp_printf("Creating new profile at index %d. Total number of active profiles is now %d", ProfileIndex, GlobalParameters.NumberOfActiveProfiles);
      setupDefaultProfileParameters(&GlobalParameters.RippleProfiles[ProfileIndex]);
    } else {
      udp_printf("Updating existing profile with name \"%s\" at index %d", GlobalParameters.RippleProfiles[ProfileIndex].ProfileName, ProfileIndex);
    }

    xSemaphoreTake(gParamsMutex, portMAX_DELAY);
    RippleProfile_struct* profile = &GlobalParameters.RippleProfiles[ProfileIndex];

    /* Update profile name */
    if(bodyJSON.containsKey("ProfileName")){
      String nameStr = bodyJSON["ProfileName"].as<String>();
      strncpy(profile->ProfileName, nameStr.c_str(), sizeof(profile->ProfileName) - 1);
      profile->ProfileName[sizeof(profile->ProfileName) - 1] = '\0';
      udp_printf(" - Updated ProfileName to \"%s\"", profile->ProfileName);
    }

    /* Update Active */
    if(bodyJSON.containsKey("Active")){
      profile->Active = bodyJSON["Active"].as<bool>();
      udp_printf(" - Updated Active to %d", profile->Active);
    }

    /* Update ProfilePeriod_ms */
    if(bodyJSON.containsKey("ProfilePeriod_ms")){
      profile->ProfilePeriod_ms = bodyJSON["ProfilePeriod_ms"].as<unsigned long>();
      if(profile->ProfilePeriod_ms < PROFILEPERIOD_MIN) profile->ProfilePeriod_ms = PROFILEPERIOD_MIN;
      if(profile->ProfilePeriod_ms > PROFILEPERIOD_MAX) profile->ProfilePeriod_ms = PROFILEPERIOD_MAX;
      udp_printf(" - Updated ProfilePeriod_ms to %lu", profile->ProfilePeriod_ms);
    }

    /* Update Colors */
    if(bodyJSON.containsKey("Colors")){
      JsonArray colorsArray = bodyJSON["Colors"].as<JsonArray>();
      unsigned int numColors = colorsArray.size();
      if(numColors > 0 && numColors <= 16){
        profile->NumberOfColors = numColors;
        for(unsigned int i = 0; i < numColors; i++){
          String colorStr = colorsArray[i] | String("#000000");
          unsigned int rgbValue = (unsigned int) strtol(colorStr.c_str() + 1, NULL, 16);
          profile->Colors[i] = rgbHexToHue16(rgbValue);
        }
        profile->CurrentColor = 0;
        udp_printf(" - Updated %d colors", numColors);
      }
    }
    if(bodyJSON.containsKey("NumberOfColors")){
      unsigned int nc = bodyJSON["NumberOfColors"].as<unsigned int>();
      if(nc > 0 && nc <= 16) profile->NumberOfColors = nc;
    }

    /* Update Events array */
    if(bodyJSON.containsKey("Events")){
      JsonArray eventsArray = bodyJSON["Events"].as<JsonArray>();
      for(int e = 0; e < MAX_EVENTS_PER_PROFILE && e < (int)eventsArray.size(); e++){
        JsonObject evtJson = eventsArray[e].as<JsonObject>();
        TimeEvent_struct* evt = &profile->Events[e];

        evt->Enabled = evtJson.containsKey("Enabled") ? evtJson["Enabled"].as<bool>() : false;
        if(!evt->Enabled) continue; /* skip parsing disabled events */

        if(evtJson.containsKey("TimeOffset_ms")) evt->TimeOffset_ms = evtJson["TimeOffset_ms"].as<unsigned short>();
        if(evtJson.containsKey("RippleLifeSpan")) evt->RippleLifeSpan = evtJson["RippleLifeSpan"].as<unsigned long>();
        if(evtJson.containsKey("RippleType")) evt->RippleType = evtJson["RippleType"].as<unsigned char>();
        if(evtJson.containsKey("Behavior")) evt->Behavior = static_cast<rippleBehavior>(evtJson["Behavior"].as<int>());
        if(evtJson.containsKey("RippleSpeed")) evt->RippleSpeed = evtJson["RippleSpeed"].as<float>();
        if(evtJson.containsKey("RainbowDeltaPerTick")) evt->RainbowDeltaPerTick = evtJson["RainbowDeltaPerTick"].as<short>();
        if(evtJson.containsKey("Direction")) evt->Direction = evtJson["Direction"].as<signed char>();
        if(evtJson.containsKey("ActiveNodes")){
          JsonArray nodesArray = evtJson["ActiveNodes"].as<JsonArray>();
          for(int j = 0; j < NUMBER_OF_NODES && j < (int)nodesArray.size(); j++){
            evt->ActiveNodes[j] = nodesArray[j].as<int>();
          }
        }
        udp_printf(" - Event[%d]: Enabled=%d Type=%d Offset=%d Speed=%.2f", e, evt->Enabled, evt->RippleType, evt->TimeOffset_ms, evt->RippleSpeed);
      }
      /* Disable any events not provided in the JSON */
      for(int e = eventsArray.size(); e < MAX_EVENTS_PER_PROFILE; e++){
        profile->Events[e].Enabled = false;
      }
    }

    /* Defer ripple kill to the main loop (Core 1) so we never call into
       ripples[] from the HTTP task (Core 0) while Ripple_MainFunction is
       running on Core 1 — that cross-core race caused the abort(). */
    pendingKillProfileIndex = ProfileIndex;
    profile->PeriodStartTime_ms = millis();
    memset(profile->EventFired, 0, sizeof(profile->EventFired));
    profile->CurrentColor = 0;
    udp_printf(" - Reset period timer for profile %d", ProfileIndex);

    xSemaphoreGive(gParamsMutex);

    EEPROM_MarkDirty();
    request->send(200, "application/json", "{}");
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

    JsonDocument bodyJSON;
    DeserializationError error = deserializeJson(bodyJSON, body);
    if (error) {
        udp_printf("JSON parse error: %s", error.c_str());
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}\n");
        return;
    }
    xSemaphoreTake(gParamsMutex, portMAX_DELAY);
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
    if (bodyJSON.containsKey("Brightness")) {
        GlobalParameters.Brightness = bodyJSON["Brightness"].as<unsigned char>();
        for (int i = 0; i < NUMBER_OF_STRIPS; i++) {
            strips[i].setBrightness(GlobalParameters.Brightness);
        }
        udp_printf(" - Updated Brightness to %d", GlobalParameters.Brightness);
    } else {
        udp_printf(" - Brightness remains %d", GlobalParameters.Brightness);
    }
    if (bodyJSON.containsKey("SequencerEnabled")) {
        boolean wasEnabled = GlobalParameters.SequencerEnabled;
        GlobalParameters.SequencerEnabled = bodyJSON["SequencerEnabled"].as<bool>();
        if (!wasEnabled && GlobalParameters.SequencerEnabled) {
            // Transitioning from off to on: reset to profile 0
            GlobalParameters.SequencerCurrentProfile = 0;
            GlobalParameters.SequencerLastSwitch_ms = millis();
        }
        udp_printf(" - Updated SequencerEnabled to %d", GlobalParameters.SequencerEnabled);
    }
    if (bodyJSON.containsKey("SequencerMode")) {
        GlobalParameters.SequencerMode = bodyJSON["SequencerMode"].as<unsigned char>();
        udp_printf(" - Updated SequencerMode to %d", GlobalParameters.SequencerMode);
    }
    if (bodyJSON.containsKey("SequencerDwellTime_s")) {
        unsigned short dwell = bodyJSON["SequencerDwellTime_s"].as<unsigned short>();
        if (dwell >= SEQUENCER_DWELL_MIN && dwell <= SEQUENCER_DWELL_MAX) {
            GlobalParameters.SequencerDwellTime_s = dwell;
        }
        udp_printf(" - Updated SequencerDwellTime_s to %d", GlobalParameters.SequencerDwellTime_s);
    }
    if (bodyJSON.containsKey("GlobalBPM")) {
        float bpm = bodyJSON["GlobalBPM"].as<float>();
        if (bpm >= BPM_MIN && bpm <= BPM_MAX) {
            GlobalParameters.GlobalBPM = bpm;
        }
        udp_printf(" - Updated GlobalBPM to %.1f", GlobalParameters.GlobalBPM);
    }
    if (bodyJSON.containsKey("StableColorMode")) {
        bool newMode = bodyJSON["StableColorMode"].as<bool>();
        if (newMode && !GlobalParameters.StableColorMode) {
            /* Entering stable mode from ripple: kill all ripples */
            pendingKillProfileIndex = -2;
            GlobalParameters.MasterFireRippleEnabled = false;
        }
        GlobalParameters.StableColorMode = newMode;
        udp_printf(" - Updated StableColorMode to %d", GlobalParameters.StableColorMode);
    }
    if (bodyJSON.containsKey("StableColorHue")) {
        const char* colorStr = bodyJSON["StableColorHue"].as<const char*>();
        if (colorStr && colorStr[0] == '#') {
            unsigned int rgbValue = (unsigned int)strtol(colorStr + 1, NULL, 16);
            GlobalParameters.StableColorHue = rgbHexToHue16(rgbValue);
        }
        udp_printf(" - Updated StableColorHue to %d", GlobalParameters.StableColorHue);
    }
    if (bodyJSON.containsKey("PulseFrequency")) {
        float f = bodyJSON["PulseFrequency"].as<float>();
        GlobalParameters.PulseFrequency = constrain(f, 0.1f, 5.0f);
        udp_printf(" - Updated PulseFrequency to %.2f", GlobalParameters.PulseFrequency);
    }
    if (bodyJSON.containsKey("PulseDepth")) {
        float d = bodyJSON["PulseDepth"].as<float>();
        GlobalParameters.PulseDepth = constrain(d, 0.0f, 1.0f);
        udp_printf(" - Updated PulseDepth to %.2f", GlobalParameters.PulseDepth);
    }
    if (bodyJSON.containsKey("StableColorSegments")) {
        JsonArray segsArr = bodyJSON["StableColorSegments"].as<JsonArray>();
        for (int i = 0; i < NUMBER_OF_SEGMENTS && i < (int)segsArr.size(); i++) {
            GlobalParameters.StableColorSegments[i] = segsArr[i].as<bool>();
        }
        udp_printf(" - Updated StableColorSegments");
    }

    xSemaphoreGive(gParamsMutex);
    EEPROM_MarkDirty(); // schedule debounced save to persist global parameter changes
    request->send(200, "application/json", "{}");
}

void handle_ManualRipple(AsyncWebServerRequest *request) {
  udp_printf("Received manual ripple request");
  manualFireRipple = 1;
  request->send(LittleFS, "/oneindex.html", String(), false, nullptr);
}

/* checkbox handling */
void handle_MasterFireRippleEnabled_On(AsyncWebServerRequest *request) {
  udp_printf("Master Automatic ripples: ON");
  xSemaphoreTake(gParamsMutex, portMAX_DELAY);
  GlobalParameters.MasterFireRippleEnabled = 1;
  xSemaphoreGive(gParamsMutex);
}

void handle_MasterFireRippleEnabled_Off(AsyncWebServerRequest *request) {
  udp_printf("Master Automatic ripples: OFF");
  xSemaphoreTake(gParamsMutex, portMAX_DELAY);
  GlobalParameters.MasterFireRippleEnabled = 0;
  xSemaphoreGive(gParamsMutex);
}
/* end of checkbox handling*/

/* to be called once at startup */
void HTTP_backend_init(void){
  gParamsMutex = xSemaphoreCreateMutex();

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
