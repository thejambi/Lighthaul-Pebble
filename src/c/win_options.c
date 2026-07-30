#include "ui.h"
#include "opts.h"

// Options. UP/DOWN pick a row, SELECT changes it, BACK returns — every change
// is persisted as it's made, so there's no save step to forget. The rows are a
// table: a new setting costs one line here and one field in LhOpts. The
// selected row explains itself underneath, where there's room for it.

static Window *s_win;
static Layer *s_layer;
static int s_sel;

static const char *ONOFF[2] = { "OFF", "ON" };
static const char *VIBE_HINTS[2] = {
  "no buzz, ever",
  "a pat on arrival and on a buy",
};

typedef struct {
  const char *label;
  uint8_t *field;
  uint8_t n;
  const char **vals;
  const char **hints;            // one per value
} OptRow;

static OptRow ROWS[] = {
  { "Vibration", &g_opts.vibe, 2, ONOFF, VIBE_HINTS },
};
#define N_ROWS ((int)(sizeof ROWS / sizeof ROWS[0]))

static uint8_t row_val(const OptRow *r) {
  uint8_t v = *r->field;
  return v < r->n ? v : 0;
}

static void draw(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  bool round = IS_ROUND, compact = IS_COMPACT(b);

  graphics_context_set_text_color(ctx, COL_GOLD);
  graphics_draw_text(ctx, "OPTIONS",
                     fonts_get_system_font(compact ? FONT_KEY_GOTHIC_24_BOLD
                                                   : FONT_KEY_GOTHIC_28_BOLD),
                     GRect(0, round ? (compact ? 10 : 20) : 2, b.size.w, 32),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  int row_h = compact ? 24 : 30;
  int bx = round ? 26 : 8;
  int rw = b.size.w - 2 * bx;
  int y = round ? (compact ? 44 : 78) : (compact ? 36 : 58);
  GFont lf = fonts_get_system_font(compact ? FONT_KEY_GOTHIC_14_BOLD
                                           : FONT_KEY_GOTHIC_18_BOLD);
  GFont f14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);

  for (int i = 0; i < N_ROWS; i++) {
    const OptRow *r = &ROWS[i];
    if (i == s_sel) {
      graphics_context_set_fill_color(ctx, COL_GOLD);
      graphics_fill_rect(ctx, GRect(bx, y, rw, row_h - 4), 3, GCornersAll);
      graphics_context_set_text_color(ctx, GColorBlack);
    } else {
      graphics_context_set_text_color(ctx, GColorWhite);
    }
    // label left, value right — the value is what you came to change, so it
    // gets the edge where the eye lands
    graphics_draw_text(ctx, r->label, lf, GRect(bx + 5, y - 1, rw - 10, 24),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, r->vals[row_val(r)], lf, GRect(bx + 5, y - 1, rw - 10, 24),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    y += row_h;
  }

  // what the selected row does, in whatever space is left above the footer
  int footer_y = b.size.h - (round ? 32 : 19);
  int hint_y = y + 2;
  if (footer_y - hint_y >= 15) {
    const OptRow *sr = &ROWS[s_sel];
    graphics_context_set_text_color(ctx, COL_DIM);
    graphics_draw_text(ctx, sr->hints[row_val(sr)], f14,
                       GRect(bx, hint_y, rw, footer_y - hint_y),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }

  graphics_context_set_text_color(ctx, COL_FAINT);
  graphics_draw_text(ctx, "SELECT: change", f14,
                     GRect(0, footer_y, b.size.w, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void click_sel(ClickRecognizerRef r, void *ctx) {
  OptRow *row = &ROWS[s_sel];
  *row->field = (uint8_t)((*row->field + 1) % row->n);
  opts_save();
  layer_mark_dirty(s_layer);
}

static void move(int d) {
  s_sel = (s_sel + d + N_ROWS) % N_ROWS;
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

void win_options_push(void) {
  if (!s_win) {
    s_win = window_create();
    window_set_background_color(s_win, GColorBlack);
    window_set_window_handlers(s_win, (WindowHandlers){
      .load = win_load, .unload = win_unload });
    window_set_click_config_provider(s_win, click_config);
  }
  s_sel = 0;
  window_stack_push(s_win, true);
}
