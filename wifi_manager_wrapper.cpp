/* This file intentionally includes WiFiManager.h in isolation.
   WiFiManager pulls in WebServer.h which defines HTTP_GET etc. as C enum
   members. ESPAsyncWebServer defines the same names in a different enum.
   Keeping them in separate translation units avoids the conflict. */
#include <WiFiManager.h>
#include "wifi_manager_wrapper.h"

bool wifiManagerConnect(const char* apName)
{
    WiFiManager wm;
    return wm.autoConnect(apName);
}

void wifiManagerResetSettings(void)
{
    WiFiManager wm;
    wm.resetSettings();
}
