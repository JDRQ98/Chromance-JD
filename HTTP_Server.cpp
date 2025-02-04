#include <WiFi.h>
#include <WebServer.h>
#include <string>  
using namespace std;

#include "HTTP_Server.h" 
#include "MCAL/ripple.h"
#include "MCAL/EEP.h"
#include "Alexa/SimpleJson.h"


// WiFi stuff - CHANGE FOR YOUR OWN NETWORK!
const char* ssid = "TP-Link-150";
const char* password = "Cenote#150";

const IPAddress ip(192, 168, 1, 241);  // IP address that THIS DEVICE should request
const IPAddress gateway(192, 168, 1, 250);  // Your router
const IPAddress subnet(255, 255, 255, 0);  // Your subnet mask (find it from your router's admin panel)

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


void handle_ManualRipple(AsyncWebServerRequest *request) {
  DEBUG_MSG_HUE("Received manual ripple request");
  manualFireRipple = 1;
  request->send(SPIFFS, "/oneindex.html", String(), false, nullptr);
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
  server.on("/ManualRipple", handle_ManualRipple);
  server.on("/MasterFireRippleEnabled/on", handle_MasterFireRippleEnabled_On);
  server.on("/MasterFireRippleEnabled/off", handle_MasterFireRippleEnabled_Off);
  server.on("/getInternalVariables", handle_getInternalVariables); 
  //server.on("/updateInternalVariables", HTTP_POST, handle_SendDashboard, nullptr, handle_UpdateInternalVariables); 
  
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
