#include "ui.h"

// The run itself — resolved instantly, replayed as a short animation: a dot
// crossing the route while the two clocks tick at their true relative rates.
// The twin paradox as a five-second cutscene.

#define DURATION_MS 4200
#define TICK_MS 50
#define N_STARS 26

static Window *s_win;
static Layer *s_layer;
static AppTimer *s_timer;
static int s_elapsed;
static GPoint s_stars[N_STARS];
static char s_from[NAME_LEN], s_to[NAME_LEN];

static double ease(double t) { return t * t * (3 - 2 * t); }   // smoothstep

static void draw(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  graphics_context_set_fill_color(ctx, GColorDarkGray);
  for (int i = 0; i < N_STARS; i++)
    graphics_fill_circle(ctx, s_stars[i], i % 7 == 0 ? 1 : 0);

  double t = (double)s_elapsed / DURATION_MS;
  if (t > 1) t = 1;
  double p = ease(t);

  int rx0 = 16, rx1 = b.size.w - 16, ry = b.size.h / 2 + 8;
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(rx0, ry), GPoint(rx1, ry));
  graphics_context_set_fill_color(ctx, GColorChromeYellow);
  graphics_fill_circle(ctx, GPoint(rx0, ry), 3);
  graphics_context_set_fill_color(ctx, GColorCyan);
  graphics_fill_circle(ctx, GPoint(rx1, ry), 3);

  int sx = rx0 + (int)((rx1 - rx0) * p);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(sx, ry), 2);
  // a short drive plume trailing the dot
  graphics_context_set_stroke_color(ctx, GColorCyan);
  graphics_context_set_stroke_width(ctx, 1);
  int plume = 4 + (s_elapsed / TICK_MS) % 3;
  if (sx - plume > rx0) graphics_draw_line(ctx, GPoint(sx - plume, ry), GPoint(sx - 3, ry));

  char buf[64], t1[20], t2[20];
  graphics_context_set_text_color(ctx, GColorLightGray);
  graphics_draw_text(ctx, s_from, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(4, ry + 8, b.size.w / 2 - 4, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, s_to, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(b.size.w / 2, ry + 8, b.size.w / 2 - 4, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  // the two clocks — the whole point
  fmt_years(t1, sizeof t1, g_last.t_uni * p);
  snprintf(buf, sizeof buf, "UNIVERSE  %s", t1);
  graphics_context_set_text_color(ctx, GColorChromeYellow);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(0, 22, b.size.w, 28), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
  fmt_years(t2, sizeof t2, g_last.t_ship * p);
  snprintf(buf, sizeof buf, "SHIP  %s", t2);
  graphics_context_set_text_color(ctx, GColorCyan);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(0, 52, b.size.w, 28), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  fmt_beta(t1, sizeof t1, g_last.beta);
  fmt_gamma(t2, sizeof t2, g_last.gamma);
  snprintf(buf, sizeof buf, "cruise %s  gamma %s  @%dg", t1, t2, g_last.g_limit);
  graphics_context_set_text_color(ctx, GColorLightGray);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, b.size.h - 22, b.size.w, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void tick(void *data) {
  s_elapsed += TICK_MS;
  if (s_elapsed >= DURATION_MS + 500) {   // hold the final frame a beat
    s_timer = NULL;
    vibes_double_pulse();
    win_results_push();
    return;
  }
  layer_mark_dirty(s_layer);
  s_timer = app_timer_register(TICK_MS, tick, NULL);
}

static void click_noop(ClickRecognizerRef r, void *ctx) {}
static void click_config(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_BACK, click_noop);   // no bailing mid-burn
}

static void win_load(Window *w) {
  Layer *root = window_get_root_layer(w);
  GRect b = layer_get_bounds(root);
  s_layer = layer_create(b);
  layer_set_update_proc(s_layer, draw);
  layer_add_child(root, s_layer);
  for (int i = 0; i < N_STARS; i++)
    s_stars[i] = GPoint(rand() % b.size.w, rand() % b.size.h);
  s_elapsed = 0;
  s_timer = app_timer_register(TICK_MS, tick, NULL);
}

static void win_unload(Window *w) {
  if (s_timer) { app_timer_cancel(s_timer); s_timer = NULL; }
  layer_destroy(s_layer);
  s_layer = NULL;
}

void win_flight_push(int offer_idx) {
  // capture endpoints, then resolve up front — the animation is a replay, so
  // a mid-flight exit can never leave the career half-run
  Contract c = g_offers[offer_idx];
  station_short_name(g.station, s_from, sizeof s_from);
  station_short_name(c.to, s_to, sizeof s_to);
  game_resolve(&c);
  if (!s_win) {
    s_win = window_create();
    window_set_background_color(s_win, GColorBlack);
    window_set_window_handlers(s_win, (WindowHandlers){ .load = win_load, .unload = win_unload });
    window_set_click_config_provider(s_win, click_config);
  }
  window_stack_push(s_win, true);
}
