#include "ui.h"

// How to play — a scrolling primer on the three clocks and the economy.

static Window *s_win;
static ScrollLayer *s_scroll;
static TextLayer *s_text;

static const char *ABOUT_TEXT =
  "THE TWO CLOCKS\n"
  "You fly a hair under lightspeed. Deadlines run on UNIVERSE time, but you "
  "and your passengers age on SHIP time - the closer to c, the less of your "
  "life a leg costs. Decades pass at the docks while you barely age. Your "
  "career ends at 82: every ship-year is a year off it.\n\n"
  "FUEL IS SPEED\n"
  "The tank holds delta-v, and you pay to speed up AND to brake. The "
  "autopilot flies each run as fast as your fuel allows - so how much fuel "
  "you buy IS the throttle. Bigger fill-ups get a bulk discount.\n\n"
  "THE G-RATING\n"
  "Every load caps how hard the autopilot may burn and brake. Fragile "
  "freight means gentle ramps, and gentle ramps cost years on both clocks. "
  "Inertial Dampers let you push harder; rugged cargo flies itself.\n\n"
  "THE LONG GAME\n"
  "Reputation raises pay with every delivery. Each dock stocks two upgrades "
  "- the map is worth learning. Run the tank low and a recovery tow costs "
  "credits and years. Touch gamma 25,000 on a clean run and deep space "
  "opens: thousand-lightyear hauls for pilots who barely age at all.\n\n"
  "Retire rich, not old.";

static void win_load(Window *w) {
  Layer *root = window_get_root_layer(w);
  GRect b = layer_get_bounds(root);
  s_scroll = scroll_layer_create(b);
  scroll_layer_set_click_config_onto_window(s_scroll, w);
  int inset = PBL_IF_ROUND_ELSE(26, 6);
  s_text = text_layer_create(GRect(inset, 0, b.size.w - 2 * inset, 2000));
  text_layer_set_background_color(s_text, GColorBlack);
  text_layer_set_text_color(s_text, GColorWhite);
  text_layer_set_font(s_text, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_text, ABOUT_TEXT);
  GSize sz = text_layer_get_content_size(s_text);
  text_layer_set_size(s_text, GSize(b.size.w - 2 * inset, sz.h + 8));
  scroll_layer_set_content_size(s_scroll, GSize(b.size.w, sz.h + 16));
  scroll_layer_add_child(s_scroll, text_layer_get_layer(s_text));
  layer_add_child(root, scroll_layer_get_layer(s_scroll));
}

static void win_unload(Window *w) {
  text_layer_destroy(s_text);
  scroll_layer_destroy(s_scroll);
  s_text = NULL;
  s_scroll = NULL;
}

void win_about_push(void) {
  if (!s_win) {
    s_win = window_create();
    window_set_background_color(s_win, GColorBlack);
    window_set_window_handlers(s_win, (WindowHandlers){ .load = win_load, .unload = win_unload });
  }
  window_stack_push(s_win, true);
}
