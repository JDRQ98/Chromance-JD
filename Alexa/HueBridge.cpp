#include "HueBridge.h"
#include <vector>
#include <WiFi.h>
#include "templates.h"
#include "SimpleJson.h"

unsigned char HueBridge::addDevice(const char *device_name)
{
    device_t device;
    unsigned int device_id = lights.size();

    // init properties
    device.name = strdup(device_name);
    device.state = false;
    device.bri = 254;
    device.hue = 0;
    device.sat = 0;
    device.ct = 153;   // must be 153 - 500
    device.mode = 'x'; // possible values 'hs', 'xy', 'ct'

    // create the uniqueid
    String mac = WiFi.macAddress();
    snprintf(device.uniqueid, 27, "%s:%s-%02X", mac.c_str(), "00:00", device_id);

    lights.push_back(device);
    DEBUG_MSG_HUE("Device '%s' added as #%d\n", device_name, device_id);
    return device_id;
}

void HueBridge::start(AsyncWebServer& server)
{
    server.on("/description.xml", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handle_GetDescription(request);
    });

    // POST /api — Alexa requests a username; we always return "userid".
    // Use 5-arg form so the response fires AFTER the body is fully consumed.
    // With the 3-arg form, the handler fires on headers-complete; sending a
    // response while the body is still in-flight can corrupt HTTP pipelining.
    server.on("/api", HTTP_POST,
        [this](AsyncWebServerRequest *request) { handle_PostDeviceType(request); },
        nullptr,
        [](AsyncWebServerRequest*, uint8_t*, size_t, size_t, size_t) {}
    );

    server.on("/api/userid/lights", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handle_GetState(request);
    });

    server.on("/api/userid/lights/1", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handle_GetState(request);
    });

    // PUT with body — use 5-arg form to receive the JSON body
    server.on("/api/userid/lights/1/state", HTTP_PUT,
        [](AsyncWebServerRequest *request) {},  // request handler (fires on completion)
        nullptr,                                 // upload handler
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handle_PutState(request, data, len);
        }
    );

    server.on("/debug/clip.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handle_clip(request);
    });

    upnp.init();
    DEBUG_MSG_HUE("HueBridge routes registered, UPnP started\n");
}

void HueBridge::handle()
{
    upnp.handle();
}

/*
    GET /description.xml — tells Alexa where the Hue Bridge API lives
*/
void HueBridge::handle_GetDescription(AsyncWebServerRequest* request)
{
    DEBUG_MSG_HUE("\nHandling handle_GetDescription (GET %s) from %s\n",
        request->url().c_str(), request->client()->remoteIP().toString().c_str());

    IPAddress ip = WiFi.localIP();
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();

    char response[strlen_P(HUE_DESCRIPTION_TEMPLATE) + 64];
    snprintf_P(
        response, sizeof(response),
        HUE_DESCRIPTION_TEMPLATE,
        ip[0], ip[1], ip[2], ip[3], // URLBase
        ip[0], ip[1], ip[2], ip[3], // friendlyName
        mac.c_str(),                 // serialNumber
        mac.c_str()                  // UDN
    );

    request->send(200, "text/xml", response);
    DEBUG_MSG_HUE(response);
}

/*
    POST /api — Alexa creates an authorized user
    Body: {"devicetype":"Echo"}
    Response: [{"success":{"username":"userid"}}]
*/
void HueBridge::handle_PostDeviceType(AsyncWebServerRequest* request)
{
    DEBUG_MSG_HUE("Handling handle_PostDeviceType (POST %s) from %s\n",
        request->url().c_str(), request->client()->remoteIP().toString().c_str());

    char buffer[strlen_P(HUE_USER_JSON_TEMPLATE) + 10];
    snprintf_P(buffer, sizeof(buffer), HUE_USER_JSON_TEMPLATE, "userid");

    request->send(200, "application/json", buffer);
    DEBUG_MSG_HUE(buffer);
}

