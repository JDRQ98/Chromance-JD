#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include "Arduino.h"
#include <WebServer.h>        // Must be included BEFORE ESPAsyncWebServer to avoid HTTP method enum conflicts
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "HTTP_Server.h"

// WebSocket message types (UI -> Firmware)
#define WS_MSG_UPDATE_CONFIG  "UPDATE_CONFIG"
#define WS_MSG_GET_STATE      "GET_STATE"
#define WS_MSG_FIRE_RIPPLE    "FIRE_RIPPLE"

// WebSocket message types (Firmware -> UI)
#define WS_MSG_ACK            "ACK"
#define WS_MSG_STATE          "STATE"
#define WS_MSG_RIPPLE_FIRED   "RIPPLE_FIRED"

// Maximum WebSocket clients
#define WS_MAX_CLIENTS 4

// APIs
void WebSocket_init(AsyncWebServer* server);
void WebSocket_loop(void);
void WebSocket_broadcastState(void);
void WebSocket_notifyRippleFired(int nodeId, int direction, uint32_t color);

// Get number of connected clients
uint8_t WebSocket_getClientCount(void);

#endif /* WEBSOCKET_SERVER_H */
