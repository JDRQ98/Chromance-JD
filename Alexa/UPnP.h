
#ifndef UPNP_H
#define UPNP_H

#define UPnP_UDP_MULTICAST_IP     IPAddress(239,255,255,250)
#define UPnP_UDP_MULTICAST_PORT   1900
#define UPnP_TCP_PORT             80

// Route UPnP debug output through UDP so it appears in UDP_Monitor.py
// udp_printf is pulled in via WiFi_utilities.h in UPnP.cpp
#define DEBUG_MSG_UPnP(fmt, ...) { udp_printf(fmt, ## __VA_ARGS__); }

#if defined(ESP32)
    #include <WiFi.h>
#else
    #error Platform not supported
#endif

#include <WiFiUdp.h>
#include "templates.h"

/* ST is always "urn:schemas-upnp-org:device:basic:1" and USN always ends with
   "::upnp:rootdevice".  Alexa's bridge discovery filters SSDP responses by ST
   and ignores anything that isn't the basic device type; echoing back the
   request ST (e.g. "upnp:rootdevice" or "ssdp:all") causes Alexa to silently
   discard the response and never fetch /description.xml. */
PROGMEM const char UPnP_UDP_RESPONSE_TEMPLATE[] =
    "HTTP/1.1 200 OK\r\n"
    "EXT:\r\n"
    "CACHE-CONTROL: max-age=100\r\n"
    "LOCATION: http://%d.%d.%d.%d:%d/description.xml\r\n"
    "SERVER: FreeRTOS/6.0.5, UPnP/1.0, IpBridge/1.17.0\r\n"
    "hue-bridgeid: %s\r\n"
    "ST: %s\r\n"
    "USN: uuid:2f402f80-da50-11e1-9b23-%s::%s\r\n"
    "\r\n";


class UPnP {
    public:
        void init();
        void handle();

    private:
        WiFiUDP _udp;
        unsigned int _tcp_port = UPnP_TCP_PORT;

        void _handleUDP();
        void _onUDPData(const IPAddress remoteIP, unsigned int remotePort, void *data, size_t len);
        void _sendUDPResponse();
};

#endif /*UPNP_H*/