#include "WebSocket_Server.h"
#include "WiFi_utilities.h"
#include "MCAL/ripple.h"

// WebSocket instance
AsyncWebSocket ws("/ws");

// Message ID tracking for ACK
static unsigned long lastMessageId = 0;

// Forward declarations
static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len);
static void handleMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len);
static void sendAck(AsyncWebSocketClient *client, unsigned long msgId, bool success);
static void sendState(AsyncWebSocketClient *client);
static String buildStateJson(void);

void WebSocket_init(AsyncWebServer* server) {
    ws.onEvent(onWsEvent);
    server->addHandler(&ws);
    udp_println("WebSocket server initialized at /ws");
}

void WebSocket_loop(void) {
    // Cleanup disconnected clients periodically
    ws.cleanupClients();
}

uint8_t WebSocket_getClientCount(void) {
    return ws.count();
}

void WebSocket_broadcastState(void) {
    if (ws.count() > 0) {
        String stateJson = buildStateJson();
        ws.textAll(stateJson);
    }
}

void WebSocket_notifyRippleFired(int nodeId, int direction, uint32_t color) {
    if (ws.count() > 0) {
        JsonDocument doc;
        doc["type"] = WS_MSG_RIPPLE_FIRED;

        JsonObject payload = doc["payload"].to<JsonObject>();
        payload["nodeId"] = nodeId;
        payload["direction"] = direction;
        payload["color"] = color;

        String output;
        serializeJson(doc, output);
        ws.textAll(output);
    }
}

static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            udp_printf("WebSocket client #%u connected from %s",
                      client->id(), client->remoteIP().toString().c_str());
            // Send current state to newly connected client
            sendState(client);
            break;

        case WS_EVT_DISCONNECT:
            udp_printf("WebSocket client #%u disconnected", client->id());
            break;

        case WS_EVT_DATA:
            handleMessage(client, data, len);
            break;

        case WS_EVT_PONG:
            udp_printf("WebSocket pong from client #%u", client->id());
            break;

        case WS_EVT_ERROR:
            udp_printf("WebSocket error from client #%u", client->id());
            break;
    }
}

