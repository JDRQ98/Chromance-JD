# Hexachrome Improvement Roadmap

## Priority 1: Architecture & Communication

### [x] 1. WebSocket Communication (COMPLETED)
**Status**: Implemented 2026-02-07
**Summary**: Bidirectional WebSocket for instant updates and state synchronization.
**Files**: `Chromance-JD/HTTP_Server.cpp`, `Chromance-WebUI/js/main.js`

**Completed Work**:
- [x] `Chromance-JD/WebSocket_Server.h` - WebSocket API declarations (32 lines)
- [x] `Chromance-JD/WebSocket_Server.cpp` - Full WebSocket server implementation (351 lines)
  - Message types: UPDATE_CONFIG, GET_STATE, FIRE_RIPPLE, ACK, STATE, RIPPLE_FIRED
  - Event handlers for connect/disconnect/data/error
  - Configuration update parsing (rippleDelay, rippleLifeSpan, rippleSpeed, decay, behavior, direction, colors)
  - Active nodes and node-specific settings support
  - State broadcast and ripple notification
- [x] `Chromance-JD/WiFi_utilities.cpp` - Modified to init WebSocket server and add routes for new JS modules
  - Routes added for: effectSequencer.js, sequenceManager.js, sequenceEditorModal.js, topology.js, rippleSimulator.js, canvasVisualizer.js
- [x] `Chromance-WebUI/js/core/wsClient.js` - WebSocket client (310 lines)
  - Auto-reconnect with exponential backoff
  - Message queue for offline messages
  - ACK handling with timeout
  - State sync with hash comparison
  - Convenience methods: updateConfig, getState, fireRipple, requestStateSync

### [x] 5. Separation of Concerns (WebUI Refactor) (COMPLETED)
**Status**: Fully implemented
**Summary**: Complete architectural refactor with event bus, centralized state store, and modular components.

**Files Created**:
- `js/core/eventBus.js` (105 lines) - Pub/sub event system
- `js/core/stateStore.js` (672 lines) - Centralized state management with localStorage persistence
- `js/core/wsClient.js` (238 lines) - WebSocket client with auto-reconnect
- `js/managers/main.js` (83 lines) - Refactored bootstrap entry point
- `js/managers/modalManager.js` (360 lines) - Refactored modal manager using event bus
- `js/managers/nodeManager.js` (273 lines) - Refactored node manager
- `js/managers/effectsManager.js` (178 lines) - Refactored effects manager
- `js/managers/globalSettingsManager.js` (251 lines) - Refactored global settings manager
- `js/components/colorPaletteManager.js` (169 lines) - Extracted reusable color palette component
- `js/utils/settingsUtils.js` (70 lines) - Setting name/ID utilities

**Files Modified**:
- `js/main.js` - Updated imports to use new architecture
- `EffectEditor.html` - Added sequence panel (21 lines added)
- `css/index.css` - Added styles for sequencer (428 lines added)

**Key Improvements**:
- Event-driven architecture with pub/sub pattern
- Single source of truth in StateStore
- Automatic persistence to localStorage
- Decoupled modules communicate via events
- Reusable ColorPaletteManager component
- modalManager.js reduced from 600+ to 360 lines

---

## Priority 2: Code Quality

### [x] 2. DRY Ripple Firing Functions (COMPLETED)
**Status**: Implemented 2026-02-07
**Summary**: Created generic `FireRipple_FromNodes()` helper function to eliminate code duplication.
**Files Modified**: `Chromance-JD/ASW.cpp`
**Changes**:
- Added static helper `FireRipple_FromNodes()` (~40 lines) that handles iteration over nodes and directions
- Created direction arrays: `dirsOddCube[]`, `dirsPairCube[]`, `dirsAll6[]`
- Refactored all 6 bulk fire functions to use the helper
- Reduced code from ~140 lines to ~100 lines (30% reduction)

### [x] 3. State Synchronization (COMPLETED)
**Status**: Implemented 2026-02-07
**Summary**: Bidirectional state sync between WebUI and firmware via WebSocket.
**Files Modified**:
- `Chromance-WebUI/js/core/wsClient.js` - Added state sync handling, hash comparison, firmware state mapping
- `Chromance-WebUI/js/core/stateStore.js` - Added `applyFirmwareState()`, `getGlobalSettings()`, `getActiveNodeIds()`, `hasStateDifference()`
- `Chromance-WebUI/js/core/eventBus.js` - Added FIRMWARE_STATE_SYNCED, FIRMWARE_STATE_RECEIVED, FIRMWARE_STATE_CONFLICT events
- `Chromance-WebUI/js/managers/globalSettingsManager.js` - Subscribe to firmware sync events, auto-update UI
**Features**:
- Firmware sends state to WebUI on WebSocket connect (already implemented in firmware)
- WebUI parses STATE messages and maps to local store format
- State hash calculation for conflict detection
- `applyFirmwareState()` merges firmware state with local settings
- GlobalSettingsManager auto-updates UI when firmware state is synced
- `hasStateDifference()` compares local vs firmware state
- Preserves local-only settings (colors, effectBasis) during sync

