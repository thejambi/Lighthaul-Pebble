#include "ui.h"

// Delivery results — and, when the clock runs out, the career epilogue.

static Window *s_win;
static Layer *s_layer;
static int s_page;                 // 0 = delivery result, 1 = career over

static void line(GContext *ctx, GRect b, int *y, const char *txt,
                 GColor color, const char *font_key, int h) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, txt, fonts_get_system_font(font_key),
                     GRect(6, *y, b.size.w - 12, h + 6),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  *y += h;
}

static void draw(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  char buf[96], t1[20], t2[20];
  int y = -2;

  if (s_page == 1) {
    line(ctx, b, &y, "CAREER OVER", GColorRed, FONT_KEY_GOTHIC_28_BOLD, 30);
    fmt1(t1, sizeof t1, g.pilot_age);
    snprintf(buf, sizeof buf, "Retired at %s, flying the Courier.", t1);
    line(ctx, b, &y, buf, GColorLightGray, FONT_KEY_GOTHIC_14, 20);
    snprintf(buf, sizeof buf, "FINAL BALANCE $%ld%s", (long)g.credits,
             g_last.rec_balance ? " *BEST*" : "");
    line(ctx, b, &y, buf, GColorChromeYellow, FONT_KEY_GOTHIC_18_BOLD, 22);
    snprintf(buf, sizeof buf, "RANK: %s", rank_for(g.credits));
    line(ctx, b, &y, buf, GColorChromeYellow, FONT_KEY_GOTHIC_18_BOLD, 24);
    snprintf(buf, sizeof buf, "%d delivered, %d failed%s", g.deliveries, g.failures,
             g_last.rec_deliveries ? " *BEST*" : "");
    line(ctx, b, &y, buf, GColorWhite, FONT_KEY_GOTHIC_14, 16);
    fmt_years(t1, sizeof t1, g.uni_time);
    snprintf(buf, sizeof buf, "%s passed in the universe", t1);
    line(ctx, b, &y, buf, GColorWhite, FONT_KEY_GOTHIC_14, 16);
    fmt_gamma(t1, sizeof t1, g.max_gamma);
    snprintf(buf, sizeof buf, "highest gamma: %s%s", t1,
             g_last.rec_gamma ? " *BEST*" : "");
    line(ctx, b, &y, buf, GColorWhite, FONT_KEY_GOTHIC_14, 16);
    snprintf(buf, sizeof buf, "seed: %s", g.seed);
    line(ctx, b, &y, buf, GColorDarkGray, FONT_KEY_GOTHIC_14, 16);
    graphics_context_set_text_color(ctx, GColorChromeYellow);
    graphics_draw_text(ctx, "SELECT: NEW CAREER",
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(0, b.size.h - 18, b.size.w, 16),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    return;
  }

  line(ctx, b, &y, g_last.ok ? "DELIVERED" : "CONTRACT FAILED",
       g_last.ok ? GColorGreen : GColorRed, FONT_KEY_GOTHIC_28_BOLD, 30);
  line(ctx, b, &y, g_last.what, GColorLightGray, FONT_KEY_GOTHIC_14, 16);
  snprintf(buf, sizeof buf, "to %s", g_last.to_name);
  line(ctx, b, &y, buf, GColorLightGray, FONT_KEY_GOTHIC_14, 18);

  snprintf(buf, sizeof buf, "PAID $%ld", (long)g_last.pay);
  line(ctx, b, &y, buf, GColorChromeYellow, FONT_KEY_GOTHIC_24_BOLD, 26);
  if (g_last.late)
    line(ctx, b, &y, "LATE - pay docked 75%", GColorRed, FONT_KEY_GOTHIC_14, 16);
  if (g_last.aged_out)
    line(ctx, b, &y, "PAX aged past cap - pay docked 80%", GColorRed, FONT_KEY_GOTHIC_14, 16);

  fmt_years(t1, sizeof t1, g_last.t_uni);
  fmt_years(t2, sizeof t2, g_last.t_ship);
  snprintf(buf, sizeof buf, "universe %s   ship %s", t1, t2);
  line(ctx, b, &y, buf, GColorWhite, FONT_KEY_GOTHIC_14, 16);
  fmt1(t1, sizeof t1, g.pilot_age);
  snprintf(buf, sizeof buf, "you are now %s   $%ld", t1, (long)g.credits);
  line(ctx, b, &y, buf, GColorWhite, FONT_KEY_GOTHIC_14, 18);

  if (g_last.licensed)
    line(ctx, b, &y, "DEEP SPACE LICENSE EARNED", GColorCyan, FONT_KEY_GOTHIC_14, 16);
  if (g_last.vignette[0]) {
    graphics_context_set_text_color(ctx, GColorCyan);
    graphics_draw_text(ctx, g_last.vignette, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(6, y, b.size.w - 12, 52), GTextOverflowModeWordWrap,
                       GTextAlignmentLeft, NULL);
  }

  graphics_context_set_text_color(ctx, GColorChromeYellow);
  graphics_draw_text(ctx, "SELECT: CONTINUE",
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, b.size.h - 18, b.size.w, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void click_sel(ClickRecognizerRef r, void *ctx) {
  if (s_page == 0 && g_last.retired) {
    s_page = 1;
    layer_mark_dirty(s_layer);
    return;
  }
  if (s_page == 1) {
    // career over: back to the title with a fresh career staged
    game_new(NULL);
    window_stack_pop_all(false);
    win_title_push();
    return;
  }
  window_stack_pop_all(false);
  win_title_push();
  win_map_push();
  win_map_refresh();
}

static void click_noop(ClickRecognizerRef r, void *ctx) {}
static void click_config(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_SELECT, click_sel);
  window_single_click_subscribe(BUTTON_ID_BACK, click_noop);   // no un-docking
}

static void win_load(Window *w) {
  Layer *root = window_get_root_layer(w);
  s_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_layer, draw);
  layer_add_child(root, s_layer);
}

static void win_unload(Window *w) { layer_destroy(s_layer); s_layer = NULL; }

void win_results_push(void) {
  s_page = 0;
  if (!s_win) {
    s_win = window_create();
    window_set_background_color(s_win, GColorBlack);
    window_set_window_handlers(s_win, (WindowHandlers){ .load = win_load, .unload = win_unload });
    window_set_click_config_provider(s_win, click_config);
  }
  window_stack_push(s_win, true);
}
