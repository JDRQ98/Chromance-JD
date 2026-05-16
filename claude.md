# Hexachrome Project

ESP32-based LED art installation with 19-node hexagonal topology driving 330 WS2812B LEDs via a web-based profile manager.

## Quick Start

| Component | Path |
|-----------|------|
| Firmware | `src/` (Arduino/PlatformIO for ESP32, submodule) |
| WebUI | `data/` (Vanilla JS, served via LittleFS, submodule) |
| Device Host | `hexagono.local` (ESP32 mDNS) |

**Key Entry Points:**
- Firmware: `src/main.cpp` → `setup()` and `loop()`
- Landing page: `data/index.html` → loads `data/js/landing.js`
- Profile editor: `data/ProfileEditor.html` → loads `data/js/main.js`

**Build:**
- PlatformIO environment: `esp32dev_OTA`
- Filesystem: LittleFS (not SPIFFS — SPIFFS has 32-char filename limit)
- Flash firmware first, then filesystem image

---

## Architecture Overview

### Physical Topology

```
          [ 0 ]                    <- Border node (2 connections)
         /     \
      [1]       [2]                <- Quad nodes (4 connections)
     / | \     / | \
  [3]  |  [ 4 ]  |  [5]           <- Mix: Border [3,5], Tri/Cube [4]
   |   |    |    |   |
   |  [6]   |   [7]  |            <- Tri/Cube nodes [6, 7]
   | /   \  |  /   \ |
  [8]     [ 9 ]     [10]          <- Mix: Quad [8,10], [9] = Center (6 connections)
   | \   /  |  \   / |
   |  [11]  |   [12] |            <- Tri/Cube [11,12]
   |   |    |     |  |
  [13] |  [ 14]   | [15]          <- Mix: Border [13,15], Tri/Cube [14]
    \  | /     \  | /
     [16]       [17]               <- Quad nodes [16, 17]
         \     /
          [ 18]                    <- Border node [18]
```

- **19 nodes** connected by **30 segments**
- **330 LEDs** total (2 strips x 165 LEDs on GPIO 33 and 32)
- **11 LEDs per segment**

**Node Types by Connection Count:**
| Type | Nodes | Connections |
|------|-------|-------------|
| Border | 0, 3, 5, 13, 15, 18 | 2 |
| CubePair (120 deg apart) | 6, 7, 14 | 3 |
| CubeOdd (120 deg apart) | 4, 11, 12 | 3 |
| Quad | 1, 2, 8, 10, 16, 17 | 4 |
| Center (Starburst) | 9 | 6 |

### Communication Protocol

HTTP REST API (HTTP) for the WebUI + MQTT (ArduinoHA) for Home Assistant:

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/getCurrentProfiles` | GET | Retrieve all ripple profiles, SC presets, global params, sequencer state |
| `/updateProfile` | POST | Update or delete a single ripple profile by index |
| `/updateGlobalParameters` | POST | Batch-update global settings (decay, brightness, master, sequencers, stable color, BPM) |
| `/updateStableColorPreset` | POST | Update a single SC preset by index |
| `/ManualRipple` | GET | Trigger single manual ripple burst |
| `/MasterFireRippleEnabled/on\|off` | GET | Enable/disable master fire |
| `/resetMicrocontroller` | GET | Soft restart |
| `/clearEEPROM` | GET | Wipe EEPROM (restart re-creates factory defaults) |
| `/update` | GET (ElegantOTA) | OTA firmware update UI |

---

## Firmware (`src/`)

### File Structure

```
src/
├── main.cpp                 # Entry point, main loop, sequencer logic
├── HTTP_Server.h/cpp        # REST API, GlobalParameters, profile management
├── ASW.h/cpp                # Effect functions, bulk ripple firing
├── WiFi_utilities.h/cpp     # WiFi, OTA, UDP debug, web server routes
└── MCAL/
    ├── ripple.h/cpp         # Core ripple engine (100 concurrent ripples)
    ├── mapping.h/cpp        # Topology definitions (nodes, segments, LEDs)
    └── EEP.h/cpp            # EEPROM (Preferences) storage with debounced saves