---

## Priority 3: Performance

### [x] 6. Time-Invariant Animation (COMPLETED)
**Status**: Implemented 2026-02-07
**Summary**: Ripple motion now uses delta-time calculation for consistent speed regardless of loop rate.
**Files Modified**: `Chromance-JD/MCAL/ripple.h`
**Changes**:
- Added `lastAdvanceTime` member variable to Ripple class
- Modified `advance()` to calculate `deltaTime` between calls
- Pressure accumulation now scaled by `timeScale = deltaTime / 16.0f` (60fps baseline)
- At faster loop rates, less pressure added; at slower rates, more pressure added
- Removed the TODO comment about loop speed affecting motion

---

## Priority 4: Features

### [x] 4. Live Preview in WebUI (COMPLETED)
**Status**: Implemented 2026-02-07
**Summary**: Canvas-based live preview with client-side ripple simulation for offline effect preview.
**Files Added**:
- `Chromance-WebUI/js/core/topology.js` (~200 lines) - Node/segment connection data ported from firmware
- `Chromance-WebUI/js/core/rippleSimulator.js` (~450 lines) - Full ripple algorithm ported to JavaScript
- `Chromance-WebUI/js/core/canvasVisualizer.js` (~350 lines) - HTML5 Canvas renderer with segment gradients
**Files Modified**:
- `Chromance-WebUI/EffectEditor.html` - Added visualizer wrapper and preview toggle button
- `Chromance-WebUI/css/index.css` - Added ~120 lines of canvas visualizer styles
- `Chromance-WebUI/js/core/eventBus.js` - Added preview/visualizer events and on/emit aliases
- `Chromance-WebUI/js/managers/main.js` - Initialize visualizer, preview toggle, and settings sync
- `Chromance-JD/WiFi_utilities.cpp` - Added routes for new JS files
**Features**:
- Client-side ripple algorithm simulation (no WebSocket required for preview)
- HTML5 Canvas rendering with DPR support for sharp display
- Segment gradient visualization showing ripple position and color
- Node glow effects with rainbow animation for active nodes
- Preview toggle button to start/stop simulation
- Real-time sync with UI settings (speed, lifespan, colors, active nodes)
- Click-to-select nodes on canvas
- Automatic cleanup when preview stops

### [x] 7. Effect Composition & Sequencing (COMPLETED)
**Status**: Implemented 2026-02-07
**Summary**: WebUI-based effect sequencer with playlist support, transition blending, and playback controls.
**Files Added**:
- `Chromance-WebUI/js/core/effectSequencer.js` - Core sequencer engine
- `Chromance-WebUI/js/managers/sequenceManager.js` - Sequence panel UI
- `Chromance-WebUI/js/components/sequenceEditorModal.js` - Sequence editor modal
**Files Modified**:
- `Chromance-WebUI/js/core/eventBus.js` - Added sequence/playback events
- `Chromance-WebUI/js/core/stateStore.js` - Added sequence state & persistence
- `Chromance-WebUI/js/managers/main.js` - Bootstrap sequencer
- `Chromance-WebUI/EffectEditor.html` - Added sequence panel HTML
- `Chromance-WebUI/css/index.css` - Added sequencer styles
**Features**:
- Create/edit/delete sequences with multiple steps
- Per-step effect selection, duration, and transition settings
- Transition types: instant, fadeIn, fadeOut, crossfade
- Playback controls: play/pause/stop/skip
- Loop and shuffle-on-loop options
- Drag-and-drop step reordering
- Progress bar with step indicator

---

## Incremental Improvement Plan (Current Branch)

### [x] Phase 0b: Fix Color Format Mismatch (COMPLETED)
**Status**: Implemented 2026-02-09
**Summary**: WebUI sends RGB hex, firmware internally uses HSV hue. Added conversion at parse boundary.
**Files Modified**: `src/HTTP_Server.cpp`
**Changes**:
- Added `rgbHexToHue16()` - converts RGB hex (0xRRGGBB) to 16-bit HSV hue (0-65535) at POST parse time
- Added `hue16ToRgbHex()` - converts HSV hue back to RGB hex for GET API responses
- Fixed default colors in `setupDefaultProfileParameters()` to use HSV hue values instead of RGB literals

### [x] Phase 0c: Fix Active Nodes Reset on Modal Save/Discard (COMPLETED)
**Status**: Implemented 2026-02-09
**Summary**: Modal save/discard was overwriting nodeManager's active nodes with a stale copy.
**Files Modified**: `data/js/modalManager.js`
**Changes**:
- Moved `takeSnapshot()` to after `loadNodeSettings()` in `openModal()`
- Removed `setActiveNodes()` calls from `saveNodeSettings()` and `discardNodeSettings()` — active nodes are managed entirely by nodeManager via the hex grid
- Simplified `closeModal()` to only handle visual deselection

