#include "ui.h"

// Contract detail card: the deal on top, the auto-computed flight plan below.
// Select accepts and flies; Back passes.

static Window *s_win;
static Layer *s_layer;
static int s_idx;
static int s_x0, s_w;      // text column, set per-screen in draw()

static void line(GContext *ctx, GRect b, int *y, const char *txt,
                 GColor color, const char *font_key, int h) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, txt, fonts_get_system_font(font_key),
                     GRect(s_x0, *y, s_w, h + 4),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  *y += h;
}

static void draw(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  bool round = IS_ROUND, compact = IS_COMPACT(b);
  s_x0 = round ? (compact ? 28 : 34) : 6;
  s_w = b.size.w - 2 * s_x0;
  int lh = compact ? 14 : 16;
  Contract *c = &g_offers[s_idx];
  RunPlan p = game_plan(c);
  char buf[96], t1[20], t2[20], t3[20];
  int y = round ? (compact ? 12 : 26) : -2;

  char nm[NAME_LEN];
  station_short_name(c->to, nm, sizeof nm);
  snprintf(buf, sizeof buf, "%s > %s", c->type == 0 ? "CARGO" : "PAX", nm);
  line(ctx, b, &y, buf, COL_GOLD, FONT_KEY_GOTHIC_18_BOLD, compact ? 18 : 20);
  line(ctx, b, &y, c->what, COL_DIM, FONT_KEY_GOTHIC_14, compact ? 16 : 30);

  if (compact) {
    // small screens: the deal in two dense lines, no section header
    snprintf(buf, sizeof buf, "$%ld   %dg   %dly", (long)contract_pay(c), c->g_limit, (int)(c->d + 0.5f));
    line(ctx, b, &y, buf, GColorWhite, FONT_KEY_GOTHIC_14, lh);
    fmt_years(t1, sizeof t1, c->deadline);
    if (c->type == 1) {
      fmt_years(t2, sizeof t2, c->max_aging);
      snprintf(buf, sizeof buf, "DL %s   CAP %s", t1, t2);
    } else {
      snprintf(buf, sizeof buf, "DL %s", t1);
    }
    line(ctx, b, &y, buf, GColorWhite, FONT_KEY_GOTHIC_14, lh);
    y += 2;
  } else {
    snprintf(buf, sizeof buf, "PAY $%ld   RATING %dg", (long)contract_pay(c), c->g_limit);
    line(ctx, b, &y, buf, GColorWhite, FONT_KEY_GOTHIC_14, lh);
    fmt_years(t1, sizeof t1, c->deadline);
    snprintf(buf, sizeof buf, "DIST %dly   DEADLINE %s", (int)(c->d + 0.5f), t1);
    line(ctx, b, &y, buf, GColorWhite, FONT_KEY_GOTHIC_14, lh);
    if (c->type == 1) {
      fmt_years(t1, sizeof t1, c->max_aging);
      snprintf(buf, sizeof buf, "PAX AGING CAP %s", t1);
      line(ctx, b, &y, buf, GColorWhite, FONT_KEY_GOTHIC_14, lh);
    }
    y += 4;
    snprintf(buf, sizeof buf, "-- PLAN %s (UP/DN) --", plan_rung_label(g.plan_rung));
    line(ctx, b, &y, buf, COL_FAINT, FONT_KEY_GOTHIC_14, lh);
  }

  fmt_beta(t1, sizeof t1, p.beta);
  fmt_gamma(t2, sizeof t2, p.gamma);
  if (compact) {
    if (g.plan_rung == 0) snprintf(buf, sizeof buf, "AUTO %s  GAMMA %s", t1, t2);
    else snprintf(buf, sizeof buf, "PLAN %s  GAMMA %s", plan_rung_label(g.plan_rung), t2);
  } else {
    snprintf(buf, sizeof buf, "CRUISE %s  GAMMA %s", t1, t2);
  }
  line(ctx, b, &y, buf, COL_CYAN, FONT_KEY_GOTHIC_14, lh);

  fmt1(t1, sizeof t1, p.dv);
  fmt1(t2, sizeof t2, g.fuel);
  fmt1(t3, sizeof t3, p.ramp_ship);
  // drive thrust for the ramp (after Redline Coils). Note this reads HIGHER
  // than the contract's g-rating when you have dampers — the dampers absorb
  // the difference, so the load still only feels its rated load.
  int thrust_g = (int)(p.accel / G_ACCEL + 0.5);
  if (compact) snprintf(buf, sizeof buf, "dv %s  RAMP %syr", t1, t3);
  else snprintf(buf, sizeof buf, "dv %s RAMP %syr THRUST %dg", t1, t3, thrust_g);
  line(ctx, b, &y, buf, COL_CYAN, FONT_KEY_GOTHIC_14, lh);

  fmt_years(t1, sizeof t1, p.t_uni);
  snprintf(buf, sizeof buf, "UNIVERSE %s %s", t1, p.deadline_ok ? "" : "LATE!");
  line(ctx, b, &y, buf, p.deadline_ok ? COL_GOOD : COL_BAD, FONT_KEY_GOTHIC_14, lh);

  fmt_years(t1, sizeof t1, p.t_ship);
  fmt1(t2, sizeof t2, g.pilot_age + p.t_ship);
  fmt1(t3, sizeof t3, retire_age());
  if (compact) snprintf(buf, sizeof buf, "SHIP %s > age %s", t1, t2);
  else snprintf(buf, sizeof buf, "SHIP %s > age %s/%s", t1, t2, t3);
  bool age_bad = !p.retire_ok || (c->type == 1 && !p.aging_ok);
  line(ctx, b, &y, buf, age_bad ? COL_BAD : COL_GOOD, FONT_KEY_GOTHIC_14, lh);
  if (!p.retire_ok)
    line(ctx, b, &y, "YOU WOULD AGE OUT!", COL_BAD, FONT_KEY_GOTHIC_14, lh);
  else if (c->type == 1 && !p.aging_ok)
    line(ctx, b, &y, "PAX WOULD AGE OUT!", COL_BAD, FONT_KEY_GOTHIC_14, lh);

  graphics_context_set_text_color(ctx, COL_GOLD);
  graphics_draw_text(ctx, (round || compact) ? "UP/DN SPEED  SEL: GO"
                                             : "UP/DN: SPEED   SELECT: ACCEPT",
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, b.size.h - (round ? 34 : 18), b.size.w, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void click_sel(ClickRecognizerRef r, void *ctx) {
  win_flight_push(s_idx);
}

static void click_rung(int d) {
  g.plan_rung = (uint8_t)((g.plan_rung + N_PLAN_RUNGS + d) % N_PLAN_RUNGS);
  layer_mark_dirty(s_layer);
}
static void click_up(ClickRecognizerRef r, void *ctx)   { click_rung(-1); }
static void click_down(ClickRecognizerRef r, void *ctx) { click_rung(1); }

static void click_config(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_SELECT, click_sel);
  window_single_click_subscribe(BUTTON_ID_UP, click_up);
  window_single_click_subscribe(BUTTON_ID_DOWN, click_down);
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
