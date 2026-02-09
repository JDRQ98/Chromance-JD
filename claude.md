# Hexachrome Project

ESP32-based LED art installation with 19-node hexagonal topology driving 330 WS2812B LEDs via a web-based effect editor.

## Quick Start

| Component | Path |
|-----------|------|
| Firmware | `Chromance-JD/` (Arduino/PlatformIO for ESP32) |
| WebUI | `Chromance-WebUI/` (Vanilla JS SPA) |
| Device Host | `hexagono.local` (ESP32 mDNS) |

**Key Entry Points:**
- Firmware: `Chromance-JD/main.cpp` → `setup()` and `loop()`
- WebUI: `Chromance-WebUI/EffectEditor.html` → loads `js/main.js`

---

## Architecture Overview

### Physical Topology

```
          [ 0 ]                    ← Border node (2 connections)
         /     \
      [1]       [2]                ← Quad nodes (4 connections)
     / | \     / | \
  [3]  |  [ 4 ]  |  [5]             ← Mix: Border [3,5] (2 connenctions), Tri/Cube [4] (3 connections)
   |   |    |    |   |
   |  [6]   |   [7]  |              ← Tri/Cube nodes [6, 7] (3 connections)
   | /   \  |  /   \ |
  [8]     [ 9 ]     [10]            ← Mix: Quad nodes [8,10] (4 connenctions), [9] = Center (6 connections)
   | \   /  |  \   / |
   |  [11]  |   [12] |             ← Tri/Cube [11,12]
   |   |    |     |  |
  [13] |  [ 14]   | [15]          ← Mix: Border nodes [8,10] (2 connenctions), Tri/Cube [14] (3 connections)
    \  | /     \  | /
     [16]       [17]           ← Quad nodes [16, 17]
         \     /
          [ 18]                 ← Border node [18] (2 connections)
```

- **19 nodes** connected by **30 segments**
- **330 LEDs** total (2 strips × 165 LEDs on GPIO 33 and 32)
- **11 LEDs per segment**

**Node Types by Connection Count:**
| Type | Nodes | Connections |
|------|-------|-------------|
| Border | 0, 3, 5, 13, 15, 18 |
| CubePair (120° apart) | 6, 7, 14 |
| CubeOdd (120° apart) | 4, 11, 12 |
| Quad | 1, 2, 8, 10, 16, 17 |
| Center (Starburst) | 9 |

### Communication Protocol

HTTP REST API (not WebSocket):

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/getInternalVariables` | GET | Retrieve current settings with min/max bounds |
| `/updateInternalVariables` | POST | Send JSON with globalSettings, activeNodes, nodeSpecificSettings |
| `/ManualRipple` | GET | Trigger single manual ripple burst |
| `/MasterFireRippleEnabled/on` | GET | Enable automatic ripple firing |
| `/MasterFireRippleEnabled/off` | GET | Disable automatic ripple firing |

---

## Firmware (Chromance-JD)

### File Structure

```
Chromance-JD/
├── main.cpp                 # Entry point, main loop
├── HTTP_Server.h/cpp        # REST API, GlobalParameters
├── ASW.h/cpp                # Effect functions, bulk ripple firing
├── WiFi_utilities.h/cpp     # WiFi, OTA, UDP debug
├── MCAL/
│   ├── ripple.h/cpp         # Core ripple engine
│   ├── mapping.h/cpp        # Topology definitions
│   └── EEP.h/cpp            # EEPROM profile storage
└── Alexa/                   # Alexa/Hue Bridge integration
```

### Ripple Engine (`MCAL/ripple.h`, `MCAL/ripple.cpp`)

The core animation system. Supports up to 100 concurrent ripples.

**Ripple States** (`rippleState` enum):
```cpp
enum rippleState {
    dead,              // Inactive, available for reuse
    withinNode,        // Passing through node (invisible, maintains momentum)
    travelingUpwards,  // Moving up a segment (rendered)
    travelingDownwards // Moving down a segment (rendered)
};
```

**Behavior Modes** (`rippleBehavior` enum):
| Value | Name | Turn Preference |
|-------|------|-----------------|
| 0 | weaksauce | Prefers straight, then 60°, then 120° |
| 1 | feisty | Similar but more aggressive |
| 2 | angry | Takes sharp 120° turns readily |
| 3 | alwaysTurnsRight | Always picks rightmost valid path |
| 4 | alwaysTurnsLeft | Always picks leftmost valid path |

**Direction Bias** (`directionBias` enum):
- `noPreference`, `preferLeft`, `preferRight`
- `preferLeftOnce/Twice`, `preferRightOnce/Twice`

**Key Functions:**
- `Ripple_MainFunction()` - Main loop: fades LEDs, advances all ripples, calls `strips[].show()`
- `FireRipple(ripple*, dir, color, node, speed, lifespan, behavior, ...)` - Spawn single ripple
- `FireDoubleRipple()` - Two diverging ripples from same node
- `FireShard()` - Two adjacent-direction ripples
- `Ripple_KillAllRipples()` - Clear all active ripples

**Pressure System** (line ~136 in ripple.h):
Ripples accumulate "pressure" each tick: `pressure += fmap(age, 0, lifespan, speed, speed/2)`. When `pressure >= 1`, ripple advances one LED. This decouples animation from loop frequency.

### Topology (`MCAL/mapping.h`, `MCAL/mapping.cpp`)

**Core Data Structures:**

```cpp
// For each node, segment ID at each of 6 directions (clockwise from 12 o'clock)
// -1 = no connection at that direction
int nodeConnections[NUMBER_OF_NODES][6];  // [19][6]

