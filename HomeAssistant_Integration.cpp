#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoHA.h>

#include "HTTP_Server.h"   // GlobalParameters_struct, NUMBER_OF_PROFILES, NUMBER_OF_SC_PRESETS
#include "MCAL/ripple.h"   // strips[]
#include "MCAL/EEP.h"      // EEPROM_MarkDirty
#include "HomeAssistant_Integration.h"

extern GlobalParameters_struct GlobalParameters;
extern SemaphoreHandle_t gParamsMutex;

// --- MQTT + HA device ---
static WiFiClient wifiClient;
static HADevice device("chromance_hexagon");
static HAMqtt mqttClient(wifiClient, device);

// --- HA Entities ---
static HALight  haLight("hexagon_light", HALight::BrightnessFeature | HALight::RGBFeature);
static HASelect haMode("hexagon_mode");
static HANumber haBpm("hexagon_bpm", HANumber::PrecisionP0);
static HAButton haTapTempo("hexagon_tap_tempo");

// --- Mode mapping (HASelect option index -> firmware state) ---
enum HAModeType : uint8_t {
    MODE_OFF = 0,
    MODE_RIPPLE_SEQ,
    MODE_STABLE_SEQ,
    MODE_RIPPLE_PROFILE,
    MODE_STABLE_PRESET
};
struct HAModeMapping { HAModeType type; int8_t slot; };
#define MAX_HA_MODES (3 + NUMBER_OF_PROFILES + NUMBER_OF_SC_PRESETS)
static HAModeMapping g_modeMap[MAX_HA_MODES];
static uint8_t g_modeCount = 0;

// Options string is built once at boot. HASelect::setOptions can only be
// called once per the ArduinoHA contract, so profile/preset name changes
// made via the WebUI after boot won't reflect in HA until restart.
static String g_modeOptions;

// --- Tap tempo ring buffer ---
#define TAP_BUFFER_SIZE 4
#define TAP_TIMEOUT_MS  2000UL
static unsigned long g_tapBuffer[TAP_BUFFER_SIZE] = {0};
static uint8_t       g_tapCount = 0;

// Hue conversion (RGB -> 16-bit HSV hue)
static unsigned int rgbHexToHue16(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t maxC = max(r, max(g, b));
    uint8_t minC = min(r, min(g, b));
    if (maxC == minC) return 0;
    float hue;
    float delta = maxC - minC;
    if (maxC == r) {
        hue = (float)(g - b) / delta;
        if (hue < 0) hue += 6.0f;
    } else if (maxC == g) {
        hue = 2.0f + (float)(b - r) / delta;
    } else {
        hue = 4.0f + (float)(r - g) / delta;
    }
    return (unsigned int)(hue * 65536.0f / 6.0f);
}

// Read-only: compute the HASelect index that matches the current GlobalParameters.
// Caller must hold gParamsMutex.
static int8_t computeCurrentModeIndex_locked() {
    if (!GlobalParameters.MasterFireRippleEnabled) {
        for (uint8_t i = 0; i < g_modeCount; i++)
            if (g_modeMap[i].type == MODE_OFF) return (int8_t)i;
        return -1;
    }
    if (GlobalParameters.StableColorMode) {
        if (GlobalParameters.SCSeqEnabled) {
            for (uint8_t i = 0; i < g_modeCount; i++)
                if (g_modeMap[i].type == MODE_STABLE_SEQ) return (int8_t)i;
        } else {
            for (uint8_t i = 0; i < g_modeCount; i++)
                if (g_modeMap[i].type == MODE_STABLE_PRESET &&
                    g_modeMap[i].slot == (int8_t)GlobalParameters.SCSeqCurrentPreset)
                    return (int8_t)i;
        }
    } else {
        if (GlobalParameters.SequencerEnabled) {
            for (uint8_t i = 0; i < g_modeCount; i++)
                if (g_modeMap[i].type == MODE_RIPPLE_SEQ) return (int8_t)i;
        } else {
            for (uint8_t i = 0; i < g_modeCount; i++)
                if (g_modeMap[i].type == MODE_RIPPLE_PROFILE &&
                    g_modeMap[i].slot == (int8_t)GlobalParameters.SequencerCurrentProfile)
                    return (int8_t)i;
        }
    }
    return -1;
}

