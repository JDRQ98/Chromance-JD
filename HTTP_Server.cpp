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

void setupDefaultProfileParameters(void) {
  /* set up default profile parameters in GlobalParameters struct */
  GlobalParameters.MasterFireRippleEnabled = 1;
  GlobalParameters.NumberOfActiveProfiles = 1;
  GlobalParameters.Decay = DECAY_DEFAULT; /* decay per tick, global for now TODO: make decay a ripple/profile property */
  GlobalParameters.RippleProfiles[0].Active = 1;
  GlobalParameters.RippleProfiles[0].Behavior = BEHAVIOR_DEFAULT;
  GlobalParameters.RippleProfiles[0].Direction = DIRECTION_DEFAULT; /* all directions */ 
  GlobalParameters.RippleProfiles[0].DelayBetweenRipples_ms = DELAYBETWEENRIPPLES_DEFAULT;
  
  GlobalParameters.RippleProfiles[0].RippleLifeSpan = RIPPLELIFESPAN_DEFAULT;
  GlobalParameters.RippleProfiles[0].RippleSpeed = RIPPLESPEED_DEFAULT;
  GlobalParameters.RippleProfiles[0].RainbowDeltaPerTick = RAINBOWDELTAPERTICK_DEFAULT;
  GlobalParameters.RippleProfiles[0].NumberOfColors = NUMBEROFCOLORS_DEFAULT;
  for (int i = 0; i < GlobalParameters.RippleProfiles[0].NumberOfColors; i++)
  {
    GlobalParameters.RippleProfiles[0].Colors[0] = 0xFF0000; /* red */
    GlobalParameters.RippleProfiles[0].Colors[1] = 0xFF7F00; /* orange */
    GlobalParameters.RippleProfiles[0].Colors[2] = 0xFFFF00; /* yellow */
    GlobalParameters.RippleProfiles[0].Colors[3] = 0x00FF00; /* green */
    GlobalParameters.RippleProfiles[0].Colors[4] = 0x0000FF; /* blue */
    GlobalParameters.RippleProfiles[0].Colors[5] = 0x4B0082; /* indigo */
    GlobalParameters.RippleProfiles[0].Colors[6] = 0x8B00FF; /* violet */

  }
  GlobalParameters.RippleProfiles[0].CurrentColor = 0;
  GlobalParameters.RippleProfiles[0].TimeLastRippleFired_ms = 0;
  GlobalParameters.RippleProfiles[0].ActiveNodes[starburstNode] = 1; /* activate profile on starburst node */
  GlobalParameters.RippleProfiles[0].ProfileName = "Rainbow 7\0"; /* name of this profile */
}

/* HANDLER FUNCTIONS */
void handle_getInternalVariables(AsyncWebServerRequest *request) {
  String response = "{\n";
//   response += "  \"currentStartingNode\": " + String(GlobalParameters.currentStartingNode) + " ,\n";
// // Enter the desired behavior:
//   response += "  \"currentBehavior\": " + String(GlobalParameters.currentBehavior) + " ,\n";
// // Enter ripple direction:
//   response += "  \"currentDirection\": " + String(GlobalParameters.currentDirection) + " ,\n";
// // Enter delay between ripples in ms [1 - 60000]:
//   response += "  \"currentDelayBetweenRipples\": " + String(GlobalParameters.currentDelayBetweenRipples) + " ,\n";
//   response += "  \"currentDelayBetweenRipplesMin\": " + String(HTTP_CURRENTDELAYBETWEENRIPPLES_MIN) + " ,\n";
//   response += "  \"currentDelayBetweenRipplesMax\": " + String(HTTP_CURRENTDELAYBETWEENRIPPLES_MAX) + " ,\n";
// // Enter ripple life span in ms [1 - 60000]:
//   response += "  \"currentRippleLifeSpan\": " + String(GlobalParameters.currentRippleLifeSpan) + " ,\n";
//   response += "  \"currentRippleLifeSpanMin\": " + String(HTTP_CURRENTRIPPLELIFESPAN_MIN) + " ,\n";
//   response += "  \"currentRippleLifeSpanMax\": " + String(HTTP_CURRENTRIPPLELIFESPAN_MAX) + " ,\n";
// // Enter ripple speed [0.01 - 5]:
//   response += "  \"currentRippleSpeed\": " + String(GlobalParameters.currentRippleSpeed) + " ,\n";
//   response += "  \"currentRippleSpeedMin\": " + String(HTTP_CURRENTRIPPLESPEED_MIN) + " ,\n";
//   response += "  \"currentRippleSpeedMax\": " + String(HTTP_CURRENTRIPPLESPEED_MAX) + " ,\n";
// // Enter decay per tick [0.90 - 1]:
//   response += "  \"currentDecay\": " + String(GlobalParameters.currentDecay) + " ,\n";
//   response += "  \"currentDecayMin\": " + String(HTTP_CURRENTDECAY_MIN) + " ,\n";
//   response += "  \"currentDecayMax\": " + String(HTTP_CURRENTDECAY_MAX) + " ,\n";
// // Starting Color:
//   response += "  \"currentColor\": " + String(GlobalParameters.currentColor) + " ,\n";
// // Enter hue delta per period [0 - 60000]:
//   response += "  \"currentRainbowDeltaPerPeriod\": " + String(GlobalParameters.currentRainbowDeltaPerPeriod) + " ,\n";
//   response += "  \"currentRainbowDeltaPerPeriodMin\": " + String(HTTP_CURRENTRAINBOWDELTAPERPERIOD_MIN) + " ,\n";
//   response += "  \"currentRainbowDeltaPerPeriodMax\": " + String(HTTP_CURRENTRAINBOWDELTAPERPERIOD_MAX) + " ,\n";
// // Enter hue delta per tick [0 - 500]:
//   response += "  \"currentRainbowDeltaPerTick\": " + String(GlobalParameters.currentRainbowDeltaPerTick) + " ,\n";
//   response += "  \"currentRainbowDeltaPerTickMin\": " + String(HTTP_CURRENTRAINBOWDELTAPERTICK_MIN) + " ,\n";
//   response += "  \"currentRainbowDeltaPerTickMax\": " + String(HTTP_CURRENTRAINBOWDELTAPERTICK_MAX) + " ,\n";
// // Enter the desired number of colors [1 - 64]:
//   response += "  \"currentNumberofColors\": " + String(GlobalParameters.currentNumberofColors) + " ,\n";
//   response += "  \"currentNumberofColorsMin\": " + String(HTTP_CURRENTNUMBEROFCOLORS_MIN) + " ,\n";
//   response += "  \"currentNumberofColorsMax\": " + String(HTTP_CURRENTNUMBEROFCOLORS_MAX) + " \n";
//   response += "}";
  
  const int length = response.length(); 
  // declaring character array (+1 for null terminator) 
  char* char_array = new char[length + 1]; 
  strcpy(char_array, response.c_str()); 
  request->send_P(200, "application/json", char_array);
  delete[] char_array;
}


