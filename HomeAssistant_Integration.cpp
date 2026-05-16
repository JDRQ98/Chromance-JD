#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoHA.h>

#include "HTTP_Server.h"   // To access GlobalParameters
#include "MCAL/ripple.h"   // To access strips variables
#include "MCAL/EEP.h"      // To access EEPROM dirty marking

// --- MQTT and HA objects ---
WiFiClient client;
HADevice device("chromance_hexagon"); // Unique ID for HA
HAMqtt mqttClient(client, device);

// Create a Light Entity with Brightness and RGB Features
HALight haLight("hexagon_light", HALight::BrightnessFeature | HALight::RGBFeature);

// External references to your existing system variables
extern GlobalParameters_struct GlobalParameters;
extern SemaphoreHandle_t gParamsMutex;

// Helper function to convert RGB to Hue16 (Local to this file)
static unsigned int rgbHexToHue16(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t maxC = max(r, max(g, b));
  uint8_t minC = min(r, min(g, b));
  if (maxC == minC) return 0;
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
  return (unsigned int)(hue * 65536.0f / 6.0f);
}

// --- HOME ASSISTANT CALLBACKS ---

// 1. Toggle on/off callback
void onStateCommand(bool state, HALight* sender) {
    xSemaphoreTake(gParamsMutex, portMAX_DELAY);
    
    GlobalParameters.MasterFireRippleEnabled = state;
    
    xSemaphoreGive(gParamsMutex);
    EEPROM_MarkDirty(); 

    sender->setState(state); // Report back to Alexa/HA success
}

// 2. Brightness change callback
void onBrightnessCommand(uint8_t brightness, HALight* sender) {
    xSemaphoreTake(gParamsMutex, portMAX_DELAY);

    GlobalParameters.Brightness = brightness;
    for (int i = 0; i < NUMBER_OF_STRIPS; i++) {
        strips[i].setBrightness(GlobalParameters.Brightness);
    }

    xSemaphoreGive(gParamsMutex);
    EEPROM_MarkDirty();

    sender->setBrightness(brightness); // Report back to Alexa/HA
}

// 3. Color change callback
void onRGBColorCommand(HALight::RGBColor color, HALight* sender) {
    xSemaphoreTake(gParamsMutex, portMAX_DELAY);

    // If Alexa sets a color, we switch to Stable Color mode
    GlobalParameters.StableColorMode = true; 
    GlobalParameters.StableColorHue = rgbHexToHue16(color.red, color.green, color.blue);
    
    // Optional: automatically turn the lights on if a color is chosen while they are off
    GlobalParameters.MasterFireRippleEnabled = true;

    xSemaphoreGive(gParamsMutex);
    EEPROM_MarkDirty();

    sender->setRGBColor(color); // Report back to Alexa/HA
    sender->setState(true);     // Confirm they are ON
}


void HA_init() {
    // Device definition for Home Assistant
    device.setName("Hexagono");
    device.setSoftwareVersion("2.0.0");
    device.setManufacturer("JD Software Labs");
    device.setModel("ESP32 Chromance");

    // Configure the light entity callbacks
    haLight.setName("Hexagono");
    haLight.onStateCommand(onStateCommand);
    haLight.onBrightnessCommand(onBrightnessCommand);
    haLight.onRGBColorCommand(onRGBColorCommand);

    // Connect to MQTT (CHANGE THE IP, USER, and PASS HERE)
    mqttClient.begin("192.168.100.201", 1883, "mqtt_hexagono", "hexagono123");
    Serial.println("[HA] Home Assistant MQTT discovery initialized.");
}

void HA_loop() {
    mqttClient.loop();
}