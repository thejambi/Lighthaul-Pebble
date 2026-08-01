#include "touch.h"

#if PBL_API_EXISTS(touch_service_subscribe)

#define TAP_SLOP 14        // px of wander still forgiven as a tap

static TouchTapHandler s_on_tap;
static GPoint s_down;
static bool s_tracking;
static bool s_wandered;

static void on_touch(const TouchEvent *e, void *ctx) {
  switch (e->type) {
    case TouchEvent_Touchdown:
      s_down = GPoint(e->x, e->y);
      s_tracking = true;
      s_wandered = false;
      break;

    case TouchEvent_PositionUpdate: {
      // Drift is judged as it happens rather than from the liftoff point:
      // liftoff coordinates are the one field of this API we have no way to
      // test locally, so nothing is made to depend on them.
      if (!s_tracking) break;
      int dx = e->x - s_down.x, dy = e->y - s_down.y;
      if (dx * dx + dy * dy > TAP_SLOP * TAP_SLOP) s_wandered = true;
      break;
    }

    case TouchEvent_Liftoff:
      if (s_tracking && !s_wandered && s_on_tap) s_on_tap(s_down);
      s_tracking = false;
      break;
  }
}

void touch_begin(TouchTapHandler on_tap) {
  s_on_tap = on_tap;
  s_tracking = false;
  s_wandered = false;
  touch_service_subscribe(on_touch, NULL);
}

void touch_end(void) {
  touch_service_unsubscribe();
  s_on_tap = NULL;
  s_tracking = false;
}

bool touch_available(void) { return touch_service_is_enabled(); }

#else   // no touchscreen on this platform: the whole thing folds away

void touch_begin(TouchTapHandler on_tap) { (void)on_tap; }
void touch_end(void) {}
bool touch_available(void) { return false; }

#endif