```

### Data Structures

**TimeEvent_struct** — one scheduled fire event within a profile's period (up to 5 per profile):
```cpp
typedef struct {
  boolean Enabled;
  unsigned short TimeOffset_ms;           // When to fire (ms from period start)
  unsigned long RippleLifeSpan;           // Per-event lifespan (ms)
  unsigned char RippleType;               // 0=single, 1=double, 2=shard
  rippleBehavior Behavior;                // 0-4
  float RippleSpeed;                      // LEDs per tick
  short RainbowDeltaPerTick;              // Hue delta per advance
  signed char Direction;                  // -1=all, 0-5
  boolean ActiveNodes[19];                // Per-event node selection
} TimeEvent_struct;
```

**RippleProfile_struct** — period-based, multi-event profile (up to 10 profiles):
```cpp
typedef struct {
  boolean Active;
  char ProfileName[32];
  unsigned int Colors[16];                 // HSV hues, shared across events
  unsigned int NumberOfColors;
  unsigned int CurrentColor;               // Runtime: cycles through Colors[]
  unsigned long ProfilePeriod_ms;          // Total period duration
  TimeEvent_struct Events[5];              // t0-t4 scheduled events
  unsigned long PeriodStartTime_ms;        // Runtime
  boolean EventFired[5];                   // Runtime
} RippleProfile_struct;
```

**StableColorPreset_struct** — solid-color preset (up to 8):
```cpp
typedef struct {
  boolean Active;
  char PresetName[32];
  unsigned int Hue;                        // 16-bit HSV hue
  boolean Segments[30];                    // Per-segment selection
} StableColorPreset_struct;
```

**GlobalParameters_struct** — global settings + all profiles + all SC presets:
```cpp
typedef struct {
  boolean MasterFireRippleEnabled;
  float Decay;                             // Brightness fade per tick
  unsigned char Brightness;                // LED brightness (0-255)
  RippleProfile_struct RippleProfiles[10];
  unsigned int NumberOfActiveProfiles;

  // Ripple-profile sequencer
  boolean SequencerEnabled;
  unsigned char SequencerMode;             // 0=sequential, 1=random
  unsigned short SequencerDwellTime_s;     // 10-120
  unsigned char SequencerCurrentProfile;
  unsigned long SequencerLastSwitch_ms;    // Runtime
  float GlobalBPM;                         // Used by SC sequencer beat mode

  // Stable color mode (bypasses ripple engine, paints solid color w/ optional pulse)
  boolean StableColorMode;
  unsigned int StableColorHue;
  unsigned char StableColorSat;
  float PulseFrequency;
  float PulseDepth;
  boolean StableColorSegments[30];

  // Stable color preset sequencer (time/beat/fps timing modes, fade transitions)
  boolean SCSeqEnabled;
  unsigned char SCSeqMode;                 // 0=sequential, 1=random
  unsigned char SCSeqTimingMode;           // 0=time, 1=pulse, 2=fps
  float SCSeqDwellTime_s;
  float SCSeqBeatsPerSwitch;
  unsigned char SCSeqFPS;
  boolean SCSeqCycleColors;
  boolean SCSeqFadeEnabled;
  boolean SCSeqFadeOuter, SCSeqFadeInner;
  unsigned short SCSeqFadeDuration_ms;
  unsigned char SCSeqCurrentPreset;
  unsigned long SCSeqLastSwitch_ms;        // Runtime
  unsigned char NumberOfSCPresets;
  // ... runtime fade state ...
  StableColorPreset_struct SCPresets[8];
} GlobalParameters_struct;
```

### Color System
- Firmware stores colors as **16-bit HSV hue** (0-65535) internally
- WebUI sends/receives **RGB hex** (`#RRGGBB`)
- Conversion at API boundary in `HTTP_Server.cpp`: `rgbHexToHue16()` / `hue16ToRgbHex()`

### Ripple Engine (`MCAL/ripple.h`, `MCAL/ripple.cpp`)

Supports up to 100 concurrent ripples (`MAX_NUMBER_OF_RIPPLES`).

**Ripple States** (`rippleState` enum): `dead`, `withinNode`, `travelingUpwards`, `travelingDownwards`

**Behavior Modes** (`rippleBehavior` enum):
| Value | Name | Turn Preference |
|-------|------|-----------------|
| 0 | weaksauce | Prefers straight, then 60 deg, then 120 deg |
| 1 | feisty | Similar but more aggressive |
| 2 | angry | Takes sharp 120 deg turns readily |
| 3 | alwaysTurnsRight | Always picks rightmost valid path |
| 4 | alwaysTurnsLeft | Always picks leftmost valid path |

