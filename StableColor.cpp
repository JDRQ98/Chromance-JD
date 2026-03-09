#include "StableColor.h"
#include "HTTP_Server.h"
#include "MCAL/mapping.h"
#include <Adafruit_NeoPixel.h>
#include <math.h>

extern Adafruit_NeoPixel strips[];

static void renderSegment(int seg, uint32_t color) {
    int strip = ledAssignments[seg][0];
    int lo = min(ledAssignments[seg][1], ledAssignments[seg][2]);
    int hi = max(ledAssignments[seg][1], ledAssignments[seg][2]);
    for (int i = lo; i <= hi; i++)
        strips[strip].setPixelColor(i, color);
}

void StableColor_MainFunction() {
    float t = millis() / 1000.0f;
    float phase = 0.5f + 0.5f * sinf(TWO_PI * GlobalParameters.PulseFrequency * t);
    float vBase = (1.0f - GlobalParameters.PulseDepth + GlobalParameters.PulseDepth * phase);

    for (int s = 0; s < NUMBER_OF_STRIPS; s++) strips[s].clear();

    for (int seg = 0; seg < NUMBER_OF_SEGMENTS; seg++) {
        if (!GlobalParameters.StableColorSegments[seg]) continue;

        float mul = 1.0f;
        if (GlobalParameters.SCSeqFadePhase > 0) {
            /* Determine zone: outer ring or inner */
            bool isOuter = false;
            for (int p = 0; p < numberOfPerimeterSegments; p++) {
                if (perimeterSegments[p] == seg) { isOuter = true; break; }
            }
            bool fadingThisSeg = (isOuter  && GlobalParameters.SCSeqFadeOuter) ||
                                 (!isOuter && GlobalParameters.SCSeqFadeInner);
            if (fadingThisSeg) mul = GlobalParameters.SCSeqFadeMultiplier;
        }

        uint8_t v = (uint8_t)constrain(vBase * mul * 255.0f, 0.0f, 255.0f);
        uint32_t color = strips[0].gamma32(
            strips[0].ColorHSV(GlobalParameters.StableColorHue, GlobalParameters.StableColorSat, v));
        renderSegment(seg, color);
    }

    for (int s = 0; s < NUMBER_OF_STRIPS; s++) strips[s].show();
}