/*
    GET /api/userid/lights        — list all lights
    GET /api/userid/lights/1      — get a single light
*/
void HueBridge::handle_GetState(AsyncWebServerRequest* request)
{
    DEBUG_MSG_HUE("\nHandling handle_GetState (GET %s) from %s\n",
        request->url().c_str(), request->client()->remoteIP().toString().c_str());

    String uri = request->url();
    int pos = uri.indexOf("lights");
    unsigned char id = uri.substring(pos + 7).toInt();

    String response;

    if (0 == id)   // list all devices
    {
        response += "{";
        for (unsigned char i = 0; i < lights.size(); i++)
        {
            if (i > 0) response += ",";
            response += "\"" + String(i + 1) + "\":" + deviceJson(i);
        }
        response += "}";
    }
    else   // single device
    {
        response = deviceJson(id - 1);
    }

    request->send(200, "application/json", response);
    DEBUG_MSG_HUE(response.c_str());
}

String HueBridge::deviceJson(unsigned char id)
{
    if (id >= lights.size())
        return "{}";

    device_t device = lights[id];
    char buffer[strlen_P(HUE_DEVICE_JSON_TEMPLATE) + 76];
    snprintf_P(
        buffer, sizeof(buffer),
        HUE_DEVICE_JSON_TEMPLATE,
        device.name,
        device.uniqueid,
        device.state ? "true" : "false",
        device.bri,
        device.hue,
        device.sat,
        device.ct,
        device.mode == 'h' ? "hs" : device.mode == 'c' ? "ct" : "xy");
    return String(buffer);
}

/*
    PUT /api/userid/lights/1/state — Alexa sets brightness, color, on/off
*/
void HueBridge::handle_PutState(AsyncWebServerRequest* request, uint8_t* data, size_t len)
{
    DEBUG_MSG_HUE("\nHandling handle_PutState (PUT %s) from %s\n",
        request->url().c_str(), request->client()->remoteIP().toString().c_str());

    String uri = request->url();
    unsigned char id = 0;
    int pos = uri.indexOf("lights");
    if (pos >= 0)
        id = uri.substring(pos + 7).toInt();

    String body = String((const char*)data, len);
    DEBUG_MSG_HUE(body.c_str());

    if (body.length() == 0)
    {
        char response[strlen_P(HUE_ERROR_TEMPLATE) + uri.length() + 40];
        snprintf_P(response, sizeof(response), HUE_ERROR_TEMPLATE,
            5, uri.c_str(), "invalid/missing parameters in body");
        request->send(400, "application/json", response);
    }
    else if (id == 0 || id > lights.size())
    {
        char response[strlen_P(HUE_ERROR_TEMPLATE) + uri.length() + 30];
        snprintf_P(response, sizeof(response), HUE_ERROR_TEMPLATE,
            3, uri.c_str(), "resource not available");
        request->send(400, "application/json", response);
    }
    else
    {
        --id;
        SimpleJson json;
        json.parse(body);

        unsigned char bri = json.hasPropery("bri") ? json["bri"].getInt() : 0;
        short ct           = json.hasPropery("ct")  ? json["ct"].getInt()  : 0;
        unsigned int hue   = json.hasPropery("hue") ? json["hue"].getInt() : 0;
        unsigned char sat  = json.hasPropery("sat") ? json["sat"].getInt() : 0;

        char mode = json.hasPropery("xy") ? 'x' : json.hasPropery("ct") ? 'c' : 'h';
        setState(id, json["on"].getBool(), bri, ct, hue, sat, mode);

        char buffer[50];
        String rep = "[";
        snprintf(buffer, sizeof(buffer), "{\"success\":{\"/lights/%d/state/on\":%s}}",
            id + 1, lights[id].state ? "true" : "false");
        rep += buffer;
        if (json.hasPropery("bri"))
        {
            snprintf(buffer, sizeof(buffer), ",{\"success\":{\"/lights/%d/state/bri\":%d}}", id + 1, lights[id].bri);
            rep += buffer;
        }
        if (json.hasPropery("hue"))
        {
            snprintf(buffer, sizeof(buffer), ",{\"success\":{\"/lights/%d/state/hue\":%d}}", id + 1, lights[id].hue);
            rep += buffer;
        }
        if (json.hasPropery("sat"))
        {
            snprintf(buffer, sizeof(buffer), ",{\"success\":{\"/lights/%d/state/sat\":%d}}", id + 1, lights[id].sat);
            rep += buffer;
        }
        if (json.hasPropery("ct"))
        {
            snprintf(buffer, sizeof(buffer), ",{\"success\":{\"/lights/%d/state/ct\":%d}}", id + 1, lights[id].ct);
            rep += buffer;
        }
        rep += "]";
        request->send(200, "application/json", rep.c_str());
        DEBUG_MSG_HUE(rep.c_str());
    }
}

