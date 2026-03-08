#ifndef WIFI_MANAGER_WRAPPER_H
#define WIFI_MANAGER_WRAPPER_H

/* Isolated wrapper so WiFiManager.h (which pulls in WebServer.h) is only
   compiled in wifi_manager_wrapper.cpp, not in any TU that also needs
   ESPAsyncWebServer.h. Both libraries define HTTP_GET etc. as global enum
   members and cannot coexist in the same translation unit. */

bool wifiManagerConnect(const char* apName);
void wifiManagerResetSettings(void);

#endif /* WIFI_MANAGER_WRAPPER_H */
