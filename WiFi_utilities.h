#ifndef WIFI_UTILITIES_H
#define WIFI_UTILITIES_H

#include "Arduino.h"
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ElegantOTA.h>
#include "SPIFFS.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include "Alexa/HueBridge.h"

/*activate these defines to get additional debugging information foe HueBridge component (Alexa)*/
//#define DEBUG_UPnP               Serial
//#define DEBUG_HUE                Serial

/* Configuration */
#define UDP_SERVER_IP "192.168.100.47" // Replace with the IP of your UDP listener (e.g., your computer)
#define UDP_PRINT_ENABLED 1
#define UDP_MAX_RETRIES 1    // Number of retries
#define UDP_RETRY_DELAY 5  // Delay in milliseconds between retries
const int udpPort = 8888;                    // Replace with the desired UDP port

#define ALEXA_DEVICE_NAME "hexagono prueba"

extern const char *udpServerIP;
extern AsyncWebServer server;


/*callbacks provided by application */
extern void handle_SetState(unsigned char id, bool state, unsigned char bri, short ct, unsigned int hue, unsigned char sat, char mode);

/*APIs*/
void WiFi_Utilities_init(void);
void WiFi_Utilities_loop(void);

size_t udp_println(const String &msg);
size_t udp_println(const char *msg);
size_t udp_println(const char msg);
size_t udp_println(int val);
size_t udp_println(long val);

#ifdef DEBUG_HUE
    #if defined(ARDUINO_ARCH_ESP32)
        #define DEBUG_MSG_HUE(fmt, ...) { DEBUG_HUE.printf_P((PGM_P) PSTR(fmt), ## __VA_ARGS__); }
    #else
        #error Platform not supported
    #endif
#else
    #define DEBUG_MSG_HUE(...)
#endif

#endif /*WIFI_UTILITIES_H*/