static void handleMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len) {
    // Parse incoming JSON message
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        udp_printf("WebSocket JSON parse error: %s", error.c_str());
        return;
    }

    const char* msgType = doc["type"];
    unsigned long msgId = doc["id"] | 0;

    if (!msgType) {
        udp_printf("WebSocket message missing type field");
        return;
    }

    udp_printf("WebSocket received: type=%s, id=%lu", msgType, msgId);

    // Handle different message types
    if (strcmp(msgType, WS_MSG_GET_STATE) == 0) {
        sendState(client);
        sendAck(client, msgId, true);
    }
    else if (strcmp(msgType, WS_MSG_FIRE_RIPPLE) == 0) {
        extern boolean manualFireRipple;
        manualFireRipple = 1;
        sendAck(client, msgId, true);
        udp_println("Manual ripple triggered via WebSocket");
    }
    else if (strcmp(msgType, WS_MSG_UPDATE_CONFIG) == 0) {
        JsonObject payload = doc["payload"];
        bool success = true;

        if (payload["globalSettings"]) {
            JsonObject settings = payload["globalSettings"];

            // Update global parameters from settings
            if (settings["rippleDelay"]) {
                int val = settings["rippleDelay"];
                if (val >= HTTP_CURRENTDELAYBETWEENRIPPLES_MIN &&
                    val <= HTTP_CURRENTDELAYBETWEENRIPPLES_MAX) {
                    GlobalParameters.currentDelayBetweenRipples = val;
                }
            }

            if (settings["rippleLifeSpan"]) {
                unsigned long val = settings["rippleLifeSpan"];
                if (val >= HTTP_CURRENTRIPPLELIFESPAN_MIN &&
                    val <= HTTP_CURRENTRIPPLELIFESPAN_MAX) {
                    GlobalParameters.currentRippleLifeSpan = val;
                }
            }

            if (settings["rippleSpeed"]) {
                float val = settings["rippleSpeed"];
                if (val >= HTTP_CURRENTRIPPLESPEED_MIN &&
                    val <= HTTP_CURRENTRIPPLESPEED_MAX) {
                    GlobalParameters.currentRippleSpeed = val;
                }
            }

            if (settings["decayPerTick"]) {
                float val = settings["decayPerTick"];
                if (val >= HTTP_CURRENTDECAY_MIN &&
                    val <= HTTP_CURRENTDECAY_MAX) {
                    GlobalParameters.currentDecay = val;
                }
            }

            if (settings["hueDeltaTick"]) {
                int val = settings["hueDeltaTick"];
                if (val >= HTTP_CURRENTRAINBOWDELTAPERTICK_MIN &&
                    val <= HTTP_CURRENTRAINBOWDELTAPERTICK_MAX) {
                    GlobalParameters.currentRainbowDeltaPerTick = val;
                }
            }

            if (settings["desiredBehavior"]) {
                const char* behavior = settings["desiredBehavior"];
                if (strcmp(behavior, "normal") == 0) {
                    GlobalParameters.currentBehavior = normal;
                } else if (strcmp(behavior, "feisty") == 0) {
                    GlobalParameters.currentBehavior = feisty;
                } else if (strcmp(behavior, "angry") == 0) {
                    GlobalParameters.currentBehavior = angry;
                } else if (strcmp(behavior, "pastel") == 0) {
                    GlobalParameters.currentBehavior = pastel;
                }
            }

            if (settings["rippleDirection"]) {
                const char* dir = settings["rippleDirection"];
                if (strcmp(dir, "allDirections") == 0) {
                    GlobalParameters.currentDirection = ALL_DIRECTIONS;
                } else if (strcmp(dir, "clockwise") == 0) {
                    GlobalParameters.currentDirection = CLOCKWISE;
                } else if (strcmp(dir, "counterClockwise") == 0) {
                    GlobalParameters.currentDirection = COUNTER_CLOCKWISE;
                }
            }

            // Handle colors array
            if (settings["colors"]) {
                JsonArray colors = settings["colors"];
                GlobalParameters.currentNumberofColors = min((int)colors.size(),
                    (int)HTTP_CURRENTNUMBEROFCOLORS_MAX);
                // Note: Color array storage would need additional implementation
            }

            udp_println("Configuration updated via WebSocket");
        }

        if (payload["masterEnabled"]) {
            GlobalParameters.loop_MasterFireRippleEnabled = payload["masterEnabled"];
            udp_printf("Master enabled: %d", GlobalParameters.loop_MasterFireRippleEnabled);
        }

        // Handle activeNodes array
        if (payload["activeNodes"]) {
            JsonArray activeNodesArr = payload["activeNodes"];
            ActiveNodesConfig.activeNodeCount = 0;
            
            for (JsonVariant nodeId : activeNodesArr) {
                if (ActiveNodesConfig.activeNodeCount < MAX_ACTIVE_NODES) {
                    int id = nodeId.as<int>();
                    if (id >= 0 && id < MAX_ACTIVE_NODES) {
                        ActiveNodesConfig.activeNodes[ActiveNodesConfig.activeNodeCount] = id;
                        ActiveNodesConfig.activeNodeCount++;
                    }
                }
            }
            udp_printf("Active nodes updated: %d nodes", ActiveNodesConfig.activeNodeCount);
        }

        // Handle nodeSpecificSettings object
        if (payload["nodeSpecificSettings"]) {
            JsonObject nodeSettings = payload["nodeSpecificSettings"];
            
            // First, clear all existing overrides
            for (int i = 0; i < MAX_ACTIVE_NODES; i++) {
                NodeSettings[i].hasOverride = false;
            }
            
            // Parse each node's settings
            for (JsonPair kv : nodeSettings) {
                int nodeId = atoi(kv.key().c_str());
                if (nodeId >= 0 && nodeId < MAX_ACTIVE_NODES) {
                    JsonObject nodeCfg = kv.value().as<JsonObject>();
                    NodeSettings[nodeId].hasOverride = true;
                    
                    if (nodeCfg["rippleSpeed"]) {
                        NodeSettings[nodeId].rippleSpeed = nodeCfg["rippleSpeed"].as<float>();
                    } else {
                        NodeSettings[nodeId].rippleSpeed = GlobalParameters.currentRippleSpeed;
                    }
                    
                    if (nodeCfg["decayPerTick"]) {
                        NodeSettings[nodeId].decayPerTick = nodeCfg["decayPerTick"].as<float>();
                    } else {
                        NodeSettings[nodeId].decayPerTick = GlobalParameters.currentDecay;
                    }
                    
                    if (nodeCfg["rippleDelay"]) {
                        NodeSettings[nodeId].rippleDelay = nodeCfg["rippleDelay"].as<short>();
                    } else {
                        NodeSettings[nodeId].rippleDelay = GlobalParameters.currentDelayBetweenRipples;
                    }
                    
                    if (nodeCfg["rippleLifeSpan"]) {
                        NodeSettings[nodeId].rippleLifeSpan = nodeCfg["rippleLifeSpan"].as<unsigned long>();
                    } else {
                        NodeSettings[nodeId].rippleLifeSpan = GlobalParameters.currentRippleLifeSpan;
                    }
                    
                    if (nodeCfg["hueDeltaTick"]) {
                        NodeSettings[nodeId].hueDeltaTick = nodeCfg["hueDeltaTick"].as<short>();
                    } else {
                        NodeSettings[nodeId].hueDeltaTick = GlobalParameters.currentRainbowDeltaPerTick;
                    }
                    
                    if (nodeCfg["desiredBehavior"]) {
                        const char* behavior = nodeCfg["desiredBehavior"];
                        if (strcmp(behavior, "normal") == 0) {
                            NodeSettings[nodeId].behavior = normal;
                        } else if (strcmp(behavior, "feisty") == 0) {
                            NodeSettings[nodeId].behavior = feisty;
                        } else if (strcmp(behavior, "angry") == 0) {
                            NodeSettings[nodeId].behavior = angry;
                        }
                    } else {
                        NodeSettings[nodeId].behavior = GlobalParameters.currentBehavior;
                    }
                    
                    if (nodeCfg["rippleDirection"]) {
                        const char* dir = nodeCfg["rippleDirection"];
                        if (strcmp(dir, "allDirections") == 0) {
                            NodeSettings[nodeId].direction = ALL_DIRECTIONS;
                        } else if (strcmp(dir, "inward") == 0) {
                            NodeSettings[nodeId].direction = 3;  // Inward = direction 3 (toward center)
                        } else if (strcmp(dir, "outward") == 0) {
                            NodeSettings[nodeId].direction = 0;  // Outward = direction 0 (away from center)
                        }
                    } else {
                        NodeSettings[nodeId].direction = GlobalParameters.currentDirection;
                    }
                    
                    udp_printf("Node %d settings updated", nodeId);
                }
            }
        }

        sendAck(client, msgId, success);

        // Broadcast updated state to all clients
        WebSocket_broadcastState();
    }
    else {
        udp_printf("WebSocket unknown message type: %s", msgType);
        sendAck(client, msgId, false);
    }
}

