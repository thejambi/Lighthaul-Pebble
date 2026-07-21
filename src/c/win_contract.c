#include "ui.h"

// Contract detail card: the deal on top, the auto-computed flight plan below.
// Select accepts and flies; Back passes.

static Window *s_win;
static Layer *s_layer;
static int s_idx;

static void line(GContext *ctx, GRect b, int *y, const char *txt,
                 GColor color, const char *font_key, int h) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, txt, fonts_get_system_font(font_key),
                     GRect(6, *y, b.size.w - 12, h + 4),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  *y += h;
}

static void draw(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  Contract *c = &g_offers[s_idx];
  RunPlan p = game_plan(c);
  char buf[96], t1[20], t2[20], t3[20];
  int y = -2;

  char nm[NAME_LEN];
  station_short_name(c->to, nm, sizeof nm);
  snprintf(buf, sizeof buf, "%s > %s", c->type == 0 ? "CARGO" : "PAX", nm);
  line(ctx, b, &y, buf, GColorChromeYellow, FONT_KEY_GOTHIC_18_BOLD, 20);
  line(ctx, b, &y, c->what, GColorLightGray, FONT_KEY_GOTHIC_14, 30);

  snprintf(buf, sizeof buf, "PAY $%ld   RATING %dg", (long)contract_pay(c), c->g_limit);
  line(ctx, b, &y, buf, GColorWhite, FONT_KEY_GOTHIC_14, 16);
  fmt_years(t1, sizeof t1, c->deadline);
  snprintf(buf, sizeof buf, "DIST %dly   DEADLINE %s", (int)(c->d + 0.5f), t1);
  line(ctx, b, &y, buf, GColorWhite, FONT_KEY_GOTHIC_14, 16);
  if (c->type == 1) {
    fmt_years(t1, sizeof t1, c->max_aging);
    snprintf(buf, sizeof buf, "PAX AGING CAP %s", t1);
    line(ctx, b, &y, buf, GColorWhite, FONT_KEY_GOTHIC_14, 16);
  }

  y += 4;
  line(ctx, b, &y, "-- FLIGHT PLAN (AUTO) --", GColorDarkGray, FONT_KEY_GOTHIC_14, 16);

  fmt_beta(t1, sizeof t1, p.beta);
  fmt_gamma(t2, sizeof t2, p.gamma);
  snprintf(buf, sizeof buf, "CRUISE %s   g %s", t1, t2);
  line(ctx, b, &y, buf, GColorCyan, FONT_KEY_GOTHIC_14, 16);

  fmt1(t1, sizeof t1, p.dv);
  fmt1(t2, sizeof t2, g.fuel);
  snprintf(buf, sizeof buf, "BURN dv %s of %s", t1, t2);
  line(ctx, b, &y, buf, GColorCyan, FONT_KEY_GOTHIC_14, 16);

  fmt_years(t1, sizeof t1, p.t_uni);
  snprintf(buf, sizeof buf, "UNIVERSE %s %s", t1, p.deadline_ok ? "" : "LATE!");
  line(ctx, b, &y, buf, p.deadline_ok ? GColorGreen : GColorRed, FONT_KEY_GOTHIC_14, 16);

  fmt_years(t1, sizeof t1, p.t_ship);
  fmt1(t2, sizeof t2, g.pilot_age + p.t_ship);
  fmt1(t3, sizeof t3, retire_age());
  snprintf(buf, sizeof buf, "SHIP %s > age %s/%s", t1, t2, t3);
  bool age_bad = !p.retire_ok || (c->type == 1 && !p.aging_ok);
  line(ctx, b, &y, buf, age_bad ? GColorRed : GColorGreen, FONT_KEY_GOTHIC_14, 16);
  if (c->type == 1 && !p.aging_ok)
    line(ctx, b, &y, "PAX WOULD AGE OUT!", GColorRed, FONT_KEY_GOTHIC_14, 16);
  if (!p.retire_ok)
    line(ctx, b, &y, "YOU WOULD AGE OUT!", GColorRed, FONT_KEY_GOTHIC_14, 16);

  graphics_context_set_text_color(ctx, GColorChromeYellow);
  graphics_draw_text(ctx, "SELECT: ACCEPT   BACK: PASS",
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, b.size.h - 18, b.size.w, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void click_sel(ClickRecognizerRef r, void *ctx) {
  win_flight_push(s_idx);
}

static void click_config(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_SELECT, click_sel);
}

static void win_load(Window *w) {
  Layer *root = window_get_root_layer(w);
  s_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_layer, draw);
  layer_add_child(root, s_layer);
}

static void win_unload(Window *w) { layer_destroy(s_layer); s_layer = NULL; }

void win_contract_push(int offer_idx) {
  s_idx = offer_idx;
  if (!s_win) {
    s_win = window_create();
    window_set_background_color(s_win, GColorBlack);
    window_set_window_handlers(s_win, (WindowHandlers){ .load = win_load, .unload = win_unload });
    window_set_click_config_provider(s_win, click_config);
  }
  window_stack_push(s_win, true);
}