void handle_UpdateInternalVariables_body(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    String body = String((char*)data, len);
    udp_printf("Received the following contents via HTTP Post Request in handle_UpdateInternalVariables:");
    udp_printf("%s", body.c_str());

    DynamicJsonDocument bodyJSON(1024);
    DeserializationError error = deserializeJson(bodyJSON, body);
    if (error) {
        udp_printf("JSON parse error: %s", error.c_str());
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}\n");
        return;
    }


    // short DelayBetweenRipples = bodyJSON.containsKey("currentDelayBetweenRipples") ? bodyJSON["currentDelayBetweenRipples"] : GlobalParameters.currentDelayBetweenRipples;
    // short RainbowDeltaPerTick = bodyJSON.containsKey("currentRainbowDeltaPerTick") ? bodyJSON["currentRainbowDeltaPerTick"] : GlobalParameters.currentRainbowDeltaPerTick;
    // unsigned long RippleLifeSpan = bodyJSON.containsKey("currentRippleLifeSpan") ? bodyJSON["currentRippleLifeSpan"] : GlobalParameters.currentRippleLifeSpan;
    // float RippleSpeed = bodyJSON.containsKey("currentRippleSpeed") ? (float) bodyJSON["currentRippleSpeed"] : GlobalParameters.currentRippleSpeed;
    // RippleSpeed = RippleSpeed/100; //scaling for proper display on HTML webpage
    // int NumberofColors = bodyJSON.containsKey("currentNumberofColors") ? bodyJSON["currentNumberofColors"] : GlobalParameters.currentNumberofColors;
    // int Behavior = bodyJSON.containsKey("currentBehavior") ? bodyJSON["currentBehavior"] : GlobalParameters.currentBehavior;
    // int Direction = bodyJSON.containsKey("currentDirection") ? bodyJSON["currentDirection"] : GlobalParameters.currentDirection;
    // float Decay = bodyJSON.containsKey("currentDecay") ? (float) bodyJSON["currentDecay"] : GlobalParameters.currentDecay;
    // Decay = Decay/1000; //scaling for proper display on HTML webpage

    // if(DelayBetweenRipples != GlobalParameters.currentDelayBetweenRipples){
    //   udp_printf("received new DelayBetweenRipples from POST request: %d", DelayBetweenRipples);
    //   if(DelayBetweenRipples >= HTTP_CURRENTDELAYBETWEENRIPPLES_MIN && DelayBetweenRipples <= HTTP_CURRENTDELAYBETWEENRIPPLES_MAX){ /* new value received */
    //     udp_printf("New value accepted. Previous value: %d", GlobalParameters.currentDelayBetweenRipples);
    //     GlobalParameters.currentDelayBetweenRipples = DelayBetweenRipples;
    //   } else {
    //     udp_printf("New DelayBetweenRipples not valid; discarded.");
    //   }
    // }

    // if(RainbowDeltaPerTick != GlobalParameters.currentRainbowDeltaPerTick){
    //   udp_printf("received new RainbowDeltaPerTick from POST request: %d", RainbowDeltaPerTick);
    //   if(RainbowDeltaPerTick >= HTTP_CURRENTRAINBOWDELTAPERTICK_MIN && RainbowDeltaPerTick <= HTTP_CURRENTRAINBOWDELTAPERTICK_MAX){ /* new value received */
    //     udp_printf("New value accepted. Previous value: %d", GlobalParameters.currentRainbowDeltaPerTick);
    //     GlobalParameters.currentRainbowDeltaPerTick = RainbowDeltaPerTick;
    //   } else {
    //     udp_printf("New RainbowDeltaPerTick not valid; discarded.");
    //   }
    // }

    // if(RippleLifeSpan != GlobalParameters.currentRippleLifeSpan){
    //   udp_printf("received new RippleLifeSpan from POST request: %d", RippleLifeSpan);
    //   if(RippleLifeSpan >= HTTP_CURRENTRIPPLELIFESPAN_MIN && RippleLifeSpan <= HTTP_CURRENTRIPPLELIFESPAN_MAX){ /* new value received */
    //     udp_printf("New value accepted. Previous value: %d", GlobalParameters.currentRippleLifeSpan);
    //     GlobalParameters.currentRippleLifeSpan = RippleLifeSpan;
    //   } else {
    //     udp_printf("New RippleLifeSpan not valid; discarded.");
    //   }
    // }

    // if(RippleSpeed != GlobalParameters.currentRippleSpeed){
    //   udp_printf("received new Ripple Speed from POST request: %.2f", RippleSpeed);
    //   if(RippleSpeed >= HTTP_CURRENTRIPPLESPEED_MIN && RippleSpeed <= HTTP_CURRENTRIPPLESPEED_MAX){ /* new value received */
    //     udp_printf("New value accepted. Previous value: %.2f", GlobalParameters.currentRippleSpeed);
    //     GlobalParameters.currentRippleSpeed = RippleSpeed;
    //   } else {
    //     udp_printf("New Ripple Speed not valid; discarded.");
    //   }
    // }

    // if(NumberofColors != GlobalParameters.currentNumberofColors){
    //   udp_printf("received new NumberofColors from POST request: %d", NumberofColors);
    //   if(NumberofColors >= HTTP_CURRENTNUMBEROFCOLORS_MIN && NumberofColors <= HTTP_CURRENTNUMBEROFCOLORS_MAX){ /* new value received */
    //     udp_printf("New value accepted. Previous value: %d", GlobalParameters.currentNumberofColors);
    //     GlobalParameters.currentNumberofColors = NumberofColors;
    //   } else {
    //     udp_printf("New NumberofColors not valid; discarded.");
    //   }
    // }

    // if(Behavior != GlobalParameters.currentBehavior){
    //   udp_printf("received new Behavior from POST request: %d", Behavior);
    //   if(Behavior >= 0 && Behavior <= 4){ /* new value received */
    //     udp_printf("New value accepted. Previous value: %d", GlobalParameters.currentBehavior);
    //     GlobalParameters.currentBehavior = Behavior;
    //   } else {
    //     udp_printf("New Behavior not valid; discarded.");
    //   }
    // }

    // if(Direction != GlobalParameters.currentDirection){
    //   udp_printf("received new Direction from POST request: %d", Direction);
    //   if(Direction >= -1 && Direction <= 6){ /* new value received */
    //     udp_printf("New value accepted. Previous value: %d", GlobalParameters.currentDirection);
    //     GlobalParameters.currentDirection = Direction;
    //   } else {
    //     udp_printf("New Direction not valid; discarded.");
    //   }
    // }

    // if(Decay != GlobalParameters.currentDecay){
    //   udp_printf("received new Decay factor from POST request: %.3f", Decay);
    //   if(Decay >= HTTP_CURRENTDECAY_MIN && Decay <= HTTP_CURRENTDECAY_MAX){ /* new value received */
    //     udp_printf("New value accepted. Previous value: %.3f", GlobalParameters.currentDecay);
    //     GlobalParameters.currentDecay = Decay;
    //   } else {
    //     udp_printf("New Decay not valid; discarded.");
    //   }
    // }

    request->send_P(200, "application/json", "{}");
}


void handle_ManualRipple(AsyncWebServerRequest *request) {
  udp_printf("Received manual ripple request");
  manualFireRipple = 1;
  request->send(SPIFFS, "/oneindex.html", String(), false, nullptr);
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
  server.on("/getInternalVariables", handle_getInternalVariables); 
  server.on("/updateInternalVariables", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, handle_UpdateInternalVariables_body);
}
