#ifndef WIFI_UTILITIES_H
#define WIFI_UTILITIES_H

//#define ELEGANTOTA_DEBUG 1

#include "Arduino.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include "LittleFS.h"
#include "wifi_manager_wrapper.h"


/* Configuration */
#define UDP_SERVER_IP "192.168.100.67" // Replace with the IP of your UDP listener (e.g., your computer)
#define UDP_PRINT_ENABLED 1
#define UDP_MAX_RETRIES 1    // Number of retries
#define UDP_RETRY_DELAY 5  // Delay in milliseconds between retries
const int udpPort = 8888;                    // Replace with the desired UDP port

extern const char *udpServerIP;
extern AsyncWebServer server;

/*APIs*/
void WiFi_Utilities_init(void);
void WiFi_Utilities_loop(void);

size_t udp_printf(const char *format, ...);
size_t udp_println(const String &msg);
size_t udp_println(const char *msg);
size_t udp_println(const char msg);
size_t udp_println(int val);
size_t udp_println(long val);

// Route HueBridge debug output through UDP so it appears in UDP_Monitor.py
#define DEBUG_MSG_HUE(fmt, ...) { udp_printf(fmt, ## __VA_ARGS__); }

#endif /*WIFI_UTILITIES_H*/