// For each segment, [ceiling_node, floor_node]
int segmentConnections[NUMBER_OF_SEGMENTS][2];  // [30][2]

// For each segment, [strip_index, ceiling_led, floor_led]
int ledAssignments[NUMBER_OF_SEGMENTS][3];  // [30][3]
```

**Direction Indexing:**
- 0 = 0° (12 o'clock)
- 1 = 60°
- 2 = 120°
- 3 = 180° (6 o'clock)
- 4 = 240°
- 5 = 300°

**Constants:**
```cpp
#define NUMBER_OF_STRIPS 2
#define NUMBER_OF_SEGMENTS 30
#define NUMBER_OF_NODES 19
#define NUMBER_OF_LEDS_PER_SEGMENT 11
```

### Effects (`ASW.h`, `ASW.cpp`)

Bulk ripple firing patterns:

| Function | Description |
|----------|-------------|
| `FireRipple_CenterNode()` | Fire from node 9 (all 6 directions if dir=-1) |
| `FireRipple_AllBorderNodes()` | Fire from nodes 0,3,5,13,15,18 |
| `FireRipple_AllQuadNodes()` | Fire from nodes 1,2,8,10,16,17 |
| `FireRipple_AllPairCubeNodes()` | Fire from nodes 6,7,14 (directions 0,2,4) |
| `FireRipple_AllOddCubeNodes()` | Fire from nodes 4,11,12 (directions 1,3,5) |
| `FireEffect_Star()` | Outward star pattern from center |
| `FireEffect_Random()` | Random effect selection |

### HTTP Server (`HTTP_Server.h`, `HTTP_Server.cpp`)

**GlobalParameters Structure** (line 55-68 in HTTP_Server.h):
```cpp
typedef struct {
    boolean loop_MasterFireRippleEnabled;
    unsigned char currentStartingNode;
    unsigned char currentBehavior;          // 0-4
    signed char currentDirection;           // -1 = all, 0-5 = specific
    short currentDelayBetweenRipples;       // ms
    unsigned long currentRippleLifeSpan;    // ms
    float currentRippleSpeed;               // LEDs per tick
    float currentDecay;                     // brightness fade multiplier
    unsigned int currentColor;              // RGB hex
    short currentRainbowDeltaPerPeriod;
    short currentRainbowDeltaPerTick;
    unsigned char currentNumberofColors;
} GlobalParameters_struct;
```

---

## WebUI (Chromance-WebUI)

### File Structure

```
Chromance-WebUI/
├── EffectEditor.html        # Main page with node grid and modals
├── css/index.css            # Styles, node animations
└── js/
    ├── main.js              # App init, node positioning, HTTP communication
    ├── nodeManager.js       # Node class, NodeManager, state management
    ├── effectsManager.js    # Effect CRUD, localStorage persistence
    ├── modalManager.js      # Node properties modal with override checkboxes
    ├── globalSettingsManager.js  # Global effect settings modal
    ├── colorUtils.js        # Rainbow/random/similar color generation
    └── drawVisualizer.js    # Segment connection lines
