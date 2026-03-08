
#include "UPnP.h"
#include "WiFi_utilities.h" // for udp_printf

void UPnP::handle()
{
    _handleUDP();
}

void UPnP::init()
{
    // UDP setup
    #ifdef ESP32
        _udp.beginMulticast(UPnP_UDP_MULTICAST_IP, UPnP_UDP_MULTICAST_PORT);
    #else
        #error Platform not supported
    #endif
    DEBUG_MSG_UPnP("[UPnP] UDP server started\n");
}

/*
    Sample response message

    HTTP/1.1 200 OK
    EXT:
    CACHE-CONTROL: max-age=100
    LOCATION: http://192.168.86.47:80/description.xml
    SERVER: FreeRTOS/6.0.5, UPnP/1.0, IpBridge/1.17.0
    hue-bridgeid: f008d1d2cb4c
    ST: urn:schemas-upnp-org:device:basic:1
    USN: uuid:2f402f80-da50-11e1-9b23-f008d1d2cb4c::upnp:rootdevice

*/
void UPnP::_sendUDPResponse()
{
    IPAddress ip = WiFi.localIP();
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();

    /* Always advertise the Hue bridge device type regardless of what ST was in
       the M-SEARCH.  Alexa's bridge discovery filters responses by ST and only
       processes ones that identify a basic UPnP device; echoing back the
       request ST (e.g. "upnp:rootdevice" or "ssdp:all") causes Alexa to skip
       the response entirely and never fetch /description.xml. */
    char response[strlen_P(UPnP_UDP_RESPONSE_TEMPLATE) + 256];
    snprintf_P(
        response, sizeof(response),
        UPnP_UDP_RESPONSE_TEMPLATE,
        ip[0], ip[1], ip[2], ip[3], _tcp_port,
        mac.c_str(),
        "urn:schemas-upnp-org:device:basic:1",  // ST  — fixed Hue bridge type
        mac.c_str(),
        "upnp:rootdevice"                        // USN suffix — fixed rootdevice
        );

    DEBUG_MSG_UPnP("[UPnP] -> response sent to %s:%d\n", _udp.remoteIP().toString().c_str(), _udp.remotePort());

    _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
    #if defined(ESP32)
        _udp.printf(response);
    #else
        #error Platform not supported
    #endif
    _udp.endPacket();
}

/*
    Sample message received from Amazon Echo

    M-SEARCH * HTTP/1.1
    HOST: 239.255.255.250:1900
    ST: ssdp:all
    MAN: "ssdp:discover"
    MX: 3

*/ 
void UPnP::_handleUDP()
{
    int len = _udp.parsePacket();
    if (len > 0)
    {
        unsigned char data[len + 1];
        _udp.read(data, len);
        data[len] = 0;
        String request = (const char *)data;
        if (request.indexOf("M-SEARCH") >= 0)
        {
            /* Extract ST header for logging (search for "\nST:" to avoid
               matching "ST" inside "HOST:"). */
            String st = "(unknown)";
            int stIdx = request.indexOf("\nST:");
            if (stIdx >= 0) {
                stIdx += 1; // skip the '\n', point at "ST:"
                int eol = request.indexOf('\r', stIdx);
                if (eol < 0) eol = request.indexOf('\n', stIdx + 1);
                st = request.substring(stIdx + 3, eol >= 0 ? eol : (int)request.length());
                st.trim();
            }

            bool willRespond = (request.indexOf("ssdp:discover") > 0)  // MAN header
                            || (request.indexOf("ssdp:all") > 0)
                            || (request.indexOf("upnp:rootdevice") > 0)
                            || (request.indexOf("device:basic:1") > 0);
            DEBUG_MSG_UPnP("[UPnP] M-SEARCH from %s:%d ST=%s %s\n",
                _udp.remoteIP().toString().c_str(), _udp.remotePort(),
                st.c_str(),
                willRespond ? "(responding)" : "(ignored)");
            if (willRespond)
            {
                _sendUDPResponse();
            }
        }
    }
}
