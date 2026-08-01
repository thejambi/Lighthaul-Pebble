#pragma once
#include <pebble.h>

// Touch, on the hardware that has it. The SDK's platform config declares
// PBL_TOUCH for emery and gabbro only — flint exposes the touch functions in
// its headers but has no touchscreen, so the header's presence is not the
// test to use. Everything here compiles to nothing where PBL_TOUCH is absent,
// and every button keeps doing exactly what it did: touch is only ever a
// second way in, never the only one.
//
// (Watchfaces need not apply — the platform doesn't deliver touch to them.)
//
// The service hands over raw touchdown / position / liftoff, not gestures, so
// the two this game needs are resolved here, both judged from where the finger
// landed and where it left:
//
//   tap    down and up within a thumb's width, reported at the landing point,
//          which is where the eye was aimed;
//   swipe  a mostly-vertical drag, reported as -1 (up) or +1 (down) and wired
//          to whatever Up and Down already do.

typedef void (*TouchTapHandler)(GPoint p);
typedef void (*TouchSwipeHandler)(int dir);   // -1 = up, +1 = down

void touch_begin(TouchTapHandler on_tap);     // taps only
void touch_begin_full(TouchTapHandler on_tap, TouchSwipeHandler on_swipe);
void touch_end(void);                         // from a window's .disappear
bool touch_available(void);                   // hardware present and enabled
