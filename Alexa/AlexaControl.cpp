/* AlexaControl.cpp — stubs only.
   Alexa discovery and control is handled by HueBridge + UPnP (WiFi_utilities.cpp).
   fauxmoESP has been removed: it advertised port 8082 in SSDP responses, which
   modern Echo devices ignore (they require port 80), and it competed for the
   UDP multicast socket on port 1900 with the custom UPnP listener. */

#include "AlexaControl.h"

void AlexaControl_Init()   { /* no-op */ }
void AlexaControl_Handle() { /* no-op */ }

