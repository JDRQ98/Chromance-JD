#include <WiFi_utilities.h>
#include <ESPmDNS.h>
#include "ASW.h" /* for OTA routines */
#include "WebSocket_Server.h"

/* Local variables */
const char *udpServerIP = UDP_SERVER_IP;

AsyncWebServer server(80);

HueBridge hueBridge;

static unsigned long ota_progress_millis = 0;

WiFiUDP udp;
IPAddress udpServer;
bool      udpConnected = false;


static void setupWebServer(void){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/EffectEditor.html", String(), false, nullptr); });

  server.on("/EffectEditor.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/EffectEditor.html", String(), false, nullptr); });

  server.on("/css/index.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/css/index.css", String(), false, nullptr); });

  server.on("/js/main.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/main.js", String(), false, nullptr); });

  server.on("/js/colorUtils.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/colorUtils.js", String(), false, nullptr); });

  server.on("/js/drawVisualizer.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/drawVisualizer.js", String(), false, nullptr); });

  server.on("/js/effectsManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/effectsManager.js", String(), false, nullptr); });

  server.on("/js/globalSettingsManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/globalSettingsManager.js", String(), false, nullptr); });

  server.on("/js/modalManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/modalManager.js", String(), false, nullptr); });

  server.on("/js/nodeManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/nodeManager.js", String(), false, nullptr); });

  // New core modules
  server.on("/js/core/eventBus.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/core/eventBus.js", String(), false, nullptr); });

  server.on("/js/core/stateStore.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/core/stateStore.js", String(), false, nullptr); });

  server.on("/js/core/wsClient.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/core/wsClient.js", String(), false, nullptr); });

  // Components
  server.on("/js/components/colorPaletteManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/components/colorPaletteManager.js", String(), false, nullptr); });

  // Utils
  server.on("/js/utils/settingsUtils.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/utils/settingsUtils.js", String(), false, nullptr); });

  // Managers
  server.on("/js/managers/modalManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/managers/modalManager.js", String(), false, nullptr); });

  server.on("/js/managers/nodeManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/managers/nodeManager.js", String(), false, nullptr); });

  server.on("/js/managers/effectsManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/managers/effectsManager.js", String(), false, nullptr); });

  server.on("/js/managers/globalSettingsManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/managers/globalSettingsManager.js", String(), false, nullptr); });

  server.on("/js/managers/main.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/managers/main.js", String(), false, nullptr); });

  server.on("/js/managers/sequenceManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/managers/sequenceManager.js", String(), false, nullptr); });

  // Core - Effect Sequencer
  server.on("/js/core/effectSequencer.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/core/effectSequencer.js", String(), false, nullptr); });

  // Components - Sequence Editor Modal
  server.on("/js/components/sequenceEditorModal.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/components/sequenceEditorModal.js", String(), false, nullptr); });

  // Core - Topology Data
  server.on("/js/core/topology.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/core/topology.js", String(), false, nullptr); });

  // Core - Ripple Simulator
  server.on("/js/core/rippleSimulator.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/core/rippleSimulator.js", String(), false, nullptr); });

  // Core - Canvas Visualizer
  server.on("/js/core/canvasVisualizer.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/core/canvasVisualizer.js", String(), false, nullptr); });

  HTTP_backend_init();
}

static void setupWifiManager(void)
{
  /* this function initializes wifi connection via 'WifiManager' library, which creates an AP for user to provide SSDI and password, avoiding the hardcoding of wifi credentials */
  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  bool res;

  res = wm.autoConnect("Chromance"); // AP not password protected

  if (!res)
  {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.print("Connected successfully. Local IP Address: ");
    Serial.println(WiFi.localIP());
  }
}

void setupWiFi(void)
{
  setupWifiManager();

  /* Initialize SPIFFS for serving HTML */
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  setupWebServer();
  WebSocket_init(&server);
}

void setupOTA(void)
{
  ElegantOTA.setAutoReboot(true);
  ElegantOTA.begin(&server); // Start ElegantOTA
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  server.begin(); 
}

void setupUDP(void)
{
  // Convert IP to IPAddress Object
  if (udpServerIP != NULL) {
    udpServer.fromString(udpServerIP);
    udpConnected = true;
    Serial.print("UDP Server address: ");
    Serial.print(udpServerIP);
    Serial.print(":");
    Serial.println(udpPort);
    udp.begin(WiFi.localIP(), udpPort);
    String ipAddress = WiFi.localIP().toString();                       // Get the microcontroller's IP address as a string
    String udpMessage = "Chromance device is ONLINE! IP: " + ipAddress; // Create the UDP message with the IP address
    udp_println(udpMessage);
  }
  else
  {
    Serial.print("No valid UDP Server Address");
    udpConnected = false;
  }
}

void WiFi_Utilities_init(void)
{
  setupWiFi();
  setupUDP();
  setupOTA();

  hueBridge.addDevice(ALEXA_DEVICE_NAME);
  hueBridge.onSetState(handle_SetState);
  hueBridge.start();

  DEBUG_MSG_HUE("Setup MDNS for http://hexagono.local");
  if (!MDNS.begin("hexagono"))
  {
    DEBUG_MSG_HUE("Error setting up MDNS responder!");
  }
}

void WiFi_Utilities_loop(void)
{
  ElegantOTA.loop();
  hueBridge.handle();
  WebSocket_loop();
}

/// UDP printf function (supports printf-style formatting)
size_t udp_printf(const char *format, ...) {
  char buffer[256]; // Choose an appropriate buffer size.  Be careful of stack overflows!
  va_list args;
  va_start(args, format);
  int len = vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  if (len > 0) {
    return udp_println(buffer);
  } else {
    Serial.println("Error formatting UDP message in udp_printf"); // Error handling
    return 0;
  }
}

// Overload udp_println to accept a single char argument
size_t udp_println(char msg) {
  return udp_println(String(msg));
}

size_t udp_println(const String &msg) {
  size_t sent = 0;
  int retries = 0;

  while ((UDP_PRINT_ENABLED == 1) && (retries <= UDP_MAX_RETRIES)) {

    if (udpConnected) {
      if (udp.beginPacket(udpServer, udpPort)) {
        sent = udp.write((uint8_t*)msg.c_str(), msg.length());

        if (udp.endPacket()) {
          break; // Packet sent successfully, break the retry loop
        }

      }
    }

    if (sent == 0) {
      retries++;
      Serial.print("UDP send failed (retrying ");
      Serial.print(retries);
      Serial.print("/");
      Serial.print(UDP_MAX_RETRIES);
      Serial.println(")");
      delay(UDP_RETRY_DELAY);
    }

  }

  Serial.println(msg);

  if (sent == 0 && retries > UDP_MAX_RETRIES)
    Serial.println("UDP message failed to send even after all retries");

  return sent;
}

size_t udp_println(const char *msg) {
  return udp_println(String(msg));
}

size_t udp_println(int val) {
  return udp_println(String(val));
}

size_t udp_println(long val) {
  return udp_println(String(val));
}