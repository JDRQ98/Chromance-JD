#include "StableColor.h"
#include "HTTP_Server.h"
#include "MCAL/mapping.h"
#include <Adafruit_NeoPixel.h>
#include <math.h>

extern Adafruit_NeoPixel strips[];

void StableColor_MainFunction() {
    /* Compute pulsed brightness via sine wave.
       phase oscillates 0→1→0 at PulseFrequency Hz.
       V oscillates between (1 - PulseDepth) * 255 and 255. */
    float t = millis() / 1000.0f;
    float phase = 0.5f + 0.5f * sinf(TWO_PI * GlobalParameters.PulseFrequency * t);
    float vf = (1.0f - GlobalParameters.PulseDepth + GlobalParameters.PulseDepth * phase) * 255.0f;
    uint8_t v = (uint8_t)constrain(vf, 0.0f, 255.0f);

    uint32_t color = strips[0].gamma32(
        strips[0].ColorHSV(GlobalParameters.StableColorHue, GlobalParameters.StableColorSat, v));

    /* Clear all LEDs then light each selected segment */
    for (int s = 0; s < NUMBER_OF_STRIPS; s++) strips[s].clear();

    for (int seg = 0; seg < NUMBER_OF_SEGMENTS; seg++) {
        if (!GlobalParameters.StableColorSegments[seg]) continue;

        int strip    = ledAssignments[seg][0];
        int ceilLed  = ledAssignments[seg][1];
        int floorLed = ledAssignments[seg][2];
        int lo = min(ceilLed, floorLed);
        int hi = max(ceilLed, floorLed);
        for (int i = lo; i <= hi; i++)
            strips[strip].setPixelColor(i, color);
    }

    for (int s = 0; s < NUMBER_OF_STRIPS; s++) strips[s].show();
}