static void sendAck(AsyncWebSocketClient *client, unsigned long msgId, bool success) {
    JsonDocument doc;
    doc["type"] = WS_MSG_ACK;
    doc["ackId"] = msgId;
    doc["success"] = success;

    String output;
    serializeJson(doc, output);
    client->text(output);
}

static void sendState(AsyncWebSocketClient *client) {
    String stateJson = buildStateJson();
    client->text(stateJson);
}

static String buildStateJson(void) {
    JsonDocument doc;
    doc["type"] = WS_MSG_STATE;

    JsonObject payload = doc["payload"].to<JsonObject>();
    payload["masterEnabled"] = GlobalParameters.loop_MasterFireRippleEnabled;
    payload["rippleDelay"] = GlobalParameters.currentDelayBetweenRipples;
    payload["rippleLifeSpan"] = GlobalParameters.currentRippleLifeSpan;
    payload["rippleSpeed"] = GlobalParameters.currentRippleSpeed;
    payload["decayPerTick"] = GlobalParameters.currentDecay;
    payload["hueDeltaTick"] = GlobalParameters.currentRainbowDeltaPerTick;
    payload["numberOfColors"] = GlobalParameters.currentNumberofColors;
    payload["behavior"] = GlobalParameters.currentBehavior;
    payload["direction"] = GlobalParameters.currentDirection;
    payload["wsClients"] = ws.count();

    // Include active nodes array
    JsonArray activeNodesArr = payload["activeNodes"].to<JsonArray>();
    for (int i = 0; i < ActiveNodesConfig.activeNodeCount; i++) {
        activeNodesArr.add(ActiveNodesConfig.activeNodes[i]);
    }

    String output;
    serializeJson(doc, output);
    return output;
}
