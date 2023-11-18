#pragma once

#include <vector>
#include "ESPAsyncWebServer.h"
#include "UPnP.h"
#include <ArduinoJson.h>

extern UPnP upnp; 

//#define DEBUG_HUE                Serial
#define DEBUG_MSG_HUE(fmt, ...) { char buf[1024]; snprintf(buf, sizeof(buf), (fmt), ## __VA_ARGS__); UDP_SendPacket(buf); }
#ifdef DEBUG_HUE
    #if defined(ARDUINO_ARCH_ESP32)
        #define DEBUG_MSG_HUE(fmt, ...) { DEBUG_HUE.printf_P((PGM_P) PSTR(fmt), ## __VA_ARGS__); }
    #else
        #error Platform not supported
    #endif
#else
    //#define DEBUG_MSG_HUE(...)
#endif


typedef struct {
    char * name;
    bool state;
    unsigned char bri;
    char uniqueid[28];
    short ct;
    unsigned int hue;
    unsigned char sat;
    char mode; 

} device_t;

typedef std::function<void(unsigned char, bool, unsigned char, short, unsigned int, unsigned char, char)> TSetStateCallback;

class HueBridge
{
    public:
        unsigned char addDevice(const char * device_name);
        void start();
        void handle();

        void onSetState(TSetStateCallback fn) { _setCallback = fn; }
        void setState(unsigned char id, bool state, unsigned char bri, short ct, unsigned int hue, unsigned char sat, char mode);
        HueBridge(uint16_t port) : webServer(port) {}
        AsyncWebServer webServer; 
    
    private:
        TSetStateCallback _setCallback = NULL;

};

String deviceJson(unsigned char id);
void handle_GetDescription(AsyncWebServerRequest *request);
void handle_PostDeviceType(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
void handle_GetState(AsyncWebServerRequest *request); 
void handle_PutState(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
void handle_root(AsyncWebServerRequest *request);
void handle_clip(AsyncWebServerRequest *request);
void handle_CORSPreflight(AsyncWebServerRequest *request);
void handle_NotFound(AsyncWebServerRequest *request);