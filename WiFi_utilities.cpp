#include <WiFi_utilities.h>
#include <ESPmDNS.h>
#include "ASW.h" /* for OTA routines */
#include "Alexa/AlexaControl.h"
#include "Alexa/HueBridge.h"
#include "HTTP_Server.h"
#include "MCAL/ripple.h"
#include "MCAL/EEP.h"

/* Local variables */
const char *udpServerIP = UDP_SERVER_IP;

AsyncWebServer server(80);

static HueBridge hueBridge;

WiFiUDP udp;
IPAddress udpServer;
bool      udpConnected = false;


static void setupWebServer(void){
  /* Landing page */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", String(), false, nullptr); });

  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", String(), false, nullptr); });

  /* Profile editor page */
  server.on("/ProfileEditor.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/ProfileEditor.html", String(), false, nullptr); });

  /* CSS */
  server.on("/css/index.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/css/index.css", String(), false, nullptr); });

  server.on("/css/landing.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/css/landing.css", String(), false, nullptr); });

  /* JS - shared modules */
  server.on("/js/main.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/js/main.js", String(), false, nullptr); });

  server.on("/js/colorUtils.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/js/colorUtils.js", String(), false, nullptr); });

  server.on("/js/drawVisualizer.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/js/drawVisualizer.js", String(), false, nullptr); });

  server.on("/js/effectsManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/js/effectsManager.js", String(), false, nullptr); });

  server.on("/js/globalSettingsManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/js/globalSettingsManager.js", String(), false, nullptr); });

  server.on("/js/modalManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/js/modalManager.js", String(), false, nullptr); });

  server.on("/js/nodeManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/js/nodeManager.js", String(), false, nullptr); });

  server.on("/js/timelineManager.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/js/timelineManager.js", String(), false, nullptr); });

  /* JS - landing page */
  server.on("/js/landing.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/js/landing.js", String(), false, nullptr); });

  server.on("/js/landing-no-cors.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/js/landing-no-cors.js", String(), false, nullptr); });

  server.on("/resetWifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "WiFi credentials erased. Restarting into AP mode — connect to 'Chromance' to reconfigure.");
    wifiManagerResetSettings();
    delay(500);
    ESP.restart();
  });

  /* Allow cross-origin requests so the WebUI works when accessed from a
     different network segment than the ESP32 (e.g. main WiFi vs guest WiFi).
     DefaultHeaders are appended to every response automatically. */
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  /* Catch-all: OPTIONS preflight → 200, /api/* → HueBridge, everything else → 404 */
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
      return;
    }
    if (request->url().startsWith("/api/")) {
      hueBridge.handleApiRequest(request);
      return;
    }
    request->send(404);
  });

  HTTP_backend_init();

  /* Register Hue Bridge API routes + start UPnP SSDP listener on port 1900.
     This must come AFTER HTTP_backend_init() so all routes are registered before
     server.begin() is called in setupOTA(). */
  hueBridge.addDevice("Chromance");
  hueBridge.onSetState([](unsigned char /*id*/, bool state, unsigned char bri,
                          short /*ct*/, unsigned int /*hue*/, unsigned char /*sat*/,
                          char /*mode*/) {
    xSemaphoreTake(gParamsMutex, portMAX_DELAY);

    GlobalParameters.StableColorMode = state;        /* ON → nightlight, OFF → ripples */
    GlobalParameters.MasterFireRippleEnabled = false;
    pendingKillProfileIndex = -2;                    /* kill all in-flight ripples */

    if (bri > 0) {
      GlobalParameters.Brightness = bri;
      for (int s = 0; s < NUMBER_OF_STRIPS; s++)
        strips[s].setBrightness(bri);
    }

    EEPROM_MarkDirty();
    xSemaphoreGive(gParamsMutex);

    udp_printf("[Alexa] state=%s bri=%d\n", state ? "ON" : "OFF", bri);
  });
  hueBridge.start(server);
}

static void setupWifiManager(void)
{
  /* Uses wifiManagerConnect() wrapper so WiFiManager.h (which pulls in
     WebServer.h) is compiled in an isolated TU, avoiding HTTP method enum
     conflicts with ESPAsyncWebServer. */
  if (!wifiManagerConnect("Chromance"))
  {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  else
  {
    Serial.print("Connected successfully. Local IP Address: ");
    Serial.println(WiFi.localIP());
  }
}

void setupWiFi(void)
{
  Serial.printf("[T+%lums] WiFiManager start\n", millis());
  setupWifiManager();
  Serial.printf("[T+%lums] WiFiManager done\n", millis());

  /* Initialize LittleFS for serving HTML */
  Serial.printf("[T+%lums] LittleFS mount start\n", millis());
  if(!LittleFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  Serial.printf("[T+%lums] LittleFS mounted\n", millis());

  setupWebServer();
  Serial.printf("[T+%lums] WebServer routes registered\n", millis());
}

void setupOTA(void)
{
  /* ElegantOTA - OTA via browser at /update and PlatformIO custom upload script */
  ElegantOTA.setAutoReboot(true);
  ElegantOTA.begin(&server);
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

  Serial.printf("[T+%lums] UDP setup start\n", millis());
  setupUDP();
  Serial.printf("[T+%lums] UDP done\n", millis());

  Serial.printf("[T+%lums] OTA setup start\n", millis());
  setupOTA();
  Serial.printf("[T+%lums] OTA done\n", millis());

  AlexaControl_Init();
  Serial.printf("[T+%lums] fauxmoESP started\n", millis());

  Serial.printf("[T+%lums] mDNS setup start\n", millis());
  if (!MDNS.begin("hexagono"))
  {
    DEBUG_MSG_HUE("Error setting up MDNS responder!");
  }
  else
  {
    DEBUG_MSG_HUE("mDNS 'hexagono.local' registered\n");
  }
  Serial.printf("[T+%lums] mDNS done\n", millis());
}

void WiFi_Utilities_loop(void)
{
  ElegantOTA.loop();
  AlexaControl_Handle();  /* no-op stub */
  hueBridge.handle();     /* polls UPnP SSDP socket for M-SEARCH packets */
}

/// UDP printf function (supports printf-style formatting)
size_t udp_printf(const char *format, ...) {
  char buffer[1024]; // Choose an appropriate buffer size.  Be careful of stack overflows!
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