### [x] Phase 1: Fix EEPROM Key Mismatch (COMPLETED)
**Status**: Implemented 2026-02-09
**Summary**: `EEPROM_Init()` used key "GlobalConfig" but Read/Store functions used "GlobalParameters", silently losing saved data.
**Files Modified**: `src/MCAL/EEP.cpp`
**Changes**:
- Defined `#define EEPROM_KEY "GlobalConfig"` and used it in all 4 locations

### [x] Phase 2: Auto-Save on Profile Update (COMPLETED)
**Status**: Implemented 2026-02-09
**Summary**: Profile changes via REST API are now persisted to EEPROM with debounced writes.
**Files Modified**: `src/MCAL/EEP.h`, `src/MCAL/EEP.cpp`, `src/HTTP_Server.cpp`, `src/main.cpp`
**Changes**:
- Added `EEPROM_MarkDirty()` and `EEPROM_DebouncedSave()` with 5-second cooldown
- Called `EEPROM_MarkDirty()` after `handle_UpdateProfile()` and `handle_UpdateGlobalParameters()`
- Called `EEPROM_DebouncedSave()` in main loop

### [x] Phase 3: Time-Invariant Animation (COMPLETED)
**Status**: Implemented 2026-02-09
**Summary**: Ripple speed is now consistent regardless of loop rate.
**Files Modified**: `src/MCAL/ripple.h`
**Changes**:
- Added `lastAdvanceTime` member to Ripple class
- Pressure and hue delta now scaled by `deltaTime / 16.0f` (60fps baseline)

### [x] Brightness Control (COMPLETED)
**Status**: Implemented 2026-02-09
**Summary**: Landing page brightness slider for real-time LED brightness control.
**Files Modified**: `src/HTTP_Server.h`, `src/HTTP_Server.cpp`, `src/MCAL/ripple.cpp`, `src/MCAL/EEP.cpp`, `src/main.cpp`, `data/index.html`, `data/css/landing.css`, `data/js/landing.js`
**Changes**:
- Added `Brightness` field (0-255) to `GlobalParameters_struct`
- GET response includes brightness; POST handler applies it immediately to LED strips
- Brightness persisted to EEPROM via existing debounced save
- Landing page has a slider that loads current value and sends updates on change

### [ ] Phase 4: Firmware-Side Sequence Engine
**Status**: Planned
**Summary**: ESP32 autonomously cycles through active profiles without WebUI involvement.

### [ ] Phase 5: Landing Page Sequencer UI
**Status**: Planned
**Summary**: Landing page controls to configure firmware-side profile sequencer.

### [ ] Phase 6: WebUI Quality-of-Life
**Status**: Planned
**Summary**: "Stay on Editor" option, shared JSON fix utility.

---

## Progress Log

| Date | Item | Status | Notes |
|------|------|--------|-------|
| 2026-02-07 | 5. Separation of Concerns | Completed | Full WebUI refactor: eventBus, stateStore, modular managers, ColorPaletteManager |
| 2026-02-07 | 1. WebSocket Communication | Completed | Firmware WebSocket server, WebUI wsClient with state sync |
| 2026-02-07 | 7. Effect Composition & Sequencing | Completed | Full WebUI implementation with sequencer engine, UI, transitions, playback controls |
| 2026-02-07 | 2. DRY Ripple Firing Functions | Completed | Generic FireRipple_FromNodes() helper, 30% code reduction in ASW.cpp |
| 2026-02-07 | 6. Time-Invariant Animation | Completed | Delta-time based pressure calculation in ripple.h advance() method |
| 2026-02-07 | 4. Live Preview in WebUI | Completed | Canvas visualizer with client-side ripple simulation, segment gradients, preview toggle |
| 2026-02-07 | 3. State Synchronization | Completed | Bidirectional WebSocket state sync, hash comparison, auto UI update |
| 2026-02-09 | Phase 0b: Color Format Fix | Completed | RGB→HSV conversion at parse boundary in HTTP_Server.cpp |
| 2026-02-09 | Phase 0c: Modal Active Nodes Fix | Completed | Removed stale active nodes override from save/discard |
| 2026-02-09 | Phase 1: EEPROM Key Fix | Completed | Unified EEPROM key to "GlobalConfig" |
| 2026-02-09 | Phase 2: Auto-Save | Completed | Debounced EEPROM writes on profile/global parameter updates |
| 2026-02-09 | Phase 3: Time-Invariant Animation | Completed | Delta-time pressure scaling in ripple.h |
| 2026-02-09 | Brightness Control | Completed | Landing page slider + firmware support for real-time brightness |
