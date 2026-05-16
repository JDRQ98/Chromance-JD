/*
   Chromance wall hexagon source
   Partially cribbed from the DotStar example
   I smooshed in the ESP32 BasicOTA sketch, too

   (C) Voidstar Lab 2021

   Improved by JDSL jdsoftwarelabs.com 2026
   - Added Stable Color mode with optional sequencer and fade transitions
   - Added Home Assistant integration via ESPHome
   - Added Web Server & HTTP API for full parameter control and profile management
   - Added ElegantOTA for OTA updates via web browser
*/

#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include "Arduino.h"

#include "MCAL/ripple.h"
#include "HTTP_Server.h"
#include "ASW.h"
#include "MCAL/EEP.h"
#include "Wifi_utilities.h"
#include "StableColor.h"
#include "HomeAssistant_Integration.h"

#define NUMBER_OF_DIRECTIONS 6
#define NUMBER_OF_STARTING_NODES 3
#define LED 2 /*onboard LED*/

/* Global Variables used by application */
int nextRipple = 0;



void setup()
{
  Serial.begin(115200);
  Serial.printf("[T+%lums] Serial started\n", millis());

  pinMode(LED, OUTPUT);
  
  Strips_init();
  Serial.printf("[T+%lums] Strips_init done\n", millis());
  
  Serial.printf("[T+%lums] EEPROM_Init start\n", millis());
  (void) EEPROM_Init();
  Serial.printf("[T+%lums] EEPROM_Init done\n", millis());
  
  // Apply brightness from EEPROM (Strips_init runs before EEPROM_Init with default value)
  for (int i = 0; i < NUMBER_OF_STRIPS; i++) {
    strips[i].setBrightness(GlobalParameters.Brightness);
  }
  
  // Reset sequencer timestamps so stale EEPROM values don't cause immediate switches
  GlobalParameters.SequencerLastSwitch_ms = millis();
  GlobalParameters.SCSeqLastSwitch_ms     = millis();
  // Reset runtime-only fade state
  GlobalParameters.SCSeqFadePhase         = 0;
  GlobalParameters.SCSeqFadeMultiplier    = 1.0f;
  
  // Initialize period start times for all profiles
  for(int i = 0; i < NUMBER_OF_PROFILES; i++){
    GlobalParameters.RippleProfiles[i].PeriodStartTime_ms = millis();
    memset(GlobalParameters.RippleProfiles[i].EventFired, 0, sizeof(GlobalParameters.RippleProfiles[i].EventFired));
  }
  
  WiFi_Utilities_init();
  HA_init();
  Serial.printf("[T+%lums] WiFi_Utilities_init done\n", millis());

  udp_printf("GlobalParameters restored from EEPROM:");
  udp_printf("MasterFireRippleEnabled: %d", GlobalParameters.MasterFireRippleEnabled);
  udp_printf("NumberOfActiveProfiles: %d", GlobalParameters.NumberOfActiveProfiles);
  for(int i = 0; i < GlobalParameters.NumberOfActiveProfiles; i++){
    udp_printf(" -Profile %d Name: \"%s\"", i, GlobalParameters.RippleProfiles[i].ProfileName);
  }

}
  