static void buildModeOptions() {
    g_modeCount = 0;
    g_modeOptions = "Off";
    g_modeMap[g_modeCount++] = { MODE_OFF, -1 };

    g_modeOptions += ";Ripple Sequence";
    g_modeMap[g_modeCount++] = { MODE_RIPPLE_SEQ, -1 };

    g_modeOptions += ";Stable Sequence";
    g_modeMap[g_modeCount++] = { MODE_STABLE_SEQ, -1 };

    for (int i = 0; i < NUMBER_OF_PROFILES; i++) {
        if (!GlobalParameters.RippleProfiles[i].Active) continue;
        g_modeOptions += ";Ripple: ";
        g_modeOptions += GlobalParameters.RippleProfiles[i].ProfileName;
        g_modeMap[g_modeCount++] = { MODE_RIPPLE_PROFILE, (int8_t)i };
    }
    for (int i = 0; i < NUMBER_OF_SC_PRESETS; i++) {
        if (!GlobalParameters.SCPresets[i].Active) continue;
        g_modeOptions += ";Stable: ";
        g_modeOptions += GlobalParameters.SCPresets[i].PresetName;
        g_modeMap[g_modeCount++] = { MODE_STABLE_PRESET, (int8_t)i };
    }
}

// --- HA callbacks ---

static void onStateCommand(bool state, HALight* sender) {
    xSemaphoreTake(gParamsMutex, portMAX_DELAY);
    GlobalParameters.MasterFireRippleEnabled = state;
    xSemaphoreGive(gParamsMutex);
    EEPROM_MarkDirty();
    sender->setState(state);
}

static void onBrightnessCommand(uint8_t brightness, HALight* sender) {
    xSemaphoreTake(gParamsMutex, portMAX_DELAY);
    GlobalParameters.Brightness = brightness;
    for (int i = 0; i < NUMBER_OF_STRIPS; i++) strips[i].setBrightness(brightness);
    xSemaphoreGive(gParamsMutex);
    EEPROM_MarkDirty();
    sender->setBrightness(brightness);
}

// Update the active hue. Does NOT force StableColorMode any more —
// the Mode select is the authoritative way to switch between ripple and stable.
// If the device is already in a Stable mode, the new hue takes effect immediately;
// in Ripple mode the hue is stored but won't visibly do anything until Mode is changed.
static void onRGBColorCommand(HALight::RGBColor color, HALight* sender) {
    xSemaphoreTake(gParamsMutex, portMAX_DELAY);
    GlobalParameters.StableColorHue = rgbHexToHue16(color.red, color.green, color.blue);
    xSemaphoreGive(gParamsMutex);
    EEPROM_MarkDirty();
    sender->setRGBColor(color);
}

static void onModeCommand(int8_t index, HASelect* sender) {
    if (index < 0 || index >= (int8_t)g_modeCount) return;
    HAModeMapping m = g_modeMap[index];

    xSemaphoreTake(gParamsMutex, portMAX_DELAY);
    switch (m.type) {
        case MODE_OFF:
            GlobalParameters.MasterFireRippleEnabled = false;
            break;
        case MODE_RIPPLE_SEQ:
            GlobalParameters.MasterFireRippleEnabled = true;
            GlobalParameters.StableColorMode         = false;
            GlobalParameters.SequencerEnabled        = true;
            break;
        case MODE_STABLE_SEQ:
            GlobalParameters.MasterFireRippleEnabled = true;
            GlobalParameters.StableColorMode         = true;
            GlobalParameters.SCSeqEnabled            = true;
            break;
        case MODE_RIPPLE_PROFILE:
            GlobalParameters.MasterFireRippleEnabled = true;
            GlobalParameters.StableColorMode         = false;
            GlobalParameters.SequencerEnabled        = false;
            GlobalParameters.SequencerCurrentProfile = (unsigned char)m.slot;
            break;
        case MODE_STABLE_PRESET:
            GlobalParameters.MasterFireRippleEnabled = true;
            GlobalParameters.StableColorMode         = true;
            GlobalParameters.SCSeqEnabled            = false;
            GlobalParameters.SCSeqCurrentPreset      = (unsigned char)m.slot;
            memcpy(GlobalParameters.StableColorSegments,
                   GlobalParameters.SCPresets[m.slot].Segments,
                   NUMBER_OF_SEGMENTS);
            GlobalParameters.StableColorHue = GlobalParameters.SCPresets[m.slot].Hue;
            break;
    }
    bool master = GlobalParameters.MasterFireRippleEnabled;
    xSemaphoreGive(gParamsMutex);
    EEPROM_MarkDirty();

    sender->setState(index);
    haLight.setState(master);
}

static void onBpmCommand(HANumeric number, HANumber* sender) {
    if (!number.isSet()) return;
    int32_t bpm = number.toInt32();
    if (bpm < (int32_t)BPM_MIN || bpm > (int32_t)BPM_MAX) return;

    xSemaphoreTake(gParamsMutex, portMAX_DELAY);
    GlobalParameters.GlobalBPM = (float)bpm;
    xSemaphoreGive(gParamsMutex);
    EEPROM_MarkDirty();
    sender->setState(bpm);
}