void HueBridge::setState(unsigned char id, bool state, unsigned char bri, short ct, unsigned int hue, unsigned char sat, char mode)
{
    if (id < lights.size())
    {
        lights[id].state = state;
        lights[id].bri   = bri != 0 ? bri : lights[id].bri;
        lights[id].ct    = ct  != 0 ? ct  : lights[id].ct;
        lights[id].hue   = hue;
        lights[id].sat   = sat;
        lights[id].mode  = mode;

        if (_setCallback)
        {
            _setCallback(id, lights[id].state, lights[id].bri, lights[id].ct,
                         lights[id].hue, lights[id].sat, lights[id].mode);
        }
    }
}

/*
    GET /api/{username} — Alexa fetches full bridge state before lights list.
    Returns a minimal bridge config with the lights payload embedded.
*/
void HueBridge::handle_GetBridgeInfo(AsyncWebServerRequest* request)
{
    DEBUG_MSG_HUE("[HueBridge] GET bridge info from %s\n",
        request->client()->remoteIP().toString().c_str());

    String ip  = WiFi.localIP().toString();
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();

    String lightsJson = "{";
    for (unsigned char i = 0; i < lights.size(); i++) {
        if (i > 0) lightsJson += ",";
        lightsJson += "\"" + String(i + 1) + "\":" + deviceJson(i);
    }
    lightsJson += "}";

    String response = "{";
    response += "\"lights\":" + lightsJson + ",";
    response += "\"groups\":{},\"schedules\":{},\"scenes\":{},";
    response += "\"config\":{";
    response += "\"name\":\"Philips hue\",";
    response += "\"apiversion\":\"1.16.0\",";
    response += "\"swversion\":\"01036659\",";
    response += "\"mac\":\"" + mac + "\",";
    response += "\"ipaddress\":\"" + ip + "\",";
    response += "\"bridgeid\":\"" + mac + "\"";
    response += "}}";

    request->send(200, "application/json", response);
}

/*
    Catch-all for any /api/* URL that didn't match a registered route.
    This handles dynamic usernames (Alexa may use whatever username it stored)
    and the /api/{username} bridge-info endpoint.
*/
void HueBridge::handleApiRequest(AsyncWebServerRequest* request)
{
    String url    = request->url();
    String method = request->methodToString();
    DEBUG_MSG_HUE("[HueBridge] unmatched %s %s from %s\n",
        method.c_str(), url.c_str(),
        request->client()->remoteIP().toString().c_str());

    if (request->method() != HTTP_GET) {
        request->send(405);
        return;
    }

    // Strip "/api/" and the username segment to get the sub-path
    int apiSlash = url.indexOf("/api/");
    if (apiSlash < 0) { request->send(404); return; }

    String afterApi = url.substring(apiSlash + 5); // "username[/rest]"
    int slash = afterApi.indexOf('/');
    String subPath = (slash >= 0) ? afterApi.substring(slash) : ""; // "/lights[/N]" or ""

    if (subPath.isEmpty() || subPath == "/") {
        handle_GetBridgeInfo(request);
    } else if (subPath.startsWith("/lights")) {
        handle_GetState(request);
    } else {
        request->send(404);
    }
}

void HueBridge::handle_clip(AsyncWebServerRequest* request)
{
    DEBUG_MSG_HUE("\nHandling handle_clip (GET %s) from %s\n",
        request->url().c_str(), request->client()->remoteIP().toString().c_str());
    char response[strlen_P(CLIP_PAGE)];
    snprintf_P(response, sizeof(response), CLIP_PAGE);
    request->send(200, "text/html", response);
}