void loop()
{
  unsigned long currentTime_ms = millis();

  WiFi_Utilities_loop();
  HA_loop();
  EEPROM_DebouncedSave(); // persist EEPROM if dirty and cooldown has elapsed

  if (!OTAinProgress)
  {
    xSemaphoreTake(gParamsMutex, portMAX_DELAY);

    /* Snapshot mode and master-enable once so rendering and event-scheduling
       gates always agree within the same loop iteration. */
    bool stableColorMode      = GlobalParameters.StableColorMode;
    bool masterFireEnabled    = GlobalParameters.MasterFireRippleEnabled;

    /* Execute any ripple kill deferred by the HTTP/Alexa handler.
       -2 = kill all ripples; >= 0 = kill specific profile. */
    if (pendingKillProfileIndex == -2) {
      Ripple_KillAllRipples();
      pendingKillProfileIndex = -1;
    } else if (pendingKillProfileIndex >= 0) {
      Ripple_KillProfileRipples((unsigned char)pendingKillProfileIndex);
      pendingKillProfileIndex = -1;
    }

    /* Sequencer: advance to next profile if dwell time has elapsed */
    if(GlobalParameters.SequencerEnabled && GlobalParameters.NumberOfActiveProfiles > 1){
      unsigned long dwellMs = (unsigned long)GlobalParameters.SequencerDwellTime_s * 1000UL;
      if(currentTime_ms - GlobalParameters.SequencerLastSwitch_ms >= dwellMs){
        GlobalParameters.SequencerLastSwitch_ms = currentTime_ms;
        if(GlobalParameters.SequencerMode == 0){
          // Sequential: advance to next profile
          GlobalParameters.SequencerCurrentProfile++;
          if(GlobalParameters.SequencerCurrentProfile >= GlobalParameters.NumberOfActiveProfiles)
            GlobalParameters.SequencerCurrentProfile = 0;
        } else {
          // Random: pick a different profile
          unsigned char next;
          do { next = random(GlobalParameters.NumberOfActiveProfiles); }
          while(next == GlobalParameters.SequencerCurrentProfile);
          GlobalParameters.SequencerCurrentProfile = next;
        }
        udp_printf("Sequencer: switched to profile %d \"%s\"",
          GlobalParameters.SequencerCurrentProfile,
          GlobalParameters.RippleProfiles[GlobalParameters.SequencerCurrentProfile].ProfileName);
      }
    }

    /* Stable Color Sequencer */
    if(stableColorMode && GlobalParameters.SCSeqEnabled && GlobalParameters.NumberOfSCPresets > 1){
      /* Compute dwell period in ms from timing mode */
      unsigned long scDwellMs;
      switch(GlobalParameters.SCSeqTimingMode){
        case 1: /* pulse-synced: 1 "beat" = 1 pulse cycle (1/PulseFrequency seconds) */
          scDwellMs = (GlobalParameters.PulseFrequency > 0.0f)
            ? (unsigned long)((1000.0f / GlobalParameters.PulseFrequency) * GlobalParameters.SCSeqBeatsPerSwitch)
            : (unsigned long)(GlobalParameters.SCSeqDwellTime_s * 1000.0f);
          break;
        case 2: /* fps-based */
          scDwellMs = (GlobalParameters.SCSeqFPS > 0) ? (1000UL / GlobalParameters.SCSeqFPS) : 83UL;
          break;
        default: /* time-based */
          scDwellMs = (unsigned long)(GlobalParameters.SCSeqDwellTime_s * 1000.0f);
          break;
      }

      /* Update fade state */
      if(GlobalParameters.SCSeqFadePhase == 1){
        /* Fading out: decrease multiplier toward 0 */
        float fadeDurMs = (float)GlobalParameters.SCSeqFadeDuration_ms;
        unsigned long elapsed = currentTime_ms - GlobalParameters.SCSeqFadePhaseStart_ms;
        GlobalParameters.SCSeqFadeMultiplier = (fadeDurMs > 0.0f)
            ? 1.0f - (float)elapsed / fadeDurMs : 0.0f;
        if(GlobalParameters.SCSeqFadeMultiplier <= 0.0f){
          GlobalParameters.SCSeqFadeMultiplier = 0.0f;
          /* Apply the pending preset and start fade-in */
          unsigned char tgt = GlobalParameters.SCSeqFadeTargetPreset;
          GlobalParameters.SCSeqCurrentPreset = tgt;
          memcpy(GlobalParameters.StableColorSegments, GlobalParameters.SCPresets[tgt].Segments, NUMBER_OF_SEGMENTS);
          if(GlobalParameters.SCSeqCycleColors)
            GlobalParameters.StableColorHue = GlobalParameters.SCPresets[tgt].Hue;
          EEPROM_MarkDirty();
          GlobalParameters.SCSeqFadePhase = 2;
          GlobalParameters.SCSeqFadePhaseStart_ms = currentTime_ms;
        }
      } else if(GlobalParameters.SCSeqFadePhase == 2){
        /* Fading in: increase multiplier toward 1 */
        float fadeDurMs = (float)GlobalParameters.SCSeqFadeDuration_ms;
        unsigned long elapsed = currentTime_ms - GlobalParameters.SCSeqFadePhaseStart_ms;
        GlobalParameters.SCSeqFadeMultiplier = (fadeDurMs > 0.0f)
            ? (float)elapsed / fadeDurMs : 1.0f;
        if(GlobalParameters.SCSeqFadeMultiplier >= 1.0f){
          GlobalParameters.SCSeqFadeMultiplier = 1.0f;
          GlobalParameters.SCSeqFadePhase = 0;
        }
      }

      /* Advance to next preset when dwell elapses (only when not fading) */
      if(GlobalParameters.SCSeqFadePhase == 0 &&
         currentTime_ms - GlobalParameters.SCSeqLastSwitch_ms >= scDwellMs){
        GlobalParameters.SCSeqLastSwitch_ms = currentTime_ms;

        unsigned char next;
        if(GlobalParameters.SCSeqMode == 0){
          next = GlobalParameters.SCSeqCurrentPreset + 1;
          if(next >= GlobalParameters.NumberOfSCPresets) next = 0;
        } else {
          do { next = random(GlobalParameters.NumberOfSCPresets); }
          while(next == GlobalParameters.SCSeqCurrentPreset && GlobalParameters.NumberOfSCPresets > 1);
        }

        if(GlobalParameters.SCSeqFadeEnabled && GlobalParameters.SCSeqFadeDuration_ms > 0){
          /* Begin fade-out toward the new preset */
          GlobalParameters.SCSeqFadeTargetPreset  = next;
          GlobalParameters.SCSeqFadePhase         = 1;
          GlobalParameters.SCSeqFadePhaseStart_ms = currentTime_ms;
          GlobalParameters.SCSeqFadeMultiplier    = 1.0f;
        } else {
          /* Immediate switch */
          GlobalParameters.SCSeqCurrentPreset = next;
          memcpy(GlobalParameters.StableColorSegments, GlobalParameters.SCPresets[next].Segments, NUMBER_OF_SEGMENTS);
          if(GlobalParameters.SCSeqCycleColors)
            GlobalParameters.StableColorHue = GlobalParameters.SCPresets[next].Hue;
          EEPROM_MarkDirty();
        }
        udp_printf("SC Sequencer: -> preset %d \"%s\"", next, GlobalParameters.SCPresets[next].PresetName);
      }
    }

    /* Period-based event scheduling — only in ripple mode */
    if(!stableColorMode && GlobalParameters.MasterFireRippleEnabled){
      for(int i = 0; i < NUMBER_OF_PROFILES; i++){
        if(!GlobalParameters.RippleProfiles[i].Active) continue;
        if(GlobalParameters.SequencerEnabled && i != GlobalParameters.SequencerCurrentProfile) continue;

        RippleProfile_struct* profile = &GlobalParameters.RippleProfiles[i];
        unsigned long elapsed = currentTime_ms - profile->PeriodStartTime_ms;

        /* Period boundary: kill this profile's ripples, restart period */
        if(elapsed >= profile->ProfilePeriod_ms){
          Ripple_KillProfileRipples(i);
          profile->PeriodStartTime_ms = currentTime_ms;
          memset(profile->EventFired, 0, sizeof(profile->EventFired));
          elapsed = 0;
        }

        /* Check each event */
        for(int e = 0; e < MAX_EVENTS_PER_PROFILE; e++){
          TimeEvent_struct* evt = &profile->Events[e];
          if(!evt->Enabled) continue;
          if(profile->EventFired[e]) continue;
          if(elapsed < evt->TimeOffset_ms) continue;

          /* Fire this event */
          profile->EventFired[e] = true;
          unsigned int color = profile->Colors[profile->CurrentColor];

          for(int j = 0; j < NUMBER_OF_NODES; j++){
            if(!evt->ActiveNodes[j]) continue;
            int dirStart = 0;
            int dirEnd = NUMBER_OF_DIRECTIONS;
            if(evt->Direction >= 0 && evt->Direction < NUMBER_OF_DIRECTIONS){
              dirStart = evt->Direction;
              dirEnd = dirStart + 1;
            }
            for(int d = dirStart; d < dirEnd; d++){
              switch(evt->RippleType){
                case RIPPLETYPE_DOUBLE:
                  FireDoubleRipple(&nextRipple, d, color, j, evt->Behavior,
                    evt->RippleLifeSpan, evt->RippleSpeed, evt->RainbowDeltaPerTick,
                    NO_NODE_LIMIT, (unsigned char)i);
                  break;
                case RIPPLETYPE_SHARD:
                  FireShard(&nextRipple, d, color, j, evt->Behavior,
                    evt->RippleLifeSpan, evt->RippleSpeed, evt->RainbowDeltaPerTick,
                    NO_NODE_LIMIT, (unsigned char)i);
                  break;
                default: /* RIPPLETYPE_SINGLE */
                  FireRipple(&nextRipple, d, color, j, evt->Behavior,
                    evt->RippleLifeSpan, evt->RippleSpeed, evt->RainbowDeltaPerTick,
                    noPreference, NO_NODE_LIMIT, (unsigned char)i);
                  break;
              }
            } /* end direction loop */
          } /* end node loop */

          /* Advance color after event fires */
          profile->CurrentColor++;
          if(profile->CurrentColor >= profile->NumberOfColors)
            profile->CurrentColor = 0;
        } /* end event loop */
      } /* end profile loop */
    } /* end MasterFireRippleEnabled check */

    xSemaphoreGive(gParamsMutex);

    /* Render after releasing the mutex so HTTP handlers aren't blocked
       during the strip clear/set/show cycle. */
    if (stableColorMode) {
      if (masterFireEnabled) {
        StableColor_MainFunction();
      } else {
        for (int s = 0; s < NUMBER_OF_STRIPS; s++) { strips[s].clear(); strips[s].show(); }
      }
    } else {
      Ripple_MainFunction();
    }

  } /* end OTAinProgress check */
}
