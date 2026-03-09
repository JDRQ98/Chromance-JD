#pragma once

/* Renders a pulsating solid color to selected node segments.
   Call from main loop when GlobalParameters.StableColorMode is true,
   in place of Ripple_MainFunction(). */
void StableColor_MainFunction();
