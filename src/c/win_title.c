#include "ui.h"

// Title screen: continue / new career / daily run / how to play, with the
// all-time records and the current map seed along the bottom.

static Window *s_win;
static Layer *s_layer;
static int s_sel;
static int s_confirm;    // row index awaiting a confirming second Select, or -1

#define N_ROWS 4
static const char *ROWS[N_ROWS] = { "CONTINUE", "NEW CAREER", "DAILY RUN", "HOW TO PLAY" };

static void draw(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  char buf[64], t1[16];

  graphics_context_set_text_color(ctx, GColorChromeYellow);
  graphics_draw_text(ctx, "LIGHTHAUL", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                     GRect(0, 2, b.size.w, 30), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
  graphics_context_set_text_color(ctx, GColorLightGray);
  graphics_draw_text(ctx, "a relativistic courier game",
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, 32, b.size.w, 16), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  int y = 58;
  for (int i = 0; i < N_ROWS; i++) {
    const char *label = ROWS[i];
    if (s_confirm == i) label = "SURE? SELECT = YES";
    if (i == s_sel) {
      graphics_context_set_fill_color(ctx, s_confirm == i ? GColorRed : GColorChromeYellow);
      graphics_fill_rect(ctx, GRect(10, y, b.size.w - 20, 26), 3, GCornersAll);
      graphics_context_set_text_color(ctx, GColorBlack);
    } else {
      graphics_context_set_text_color(ctx, GColorWhite);
    }
    graphics_draw_text(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(10, y, b.size.w - 20, 24), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
    y += 30;
  }

  y = b.size.h - 46;
  if (g_rec.careers > 0) {
    fmt_gamma(t1, sizeof t1, g_rec.best_gamma);
    snprintf(buf, sizeof buf, "BEST $%ld  %d runs  gamma %s",
             (long)g_rec.best_balance, g_rec.best_deliveries, t1);
    graphics_context_set_text_color(ctx, GColorCyan);
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(0, y, b.size.w, 16), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
  }
  snprintf(buf, sizeof buf, "map seed: %s", g.seed);
  graphics_context_set_text_color(ctx, GColorDarkGray);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, y + 16, b.size.w, 16), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
}

static void start_career(const char *seed) {
  game_new(seed);
  s_confirm = -1;
  layer_mark_dirty(s_layer);
  win_map_push();
}

static void click_sel(ClickRecognizerRef r, void *ctx) {
  char ds[10];
  switch (s_sel) {
    case 0: win_map_push(); break;
    case 1:
      if (s_confirm == 1) start_career(NULL);
      else { s_confirm = 1; layer_mark_dirty(s_layer); }
      break;
    case 2:
      daily_seed(ds, sizeof ds);
      if (strcmp(ds, g.seed) == 0) { win_map_push(); break; }   // already flying today's map
      if (s_confirm == 2) start_career(ds);
      else { s_confirm = 2; layer_mark_dirty(s_layer); }
      break;
    case 3: win_about_push(); break;
  }
}

static void move(int d) {
  s_sel = (s_sel + d + N_ROWS) % N_ROWS;
  s_confirm = -1;
  layer_mark_dirty(s_layer);
}
static void click_up(ClickRecognizerRef r, void *ctx)   { move(-1); }
static void click_down(ClickRecognizerRef r, void *ctx) { move(1); }

static void click_config(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_UP, click_up);
  window_single_click_subscribe(BUTTON_ID_DOWN, click_down);
  window_single_click_subscribe(BUTTON_ID_SELECT, click_sel);
}

static void win_load(Window *w) {
  Layer *root = window_get_root_layer(w);
  s_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_layer, draw);
  layer_add_child(root, s_layer);
}

static void win_unload(Window *w) { layer_destroy(s_layer); s_layer = NULL; }
static void win_appear(Window *w) {
  s_confirm = -1;
  if (s_layer) layer_mark_dirty(s_layer);
}

void win_title_push(void) {
  if (!s_win) {
    s_win = window_create();
    window_set_background_color(s_win, GColorBlack);
    window_set_window_handlers(s_win, (WindowHandlers){
      .load = win_load, .unload = win_unload, .appear = win_appear });
    window_set_click_config_provider(s_win, click_config);
  }
  window_stack_push(s_win, true);
}
