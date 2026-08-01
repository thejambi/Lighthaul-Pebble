#include "touch.h"

#if PBL_API_EXISTS(touch_service_subscribe)

#define TAP_SLOP   14      // px between landing and liftoff still read as a tap
#define SWIPE_MIN  26      // px of travel before a drag counts as a swipe

static TouchTapHandler s_on_tap;
static TouchSwipeHandler s_on_swipe;
static GPoint s_down;
static bool s_tracking;
static bool s_subscribed;

static void on_touch(const TouchEvent *e, void *ctx) {
  switch (e->type) {
    case TouchEvent_Touchdown:
      s_down = GPoint(e->x, e->y);
      s_tracking = true;
      break;

    case TouchEvent_PositionUpdate:
      break;               // only the two ends of the gesture decide it

    case TouchEvent_Liftoff: {
      if (!s_tracking) break;
      s_tracking = false;
      // Liftoff carries the final position, so the gesture is measured from
      // landing to lift. Reading it from the last position update instead
      // would miss a flick quick enough to produce none of them, and call
      // the swipe a tap.
      int dx = e->x - s_down.x, dy = e->y - s_down.y;
      int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
      if (s_on_swipe && ady >= SWIPE_MIN && ady > adx) {
        // Content follows the finger: drag upward and the list travels up,
        // walking the selection onward — the way Down already goes.
        s_on_swipe(dy < 0 ? 1 : -1);
      } else if (s_on_tap && adx * adx + ady * ady <= TAP_SLOP * TAP_SLOP) {
        s_on_tap(s_down);
      }
      break;
    }
  }
}

static void begin(TouchTapHandler tap, TouchSwipeHandler swipe) {
  s_on_tap = tap;
  s_on_swipe = swipe;
  s_tracking = false;
  // The runtime check decides, not the platform define. PBL_TOUCH is
  // declared for emery and gabbro only, but flint carries the same
  // functions, and whether that hardware has a screen to touch is not
  // something this file should assert — touch_service_is_enabled() knows,
  // and answers false for absent hardware and for a wearer who switched it
  // off. Compiling it in everywhere the API exists costs a few dead bytes
  // on a watch without a panel; guessing wrong costs the feature entirely.
  if (s_subscribed || !touch_service_is_enabled()) return;
  touch_service_subscribe(on_touch, NULL);
  s_subscribed = true;
}

void touch_begin(TouchTapHandler on_tap) { begin(on_tap, NULL); }
void touch_begin_full(TouchTapHandler on_tap, TouchSwipeHandler on_swipe) {
  begin(on_tap, on_swipe);
}

void touch_end(void) {
  if (s_subscribed) {
    touch_service_unsubscribe();   // let the sensor rest while we're away
    s_subscribed = false;
  }
  s_on_tap = NULL;
  s_on_swipe = NULL;
  s_tracking = false;
}

bool touch_available(void) { return touch_service_is_enabled(); }

#else   // no touchscreen on this platform: the whole thing folds away

void touch_begin(TouchTapHandler on_tap) { (void)on_tap; }
void touch_begin_full(TouchTapHandler t, TouchSwipeHandler s) { (void)t; (void)s; }
void touch_end(void) {}
bool touch_available(void) { return false; }

#endif
