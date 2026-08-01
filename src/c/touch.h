#pragma once
#include <pebble.h>

// Touch, on the hardware that has it (emery, flint, gabbro). Everything here
// compiles to nothing elsewhere, and every button keeps doing exactly what it
// did — touch is only ever a second way in, never the only one.
//
// The service hands over raw touchdown / position / liftoff, not gestures, so
// a tap is resolved here: press and release without wandering more than a
// thumb's width. The tap is reported at the point where the finger landed,
// which is where the eye was aimed.

typedef void (*TouchTapHandler)(GPoint p);

void touch_begin(TouchTapHandler on_tap);   // from a window's .appear
void touch_end(void);                       // from its .disappear
bool touch_available(void);                 // false when there's no touchscreen