// Compute BPM from the recent tap intervals (up to TAP_BUFFER_SIZE most recent).
static void onTapTempo(HAButton* /*sender*/) {
    unsigned long now = millis();
    if (g_tapCount > 0) {
        unsigned long lastTap = g_tapBuffer[(g_tapCount - 1) % TAP_BUFFER_SIZE];
        if (now - lastTap > TAP_TIMEOUT_MS) g_tapCount = 0;
    }
    g_tapBuffer[g_tapCount % TAP_BUFFER_SIZE] = now;
    g_tapCount++;

    if (g_tapCount < 2) return; /* need at least 2 taps */

    uint8_t taps  = (g_tapCount > TAP_BUFFER_SIZE) ? TAP_BUFFER_SIZE : g_tapCount;
    uint8_t start = (g_tapCount - taps) % TAP_BUFFER_SIZE;
    unsigned long oldest = g_tapBuffer[start];
    unsigned long span   = now - oldest;
    if (span == 0) return;

    float bpm = 60000.0f * (float)(taps - 1) / (float)span;
    if (bpm < BPM_MIN || bpm > BPM_MAX) return;

    xSemaphoreTake(gParamsMutex, portMAX_DELAY);
    GlobalParameters.GlobalBPM = bpm;
    xSemaphoreGive(gParamsMutex);
    EEPROM_MarkDirty();
    haBpm.setState((int32_t)(bpm + 0.5f));
}

void HA_init() {
    device.setName("Hexagono");
    device.setSoftwareVersion("2.1.0");
    device.setManufacturer("JD Software Labs");
    device.setModel("ESP32 Chromance");

    // Configuration URL points at the WebUI for deep-dive configuration.
    // Stored in a static buffer so the pointer remains valid for the lib's lifetime.
    static char urlBuf[64];
    IPAddress ip = WiFi.localIP();
    snprintf(urlBuf, sizeof(urlBuf), "http://%u.%u.%u.%u/", ip[0], ip[1], ip[2], ip[3]);
    device.setConfigurationUrl(urlBuf);

    haLight.setName("Hexagono Light");
    haLight.onStateCommand(onStateCommand);
    haLight.onBrightnessCommand(onBrightnessCommand);
    haLight.onRGBColorCommand(onRGBColorCommand);

    buildModeOptions();
    haMode.setName("Hexagono Mode");
    haMode.setIcon("mdi:auto-fix");
    haMode.setOptions(g_modeOptions.c_str());
    haMode.onCommand(onModeCommand);

    haBpm.setName("Hexagono BPM");
    haBpm.setIcon("mdi:metronome");
    haBpm.setMin((float)BPM_MIN);
    haBpm.setMax((float)BPM_MAX);
    haBpm.setStep(1.0f);
    haBpm.setUnitOfMeasurement("BPM");
    haBpm.setMode(HANumber::ModeSlider);
    haBpm.onCommand(onBpmCommand);

    haTapTempo.setName("Hexagono Tap Tempo");
    haTapTempo.setIcon("mdi:gesture-tap");
    haTapTempo.onCommand(onTapTempo);

    /* Seed initial states so HA shows the right thing on first connect */
    xSemaphoreTake(gParamsMutex, portMAX_DELAY);
    int8_t modeIdx = computeCurrentModeIndex_locked();
    haLight.setCurrentState(GlobalParameters.MasterFireRippleEnabled);
    haLight.setCurrentBrightness(GlobalParameters.Brightness);
    haMode.setCurrentState(modeIdx);
    haBpm.setCurrentState((int32_t)GlobalParameters.GlobalBPM);
    xSemaphoreGive(gParamsMutex);

    /* PubSubClient defaults to a 256-byte buffer; the HASelect discovery
       payload (device metadata + options array + topics) easily exceeds
       that and gets silently dropped. Bump well past anything we'd send. */
    mqttClient.setBufferSize(1024);

    mqttClient.begin("192.168.100.201", 1883, "mqtt_hexagono", "hexagono123");
    Serial.println("[HA] Home Assistant MQTT initialized.");
}

void HA_loop() {
    mqttClient.loop();

    /* Polling state sync — push current mode + BPM back to HA periodically.
       setState is a no-op when the value matches the last published one,
       so this is cheap. 500 ms is fast enough that sequencer advances feel
       responsive in the HA UI without flooding MQTT. */
    static unsigned long lastSync_ms = 0;
    unsigned long now = millis();
    if (now - lastSync_ms < 500UL) return;
    lastSync_ms = now;

    xSemaphoreTake(gParamsMutex, portMAX_DELAY);
    int8_t   modeIdx = computeCurrentModeIndex_locked();
    bool     master  = GlobalParameters.MasterFireRippleEnabled;
    uint8_t  bright  = GlobalParameters.Brightness;
    int32_t  bpm     = (int32_t)GlobalParameters.GlobalBPM;
    xSemaphoreGive(gParamsMutex);

    if (modeIdx >= 0) haMode.setState(modeIdx);
    haLight.setState(master);
    haLight.setBrightness(bright);
    haBpm.setState(bpm);
}
