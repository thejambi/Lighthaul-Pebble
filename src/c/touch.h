#pragma once
#include <pebble.h>

// Touch, on the hardware that has it (emery, flint, gabbro). Everything here
// compiles to nothing elsewhere, and every button keeps doing exactly what it
// did — touch is only ever a second way in, never the only one.
//
// The service hands over raw touchdown / position / liftoff, not gestures, so
// the two this game needs are resolved here:
//
//   tap    press and release without wandering more than a thumb's width,
//          reported at the point the finger landed, which is where the eye
//          was aimed;
//   swipe  a mostly-vertical drag, reported as -1 (up) or +1 (down) and
//          wired to whatever Up and Down already do.
//
// Liftoff coordinates are the one field of this API that can't be tested
// locally, so nothing depends on them: both gestures are judged from the
// touchdown point and the last position update.

typedef void (*TouchTapHandler)(GPoint p);
typedef void (*TouchSwipeHandler)(int dir);   // -1 = up, +1 = down

void touch_begin(TouchTapHandler on_tap);     // taps only
void touch_begin_full(TouchTapHandler on_tap, TouchSwipeHandler on_swipe);
void touch_end(void);                         // from a window's .disappear
bool touch_available(void);                   // false without a touchscreen
