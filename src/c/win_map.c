#include "ui.h"

// Docked star map: top-down cluster chart. Up/Down cycles the three contract
// offers (gold route) plus a FUEL & OUTFITTING slot; Select opens it.

#define TOP_H 38
#define BOT_H 62

static Window *s_win;
static Layer *s_layer;
static int s_sel;                     // 0..2 offers, 3 = outfitting

static void project_bounds(float *minx, float *maxx, float *minz, float *maxz) {
  *minx = *minz = 1e9f; *maxx = *maxz = -1e9f;
  for (int i = 0; i < g_n_stations; i++) {
    Station *s = &g_stations[i];
    if (s->deep && !g.deep_license) continue;
    if (s->x < *minx) *minx = s->x;
    if (s->x > *maxx) *maxx = s->x;
    if (s->z < *minz) *minz = s->z;
    if (s->z > *maxz) *maxz = s->z;
  }
}

static GPoint project(float x, float z, GRect area,
                      float minx, float maxx, float minz, float maxz) {
  float dx = maxx - minx, dz = maxz - minz;
  if (dx < 1) dx = 1;
  if (dz < 1) dz = 1;
  float sx = (area.size.w - 16) / dx, sz = (area.size.h - 16) / dz;
  float s = sx < sz ? sx : sz;        // uniform scale, centered
  float cx = (minx + maxx) / 2, cz = (minz + maxz) / 2;
  return GPoint(area.origin.x + area.size.w / 2 + (int)((x - cx) * s),
                area.origin.y + area.size.h / 2 + (int)((z - cz) * s));
}

static void draw(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  GRect map_area = GRect(0, TOP_H, b.size.w, b.size.h - TOP_H - BOT_H);
  float minx, maxx, minz, maxz;
  project_bounds(&minx, &maxx, &minz, &maxz);

  char buf[96], t1[16], t2[16], t3[16];

  // --- top bar: station name + pilot vitals
  graphics_context_set_text_color(ctx, GColorChromeYellow);
  graphics_draw_text(ctx, g_stations[g.station].name,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(4, -3, b.size.w - 8, 20), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
  fmt1(t1, sizeof t1, g.fuel);
  fmt1(t2, sizeof t2, tank_cap());
  fmt1(t3, sizeof t3, g.pilot_age);
  snprintf(buf, sizeof buf, "$%ld   dv %s/%s   age %s", (long)g.credits, t1, t2, t3);
  graphics_context_set_text_color(ctx, GColorLightGray);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, 17, b.size.w, 16), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // --- selected route (under the dots)
  GPoint here = project(g_stations[g.station].x, g_stations[g.station].z,
                        map_area, minx, maxx, minz, maxz);
  if (s_sel < 3) {
    Station *t = &g_stations[g_offers[s_sel].to];
    GPoint dst = project(t->x, t->z, map_area, minx, maxx, minz, maxz);
    graphics_context_set_stroke_color(ctx, GColorChromeYellow);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx, here, dst);
  }

  // --- stations
  for (int i = 0; i < g_n_stations; i++) {
    Station *s = &g_stations[i];
    if (s->deep && !g.deep_license) continue;
    GPoint p = project(s->x, s->z, map_area, minx, maxx, minz, maxz);
    bool is_target = s_sel < 3 && g_offers[s_sel].to == i;
    bool is_offer = false;
    for (int o = 0; o < 3; o++) if (g_offers[o].to == i) is_offer = true;
    if (i == (int)g.station) {
      graphics_context_set_fill_color(ctx, GColorChromeYellow);
      graphics_fill_circle(ctx, p, 3);
      graphics_context_set_stroke_color(ctx, GColorChromeYellow);
      graphics_context_set_stroke_width(ctx, 1);
      graphics_draw_circle(ctx, p, 6);
    } else if (is_target) {
      graphics_context_set_fill_color(ctx, GColorCyan);
      graphics_fill_circle(ctx, p, 4);
    } else if (is_offer) {
      graphics_context_set_fill_color(ctx, GColorPictonBlue);
      graphics_fill_circle(ctx, p, 3);
    } else {
      graphics_context_set_fill_color(ctx, s->deep ? GColorDarkGray : GColorWhite);
      graphics_fill_circle(ctx, p, 2);
    }
    // label the selected target so the route reads at a glance
    if (is_target) {
      char nm[NAME_LEN];
      station_short_name(i, nm, sizeof nm);
      int lx = p.x < b.size.w / 2 ? p.x + 6 : p.x - 66;
      int ly = p.y < map_area.origin.y + 14 ? p.y + 4 : p.y - 16;
      graphics_context_set_text_color(ctx, GColorCyan);
      graphics_draw_text(ctx, nm, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                         GRect(lx, ly, 62, 16), GTextOverflowModeTrailingEllipsis,
                         p.x < b.size.w / 2 ? GTextAlignmentLeft : GTextAlignmentRight, NULL);
    }
  }

  // --- bottom panel
  int y = b.size.h - BOT_H;
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(0, y), GPoint(b.size.w, y));

  if (s_sel < 3) {
    Contract *c = &g_offers[s_sel];
    char nm[NAME_LEN];
    station_short_name(c->to, nm, sizeof nm);
    snprintf(buf, sizeof buf, "%d/3 %s > %s", s_sel + 1,
             c->type == 0 ? "CARGO" : "PAX", nm);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(4, y - 2, b.size.w - 8, 20), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, c->what, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(4, y + 17, b.size.w - 8, 16), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
    RunPlan p = game_plan(c);
    bool ok = p.deadline_ok && p.aging_ok && p.retire_ok;
    fmt_years(t1, sizeof t1, c->deadline);
    snprintf(buf, sizeof buf, "$%ld  %dg  %dly  DL %s %s",
             (long)contract_pay(c), c->g_limit, (int)(c->d + 0.5f), t1,
             ok ? "OK" : "!!");
    graphics_context_set_text_color(ctx, ok ? GColorGreen : GColorRed);
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(4, y + 33, b.size.w - 8, 16), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
  } else {
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, "FUEL & OUTFITTING",
                       fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(4, y - 2, b.size.w - 8, 20), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
    const char *ev = g.dock_event >= 0 ? DOCK_EVENTS[g.dock_event].txt
                                       : "Steady trade at this dock.";
    graphics_context_set_text_color(ctx, g.dock_event >= 0 ? GColorChromeYellow : GColorLightGray);
    graphics_draw_text(ctx, ev, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(4, y + 17, b.size.w - 8, 32), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
  }
}

static void click_up(ClickRecognizerRef r, void *ctx)   { s_sel = (s_sel + 3) % 4; layer_mark_dirty(s_layer); }
static void click_down(ClickRecognizerRef r, void *ctx) { s_sel = (s_sel + 1) % 4; layer_mark_dirty(s_layer); }
static void click_sel(ClickRecognizerRef r, void *ctx) {
  if (s_sel < 3) win_contract_push(s_sel);
  else win_outfit_push();
}

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
static void win_appear(Window *w) { if (s_layer) layer_mark_dirty(s_layer); }

void win_map_refresh(void) {
  s_sel = 0;
  if (s_layer) layer_mark_dirty(s_layer);
}

void win_map_push(void) {
  if (!s_win) {
    s_win = window_create();
    window_set_background_color(s_win, GColorBlack);
    window_set_window_handlers(s_win, (WindowHandlers){
      .load = win_load, .unload = win_unload, .appear = win_appear });
    window_set_click_config_provider(s_win, click_config);
  }
  window_stack_push(s_win, true);
}
