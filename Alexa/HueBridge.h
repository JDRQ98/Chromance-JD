#ifndef HUE_BRIDGE_H
#define HUE_BRIDGE_H

#include <vector>
#include <ESPAsyncWebServer.h>
#include "UPnP.h"
#include "WiFi_utilities.h"


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
        void start(AsyncWebServer& server);
        void handle(); // drives UPnP discovery polling

        void onSetState(TSetStateCallback fn) { _setCallback = fn; }
        void setState(unsigned char id, bool state, unsigned char bri, short ct, unsigned int hue, unsigned char sat, char mode);

        /* Called from the onNotFound handler for any /api/* URL that doesn't
           match a registered route (e.g. different username, or /api/{user}) */
        void handleApiRequest(AsyncWebServerRequest* request);

    private:
        void handle_GetDescription(AsyncWebServerRequest* request);
        void handle_PostDeviceType(AsyncWebServerRequest* request);
        void handle_GetState(AsyncWebServerRequest* request);
        void handle_GetBridgeInfo(AsyncWebServerRequest* request);
        String deviceJson(unsigned char id);
        void handle_PutState(AsyncWebServerRequest* request, uint8_t* data, size_t len);
        void handle_clip(AsyncWebServerRequest* request);

        std::vector<device_t> lights;
        UPnP upnp;
        TSetStateCallback _setCallback = NULL;
};

#endif /*HUE_BRIDGE_H*/
