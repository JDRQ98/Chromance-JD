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

HTTP REST API:

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/getCurrentProfiles` | GET | Retrieve all profiles, global params, and sequencer state |
| `/updateProfile` | POST | Update a single profile by index (JSON body) |
| `/updateGlobalParameters` | POST | Update global settings: decay, brightness, sequencer (JSON body) |
| `/restoreDefaults` | POST | Wipe EEPROM and restart with factory defaults |
| `/ManualRipple` | GET | Trigger single manual ripple burst |
| `/MasterFireRippleEnabled/on` | GET | Enable automatic ripple firing |
| `/MasterFireRippleEnabled/off` | GET | Disable automatic ripple firing |

---

## Firmware (`src/`)

### File Structure

```
src/
├── main.cpp                 # Entry point, main loop, sequencer logic
├── HTTP_Server.h/cpp        # REST API, GlobalParameters, profile management
├── ASW.h/cpp                # Effect functions, bulk ripple firing
├── WiFi_utilities.h/cpp     # WiFi, OTA, UDP debug, web server routes
├── MCAL/
│   ├── ripple.h/cpp         # Core ripple engine (100 concurrent ripples)
│   ├── mapping.h/cpp        # Topology definitions (nodes, segments, LEDs)
│   └── EEP.h/cpp            # EEPROM (Preferences) storage with debounced saves
└── Alexa/                   # Alexa/Hue Bridge integration
```

### Data Structures

**RippleProfile_struct** — per-profile settings (up to 10 profiles):
```cpp
typedef struct {
  boolean Active;
  char ProfileName[32];
  boolean ActiveNodes[19];         // Which nodes fire ripples
  rippleBehavior Behavior;         // 0-4 turn aggressiveness
  signed char Direction;           // -1 = all, 0-5 = specific direction
  unsigned long RippleLifeSpan;    // ms
  float RippleSpeed;               // LEDs per tick (pressure gain)
  short RainbowDeltaPerTick;       // Hue shift per ripple advance
  unsigned int Colors[16];         // HSV hue values (0-65535)
  unsigned int NumberOfColors;
  unsigned int CurrentColor;       // Index into Colors[]
  short DelayBetweenRipples_ms;
  unsigned long TimeLastRippleFired_ms;  // Runtime: last fire timestamp
} RippleProfile_struct;
```

**GlobalParameters_struct** — global settings + all profiles:
```cpp
typedef struct {
  boolean MasterFireRippleEnabled;
  float Decay;                             // Brightness fade per tick (0.9-1.0)
  unsigned char Brightness;                // LED brightness (0-255)
  RippleProfile_struct RippleProfiles[10]; // Up to 10 profiles
  unsigned int NumberOfActiveProfiles;
  boolean SequencerEnabled;                // Is sequencer running?
  unsigned char SequencerMode;             // 0 = sequential, 1 = random
  unsigned short SequencerDwellTime_s;     // Seconds per profile (10-120)
  unsigned char SequencerCurrentProfile;   // Current profile index
  unsigned long SequencerLastSwitch_ms;    // Runtime: last switch timestamp
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

### Default Profiles (4 presets on first boot)

| Index | Name | Nodes | Colors | Speed | Behavior |
|-------|------|-------|--------|-------|----------|
| 0 | Rainbow 7 | All (default) | 7 rainbow hues | 0.5 | feisty |
| 1 | Ocean Wave | Border nodes | Blues/cyans | 0.3 | weaksauce |
| 2 | Fire Storm | Center node (9) | Reds/oranges | 1.5 | angry |
| 3 | Forest | Quad nodes | Greens/yellows | 0.7 | feisty |

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

## Configuration Parameters

| Parameter | Min | Default | Max | Description |
|-----------|-----|---------|-----|-------------|
| `DelayBetweenRipples_ms` | 1 | 3000 | 20000 | ms between auto-fired bursts |
| `RippleLifeSpan` | 1 | 3000 | 20000 | Ripple lifetime in ms |
| `RippleSpeed` | 0.01 | 0.5 | 10 | LEDs per tick (pressure gain) |
| `Decay` | 0.9 | 0.985 | 1.0 | Brightness fade per tick |
| `Behavior` | 0 | 1 (feisty) | 4 | Turn aggressiveness |
| `Direction` | -1 | -1 (all) | 5 | Starting direction |
| `RainbowDeltaPerTick` | 0 | 200 | 2000 | Hue shift per ripple advance |
| `NumberOfColors` | 1 | 7 | 16 | Colors in palette |
| `Brightness` | 0 | 128 | 255 | Global LED brightness |
| `SequencerDwellTime_s` | 10 | 30 | 120 | Seconds per profile in sequencer |

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