**Key Functions:**
- `Ripple_MainFunction()` — fades LEDs, advances all ripples, calls `strips[].show()`
- `FireRipple()` — spawn single ripple from a node
- `FireDoubleRipple()` — two diverging ripples from same node
- `FireShard()` — two adjacent-direction ripples
- `Ripple_KillAllRipples()` — clear all active ripples

**Pressure System:** Ripples accumulate pressure scaled by delta-time (`deltaTime / 16.0f` for 60fps baseline). When `pressure >= 1`, ripple advances one LED. This makes animation speed consistent regardless of loop rate.

**Public Setters** (for reactive updates): `setSpeed()`, `setBehavior()`, `setHueDeltaPerTick()`

**Extern:** `ripples[]` array is accessible from other modules for iterating in-flight ripples.

### Firmware-Side Sequencer

The sequencer cycles through active profiles autonomously, without WebUI involvement during playback.

**Design:** Uses a "gate" pattern in the main loop — when sequencer is enabled, only fires ripples for `SequencerCurrentProfile`. Profile `Active` flags are NOT modified, so disabling the sequencer returns to normal multi-profile behavior.

**Logic (in `main.cpp` loop):**
1. Check if dwell time has elapsed since last switch
2. Advance to next profile (sequential wrap-around or random non-repeat)
3. Gate: skip ripple firing for profiles that aren't the current sequencer profile

**Boot behavior:** `SequencerLastSwitch_ms` is reset to `millis()` in `setup()` to prevent stale EEPROM timestamps from causing immediate switches.

### Reactive Profile Updates

When profile properties are updated via REST API, the firmware reacts immediately:

| Property Changed | Reaction |
|-----------------|----------|
| Direction, Colors | Kill all ripples + reset fire timer (immediate refire) |
| Speed | Apply to all in-flight ripples via `setSpeed()` |
| Behavior | Apply to all in-flight ripples via `setBehavior()` |
| RainbowDeltaPerTick | Apply to all in-flight ripples via `setHueDeltaPerTick()` |
| ActiveNodes (new nodes) | Fire catch-up ripples with adjusted lifespan (`RippleLifeSpan - elapsed`) |
| Decay | Applies naturally (global fade multiplier) |
| DelayBetweenRipples, RippleLifeSpan, Brightness, Sequencer | No reactive action |

### Default Ripple Profiles (4 presets on first boot, only Rainbow 7 active by default)

| Index | Name | Period | Events | Notes |
|-------|------|--------|--------|-------|
| 0 | Rainbow 7 | 3000 ms | 1 single from center | Rainbow palette with hue delta |
| 1 | Cyclone | 4000 ms | 2 simultaneous: pair cubes (alwaysTurnsRight), odd cubes (alwaysTurnsLeft) | Counter-rotating spirals |
| 2 | Bloom | 2000 ms | 3 DOUBLE bursts from center, rotated 120° at offsets 0/666/1333 ms | Flower bloom pattern |
| 3 | Heartbeat | 2000 ms | 2 SHARD bursts from border nodes (lub-DUB at 0 ms / 250 ms) | Pulsing rhythm |

### Default Stable Color Presets (4 active on first boot)

| Index | Name | Segments | Hue |
|-------|------|----------|-----|
| 0 | Solid | All 30 | Blue |
| 1 | Even Segs | 0,2,...,28 | Cyan |
| 2 | Odd Segs | 1,3,...,29 | Purple |
| 3 | Sparse | Every 3rd | Orange |

### EEPROM / Preferences (`MCAL/EEP.h`, `MCAL/EEP.cpp`)

- Uses ESP32 Preferences library (NVS) with namespace `"chromance"`, key `"GlobalConfig"`
- First-time run detected by absence of `"nvsInit"` key → writes factory defaults
- **Struct size change detection:** If `getBytes()` returns fewer bytes than `sizeof(GlobalParameters)`, forces a clear + restart (auto-wipe on struct changes during development)
- **Debounced saves:** `EEPROM_MarkDirty()` sets a dirty flag; `EEPROM_DebouncedSave()` (called in main loop) writes after 5-second cooldown (`EEPROM_SAVE_COOLDOWN_MS`)
- Auto-save triggered after profile updates and global parameter changes

