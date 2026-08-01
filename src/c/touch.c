#include "touch.h"

#if PBL_API_EXISTS(touch_service_subscribe)

#define TAP_SLOP   14      // px of wander still forgiven as a tap
#define SWIPE_MIN  26      // px of travel before a drag counts as a swipe

static TouchTapHandler s_on_tap;
static TouchSwipeHandler s_on_swipe;
static GPoint s_down, s_last;
static bool s_tracking;

static void on_touch(const TouchEvent *e, void *ctx) {
  switch (e->type) {
    case TouchEvent_Touchdown:
      s_down = s_last = GPoint(e->x, e->y);
      s_tracking = true;
      break;

    case TouchEvent_PositionUpdate:
      // The gesture is judged from these, never from the liftoff point:
      // liftoff coordinates are the one field of this API with no way to
      // test them locally, so nothing is made to depend on them.
      if (s_tracking) s_last = GPoint(e->x, e->y);
      break;

    case TouchEvent_Liftoff: {
      if (!s_tracking) break;
      s_tracking = false;
      int dx = s_last.x - s_down.x, dy = s_last.y - s_down.y;
      int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
      if (s_on_swipe && ady >= SWIPE_MIN && ady > adx) {
        // Content follows the finger: drag upward and the list travels up,
        // which walks the selection onward — the way Down already goes.
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
  touch_service_subscribe(on_touch, NULL);
}

void touch_begin(TouchTapHandler on_tap) { begin(on_tap, NULL); }
void touch_begin_full(TouchTapHandler on_tap, TouchSwipeHandler on_swipe) {
  begin(on_tap, on_swipe);
}

void touch_end(void) {
  touch_service_unsubscribe();
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
