#include <WiFi_utilities.h>
#include <ESPmDNS.h>

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
}

void setupOTA(void)
{
  ElegantOTA.begin(&server); // Start ElegantOTA
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
}

// Custom println implementation for UDP
size_t udp_println(const String &msg) {
  size_t sent = 0;
  int retries = 0;

  while ((UDP_PRINT_ENABLED == 1) && (retries <= UDP_MAX_RETRIES)) {
      
    if (udpConnected) {
      if(udp.beginPacket(udpServer, udpPort))
      {
        sent = udp.write((uint8_t*)msg.c_str(), msg.length());

        if(udp.endPacket()){
          break; // Packet sent successfully, break the retry loop
        }

      }
    }

    if(sent == 0)
    {
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

size_t udp_println(const char msg) {
  return udp_println(String(msg));
}

size_t udp_println(int val)
{
    return udp_println(String(val));
}

size_t udp_println(long val)
{
  return udp_println(String(val));
}