### Topology (`MCAL/mapping.h`, `MCAL/mapping.cpp`)

```cpp
int nodeConnections[19][6];     // Per node: segment ID at each of 6 directions (-1 = none)
int segmentConnections[30][2];  // Per segment: [ceiling_node, floor_node]
int ledAssignments[30][3];      // Per segment: [strip_index, ceiling_led, floor_led]
```

**Direction Indexing:** 0=0 deg (12 o'clock), 1=60 deg, 2=120 deg, 3=180 deg, 4=240 deg, 5=300 deg

**Constants:** `NUMBER_OF_STRIPS=2`, `NUMBER_OF_SEGMENTS=30`, `NUMBER_OF_NODES=19`, `NUMBER_OF_LEDS_PER_SEGMENT=11`

### Effects (`ASW.h`, `ASW.cpp`)

Bulk ripple firing patterns using generic `FireRipple_FromNodes()` helper:

| Function | Description |
|----------|-------------|
| `FireRipple_CenterNode()` | Fire from node 9 (all 6 directions if dir=-1) |
| `FireRipple_AllBorderNodes()` | Fire from nodes 0,3,5,13,15,18 |
| `FireRipple_AllQuadNodes()` | Fire from nodes 1,2,8,10,16,17 |
| `FireRipple_AllPairCubeNodes()` | Fire from nodes 6,7,14 (directions 0,2,4) |
| `FireRipple_AllOddCubeNodes()` | Fire from nodes 4,11,12 (directions 1,3,5) |

---

## WebUI (`data/`)

### File Structure

```
data/
├── index.html               # Landing page (profile manager)
├── ProfileEditor.html        # Profile editor page
├── css/
│   └── landing.css           # Styles for landing page
└── js/
    ├── landing.js             # Landing page logic (profiles, brightness, sequencer)
    ├── main.js                # Profile editor logic
    ├── modalManager.js        # Node properties modal
    ├── nodeManager.js         # Node grid management
    └── drawVisualizer.js      # Hexagon segment visualizer
```

### Landing Page (`index.html` + `landing.js`)

The main interface for managing profiles and global controls:
- **Profile cards** with color previews, hex grid visualization, and select/edit buttons
- **Brightness slider** (0-255) with real-time updates
- **Sequencer controls**: enable/disable toggle, mode (sequential/random), dwell time slider (10-120s)
- **Status indicator** showing current sequencer profile
- Restore defaults button with confirmation modal

### Profile Editor (`ProfileEditor.html` + `main.js`)

Per-profile editing with hex grid node selection:
- Click nodes to select/activate for ripple firing
- Node properties modal for per-node overrides
- Color palette management
- Settings: speed, lifespan, delay, behavior, direction, rainbow delta

---

## Home Assistant Integration (`HomeAssistant_Integration.{cpp,h}`)

MQTT-based integration using the `ArduinoHA` library (replaced the old fauxmoESP/Hue-Bridge-emulation Alexa path).

**MQTT broker:** hardcoded at `192.168.100.201:1883` with credentials `mqtt_hexagono:hexagono123` — change in `HA_init()` if your broker moves.

**Exposed HA entities (under device "Hexagono"):**

| Entity | Type | Purpose |
|--------|------|---------|
| `light.hexagono` | HALight (state/brightness/RGB) | Master on/off, brightness slider, hue (writes `StableColorHue` only — does NOT force Stable mode) |
| `select.hexagono_mode` | HASelect | Unified mode picker: `Off`, `Ripple Sequence`, `Stable Sequence`, `Ripple: <name>` (per active ripple profile), `Stable: <name>` (per active SC preset) |
| `number.hexagono_bpm` | HANumber | Sets `GlobalBPM` (40-300), used by SC sequencer beat mode |
| `button.hexagono_tap_tempo` | HAButton | Tap-tempo button (ring buffer of last 4 taps → computed BPM) |

**Configuration URL:** `HADevice::setConfigurationUrl("http://<ip>/")` — HA renders a "Visit Device" link on the device page pointing at the WebUI.

**State sync:** `HA_loop()` polls GlobalParameters every 500 ms and publishes mode/brightness/BPM. `setState` is a no-op when the value matches the last published one, so the polling is cheap. This means HA reflects WebUI changes and sequencer advances within ~500 ms.

**Limitation:** `HASelect::setOptions()` can only be called once per the ArduinoHA contract. The Mode option list is built at boot from the currently-active profile/preset names. Renaming a profile in the WebUI requires a device restart to update the HA dropdown.

---

## Configuration Parameters

Per-event (`TimeEvent_struct`):

| Parameter | Min | Default | Max | Description |
|-----------|-----|---------|-----|-------------|
| `TimeOffset_ms` | 0 | 0 | <ProfilePeriod | When the event fires within the period |
| `RippleLifeSpan` | 1 | 5000 | 20000 | Ripple lifetime in ms |
| `RippleType` | 0 | 0 | 2 | 0=single, 1=double, 2=shard |
| `RippleSpeed` | 0.01 | 0.5 | 10 | LEDs per tick (pressure gain) |
| `Behavior` | 0 | 1 (feisty) | 4 | weaksauce/feisty/angry/right/left |
| `Direction` | -1 | -1 (all) | 5 | Starting direction |
| `RainbowDeltaPerTick` | 0 | 200 | 2000 | Hue shift per ripple advance |

Per-profile:

| Parameter | Min | Default | Max | Description |
|-----------|-----|---------|-----|-------------|
| `ProfilePeriod_ms` | 500 | 5000 | 60000 | Total period duration |
| `NumberOfColors` | 1 | 7 | 16 | Colors in palette |

Global:

| Parameter | Min | Default | Max | Description |
|-----------|-----|---------|-----|-------------|
| `Decay` | 0.9 | 0.985 | 1.0 | Brightness fade per tick |
| `Brightness` | 0 | 128 | 255 | Global LED brightness |
| `SequencerDwellTime_s` | 10 | 30 | 120 | Seconds per profile in sequencer |
| `GlobalBPM` | 40 | 120 | 300 | Used by SC sequencer beat-mode and HA |
| `SCSeqDwellTime_s` | 0.5 | 30 | 120 | SC preset dwell (time mode) |
| `SCSeqFadeDuration_ms` | 5 | 300 | 2000 | Per-half fade duration |

---

## Debugging

### Firmware

- **Serial**: 115200 baud, outputs connection status and errors
- **UDP Logging**: Sends to `192.168.100.18:8888` via `udp_printf()`
- **Monitor script**: `scripts/UDP_Monitor.py`
- **Debug Flags** (ripple.h): Uncomment to enable:
  ```cpp
  // #define DEBUG_ADVANCEMENT
  // #define DEBUG_RENDERING
  // #define DEBUG_PRESSURE
  ```
- **EEPROM Debug**: `#define EEPROM_DEBUGGING true` in EEP.cpp

### WebUI

- Console logs throughout - search for `console.log` in js files
- Network tab to inspect POST payloads to `hexagono.local`

---

## Dependencies

### Firmware (platformio.ini)
- Adafruit_NeoPixel
- ESPAsyncWebServer
- ArduinoJson
- WiFiManager
- ElegantOTA
- LittleFS (board_build.filesystem = littlefs)

### WebUI
- No external dependencies (vanilla JS)

---

## Development Notes

### ASW.h vs ripple.h Type Mismatch
- `ASW.h` declares functions with `byte behavior`
- `ripple.h` declares `FireRipple`/`FireDoubleRipple`/`FireShard` with `rippleBehavior behavior`
- Need explicit `(rippleBehavior)` cast when calling ripple.h functions from byte-typed wrappers

### Web Server Routes
- All static file routes must be explicitly registered in `WiFi_utilities.cpp setupWebServer()`
- Adding a new JS/CSS file requires adding a corresponding route

### EEPROM Struct Changes
- Any change to `GlobalParameters_struct` or `RippleProfile_struct` changes `sizeof()`, triggering auto-wipe on next boot
- This is acceptable during development but means all saved profiles are lost

---

## Branches

### `WebSocketRework` (not merged)
Contains significant WebUI refactoring work:
- WebSocket communication (bidirectional real-time updates)
- Event bus architecture (`eventBus.js`)
- Centralized state store (`stateStore.js`)
- WebUI-based effect sequencer with playlist/transitions
- Canvas-based live ripple preview
- Modular component architecture

### `AmazonEcho_Support`
Current firmware branch with Alexa/Hue Bridge integration.