```

### State Management

**Node States:**
| State | CSS Class | Visual | Meaning |
|-------|-----------|--------|---------|
| `inactive` | `regularNode` | Black | Default |
| `selected` | `SelectedNode` | Red | User clicked for editing |
| `active` | `ActiveNode` | Pulsing rainbow | Will fire ripples |
| `activeandselected` | `ActiveandSelectedNode` | Pulsing + red | Both |

**Data Objects:**
- `globalSettings` - Shared settings for all active nodes
- `nodeSpecificSettings[nodeId]` - Per-node overrides
- `effects[effectId]` - Saved effects with name, globalSettings, nodeSpecificSettings, activeNodes
- All persisted to `localStorage`

**Data Flow:**
1. User edits settings in modals
2. `updateCurrentEffect()` saves to `effects` object
3. `localStorage.setItem('effects', ...)` persists
4. "Apply Changes" → `sendConfigurationToMicrocontroller()` POSTs to ESP32

### Key Classes

**Node** (nodeManager.js):
- `toggleSelect()`, `activate()`, `deactivate()`
- `applyColorAnimation(colors)` - Sets CSS custom properties for pulse animation

**NodeManager** (nodeManager.js):
- `nodes[]`, `selectedNodes[]`, `activeNodes[]`
- `selectBiNodes()`, `selectTriNodes()`, `selectQuadNodes()`
- `getActiveNodes()`, `setActiveNodes(ids)`

**Modal** (modalManager.js):
- Handles node-specific settings with override checkboxes
- `loadNodeSettings(node)`, `saveNodeSettings()`, `discardNodeSettings()`
- Snapshot/restore for cancel functionality

---

## Configuration Parameters

| Parameter | Min | Default | Max | Description |
|-----------|-----|---------|-----|-------------|
| `currentDelayBetweenRipples` | 1 | 3000 | 20000 | ms between auto-fired bursts |
| `currentRippleLifeSpan` | 1 | 3000 | 20000 | Ripple lifetime in ms |
| `currentRippleSpeed` | 0.01 | 0.5 | 10 | LEDs per tick (pressure gain) |
| `currentDecay` | 0.9 | 0.985 | 1.0 | Brightness fade per tick |
| `currentBehavior` | 0 | 1 | 4 | Turn aggressiveness (0=weak, 4=always left) |
| `currentDirection` | -1 | -1 | 5 | Starting direction (-1=all) |
| `currentRainbowDeltaPerTick` | 0 | 200 | 2000 | Hue shift per ripple advance |
| `currentNumberofColors` | 1 | 7 | 64 | Colors in palette cycle |

---

## Common Tasks

### Adding a New Effect Type

1. **ASW.cpp**: Add function following `FireEffect_*` pattern:
   ```cpp
   bool FireEffect_MyEffect(unsigned char* currentRipple, int color) {
       FireRipple(currentRipple, dir, color, node, ...);
       return true;
   }
   ```
2. **ASW.cpp**: Add to `FireEffect_Random()` switch case
3. **HTTP_Server.cpp**: Add endpoint handler if exposing via API

### Modifying Topology

1. **mapping.cpp**: Update `nodeConnections[]` and `segmentConnections[]`
2. **mapping.h**: Update `NUMBER_OF_NODES`, `NUMBER_OF_SEGMENTS`
3. **main.js**: Update `calculateNodePositions()` for new node positions
4. **drawVisualizer.js**: Update `drawHexagon()` with new segment connections
5. **nodeManager.js**: Update node category arrays (`borderNodes`, etc.)

### Adding a New Setting

1. **HTTP_Server.h**: Add to `GlobalParameters_struct`
2. **HTTP_Server.h**: Add `HTTP_NEWSETTING_MIN/DEFAULT/MAX` defines
3. **HTTP_Server.cpp**: Add parsing in `handle_UpdateInternalVariables_body()`
4. **HTTP_Server.cpp**: Add to `handle_getInternalVariables()` response
5. **EffectEditor.html**: Add input in global settings modal
6. **globalSettingsManager.js**: Add to `globalSettings` object and `resetGlobalSettings()`
7. **main.js**: Include in `sendConfigurationToMicrocontroller()` payload

---

## Debugging

### Firmware

- **Serial**: 115200 baud, outputs connection status and errors
- **UDP Logging**: Sends to `192.168.100.18:8888` via `udp_printf()`
- **Debug Flags** (ripple.h): Uncomment to enable:
  ```cpp
  // #define DEBUG_ADVANCEMENT
  // #define DEBUG_RENDERING
  // #define DEBUG_PRESSURE
  ```

### WebUI

- Console logs throughout - search for `console.log` in js files
- Check localStorage: `localStorage.getItem('effects')` in DevTools
- Network tab to inspect POST payloads to `hexagono.local`

---

## Dependencies

### Firmware (platformio.ini)
- Adafruit_NeoPixel
- ESPAsyncWebServer
- ArduinoJson
- WiFiManager
- ElegantOTA
- SPIFFS

### WebUI
- No external dependencies (vanilla ES6 modules)

---

## Improvement Opportunities

### ~~1. WebSocket Communication~~ (IN PROGRESS - ~90%)
**Status**: Firmware implementation complete, WebUI client complete, needs end-to-end testing.
**Completed**: `WebSocket_Server.h/cpp` (firmware), `wsClient.js` (WebUI)
**Remaining**: Test integration, add activeNodes/nodeSpecificSettings handling in firmware

### 2. DRY Ripple Firing Functions
**Current**: `FireRipple_AllBorderNodes()`, `AllQuadNodes()`, etc. have repetitive loop structures.
**Proposed**: Generic `FireRipple_ByNodeType(nodeArray, directionArray)`.
**Files**: `ASW.cpp` (lines 45-179)

### 3. State Synchronization
**Current**: WebUI sends settings; firmware may drift without bidirectional sync.
**Proposed**: Fetch current state on WebUI load via `/getInternalVariables`, implement state hash for conflict detection.
**Files**: `main.js`, `HTTP_Server.cpp`, `globalSettingsManager.js`

### 4. Live Preview in WebUI
**Current**: CSS-only node animations, no actual ripple path visualization.
**Proposed**: Canvas-based animation showing ripples traversing segments using topology data.
**Files**: `drawVisualizer.js` (expand), new `rippleSimulator.js`

### ~~5. Separation of Concerns~~ (IMPLEMENTED)
**Status**: Fully implemented. See "WebUI Architecture" section below.

### 6. Time-Invariant Animation
**Current**: Ripple motion affected by loop speed (noted TODO in ripple.h:136).
**Proposed**: Use `millis()` delta for pressure calculation instead of per-tick increment.
**Files**: `MCAL/ripple.h` (advance method ~line 135)

### ~~7. Effect Composition & Sequencing~~ (IMPLEMENTED)
**Status**: Implemented in WebUI. See "Effect Sequencer" section below.

---

## WebUI Architecture (REFACTORED)

### Overview

The WebUI has been refactored from a tightly-coupled architecture to an event-driven modular system with centralized state management.

### Directory Structure

```
Chromance-WebUI/js/
├── core/                    # Core infrastructure
│   ├── eventBus.js          # Pub/sub event system
│   ├── stateStore.js        # Centralized state with localStorage
│   ├── wsClient.js          # WebSocket client with auto-reconnect
│   └── effectSequencer.js   # Sequencer engine
├── managers/                # UI managers
│   ├── main.js              # Bootstrap entry point
│   ├── modalManager.js      # Node properties modal
│   ├── nodeManager.js       # Node grid management
│   ├── effectsManager.js    # Effect CRUD
│   ├── globalSettingsManager.js  # Global settings modal
│   └── sequenceManager.js   # Sequence panel
├── components/              # Reusable components
│   ├── colorPaletteManager.js    # Color swatch component
│   └── sequenceEditorModal.js    # Sequence editor
├── utils/                   # Utilities
│   └── settingsUtils.js     # Setting name/ID helpers
├── colorUtils.js            # Color generation
└── drawVisualizer.js        # Hexagon visualizer
```

### Key Patterns

**Event Bus** (`eventBus.js`):
```javascript
// Subscribe to events
eventBus.subscribe(Events.NODES_ACTIVATED, ({ nodeIds }) => { ... });

// Publish events
eventBus.publish(Events.SETTINGS_CHANGED, { type: 'global', settings });
```

**State Store** (`stateStore.js`):
- Single source of truth for all application state
- Automatic persistence to localStorage
- Publishes events on state changes
- CRUD operations for effects and sequences

**WebSocket Client** (`wsClient.js`):
- Auto-reconnect with exponential backoff (1s to 30s)
- Message queue for offline messages
- ACK-based message confirmation with 5s timeout
- Convenience methods: `updateConfig()`, `getState()`, `fireRipple()`

---

## Effect Sequencer (NEW)

### Overview

The Effect Sequencer allows users to create playlists of effects that play automatically with configurable durations and transitions. The WebUI manages timing and sends configurations to the ESP32 at each step.

### Architecture

```
Chromance-WebUI/js/
├── core/
│   ├── eventBus.js        # Extended with sequence/playback events
│   ├── stateStore.js      # Extended with sequence state & persistence
│   └── effectSequencer.js # NEW: Core sequencer engine
├── managers/
│   └── sequenceManager.js # NEW: Sequence panel UI manager
└── components/
    └── sequenceEditorModal.js # NEW: Sequence editor modal
```

### Data Structures

**Sequence** (stored in localStorage under `sequences` key):
```javascript
{
    id: number,              // Unique sequence ID
    name: string,            // Display name
    steps: [                 // Array of steps
        {
            effectId: number,         // Reference to effect ID
            duration: number,         // Step duration in ms (500-60000)
            transitionType: string,   // 'instant', 'fadeIn', 'fadeOut', 'crossfade'
            transitionDuration: number // Transition time in ms (0-5000)
        }
    ],
    loop: boolean,           // Loop sequence when complete
    shuffleOnLoop: boolean   // Randomize step order on each loop
}
```

**Playback State** (in-memory only):
```javascript
{
    isPlaying: boolean,
    isPaused: boolean,
    currentSequenceId: number,
    currentStepIndex: number,
    stepStartTime: number,    // performance.now() timestamp
    elapsedTime: number,      // Accumulated elapsed time (for pause/resume)
    inTransition: boolean
}
```

### Events

| Event | Description |
|-------|-------------|
| `SEQUENCE_CREATED` | New sequence created |
| `SEQUENCE_UPDATED` | Sequence modified |
| `SEQUENCE_DELETED` | Sequence deleted |
| `SEQUENCE_LOADED` | Sequence selected |
| `PLAYBACK_STARTED` | Playback began |
| `PLAYBACK_PAUSED` | Playback paused |
| `PLAYBACK_RESUMED` | Playback resumed after pause |
| `PLAYBACK_STOPPED` | Playback stopped |
| `PLAYBACK_STEP_CHANGED` | Advanced to new step |
| `PLAYBACK_PROGRESS` | Progress update (fired every tick) |
| `PLAYBACK_TRANSITION_START` | Transition began |
| `PLAYBACK_TRANSITION_END` | Transition completed |
| `PLAYBACK_LOOP` | Sequence looped |

### Transition Types

| Type | Behavior |
|------|----------|
| `instant` | Immediately apply next effect config |
| `fadeIn` | Briefly set high decay, then apply next effect |
| `fadeOut` | Stop ripple firing, let current ripples fade, then switch |
| `crossfade` | Fire both effects briefly (old ripples fade while new ones start) |

### UI Components

**Sequence Panel** (in aside section):
- Sequence dropdown selector
- Add/Edit/Delete sequence buttons
- Playback controls: Previous, Play, Pause, Stop, Next
- Progress bar with step indicator

**Sequence Editor Modal**:
- Sequence name input
- Loop and Shuffle checkboxes
- Step list with drag-and-drop reordering
- Per-step settings: effect, duration, transition type, transition duration

### Usage

1. Click "+" to create a new sequence
2. In the editor, add steps by clicking "+" next to "Steps"
3. For each step, select an effect and set duration
4. Choose transition type (instant, fade in, fade out, crossfade)
5. Enable loop/shuffle options as desired
6. Click Save
7. Select the sequence and click Play

### Key Files

| File | Purpose |
|------|---------|
| `js/core/effectSequencer.js` | Core engine with timer, transitions, playback control |
| `js/managers/sequenceManager.js` | UI manager for sequence panel |
| `js/components/sequenceEditorModal.js` | Modal for editing sequences |
| `js/core/stateStore.js` | Extended with sequence CRUD and playback state |
| `js/core/eventBus.js` | Extended with sequence/playback events |
| `css/index.css` | Styles for sequence panel and editor modal |
| `EffectEditor.html` | Sequence panel HTML added to aside |
