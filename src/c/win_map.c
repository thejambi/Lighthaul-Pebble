#include "ui.h"

// Docked star map: top-down cluster chart. Up/Down cycles the three contract
// offers (gold route) plus a FUEL & OUTFITTING slot; Select opens it.

// bar heights are computed per-screen in draw(): rect 200x228 gets the roomy
// layout, 144x168 a compact one, round screens extra top/bottom clearance

static Window *s_win;
static Layer *s_layer;
static int s_sel;                     // 0..2 offers, 3 = outfitting

// Projection. The CORE cluster alone sets the scale: deep-space stations sit
// 800-2500 ly out against a ~340 ly cluster, so fitting them all would crush
// the cluster to a dot. Deep stations instead pin to the frame border along
// their true bearing — signposts pointing off the map, as the web chart does.
typedef struct {
  GRect area;
  float cx, cz, scale;
  int mx, my;                         // frame centre, screen coords
} Proj;

static Proj make_proj(GRect area) {
  Proj p;
  float minx = 1e9f, maxx = -1e9f, minz = 1e9f, maxz = -1e9f;
  for (int i = 0; i < g_n_stations; i++) {
    Station *s = &g_stations[i];
    if (s->deep) continue;            // core only, licensed or not
    if (s->x < minx) minx = s->x;
    if (s->x > maxx) maxx = s->x;
    if (s->z < minz) minz = s->z;
    if (s->z > maxz) maxz = s->z;
  }
  float dx = maxx - minx, dz = maxz - minz;
  if (dx < 1) dx = 1;
  if (dz < 1) dz = 1;
  float sx = (area.size.w - 16) / dx, sz = (area.size.h - 16) / dz;
  p.area = area;
  p.scale = sx < sz ? sx : sz;        // uniform scale, centred
  p.cx = (minx + maxx) / 2;
  p.cz = (minz + maxz) / 2;
  p.mx = area.origin.x + area.size.w / 2;
  p.my = area.origin.y + area.size.h / 2;
  return p;
}

static GPoint proj_of(const Proj *p, int idx) {
  Station *s = &g_stations[idx];
  float dx = s->x - p->cx, dz = s->z - p->cz;
  if (!s->deep)
    return GPoint(p->mx + (int)(dx * p->scale), p->my + (int)(dz * p->scale));
  // Ride the bearing out to whichever frame edge it meets first. Scaling the
  // raw bearing finds that edge without ever needing its length.
  int hw = p->area.size.w / 2 - 7, hh = p->area.size.h / 2 - 7;
  if (hw < 4) hw = 4;
  if (hh < 4) hh = 4;
  float ax = dx < 0 ? -dx : dx, az = dz < 0 ? -dz : dz;
  float t = 1e9f;
  if (ax > 1e-4f) { float tx = hw / ax; if (tx < t) t = tx; }
  if (az > 1e-4f) { float tz = hh / az; if (tz < t) t = tz; }
  if (t > 1e8f) t = 0;                // degenerate: sits on the centre
  return GPoint(p->mx + (int)(dx * t), p->my + (int)(dz * t));
}

static void draw(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  bool round = IS_ROUND, compact = IS_COMPACT(b);
  int top_h = round ? (compact ? 46 : 56) : (compact ? 34 : 38);
  int bot_h = round ? (compact ? 58 : 66) : (compact ? 54 : 62);
  int inset = round ? 24 : 0;
  GRect map_area = GRect(inset, top_h, b.size.w - 2 * inset, b.size.h - top_h - bot_h);
  Proj pj = make_proj(map_area);

  char buf[96], t1[16], t2[16], t3[16];

  // --- top bar: station name + pilot vitals
  graphics_context_set_text_color(ctx, COL_GOLD);
  graphics_draw_text(ctx, g_stations[g.station].name,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(round ? 30 : 4, round ? (compact ? 4 : 16) : -3, b.size.w - (round ? 60 : 8), 20),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  fmt1(t1, sizeof t1, g.fuel);
  fmt1(t2, sizeof t2, tank_cap());
  fmt1(t3, sizeof t3, g.pilot_age);
  if (compact) snprintf(buf, sizeof buf, "$%ld  dv %s  a %s", (long)g.credits, t1, t3);
  else snprintf(buf, sizeof buf, "$%ld   dv %s/%s   age %s", (long)g.credits, t1, t2, t3);
  graphics_context_set_text_color(ctx, COL_DIM);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, round ? (compact ? 24 : 36) : (compact ? 15 : 17), b.size.w, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // --- selected route (under the dots)
  GPoint here = proj_of(&pj, g.station);
  if (s_sel < 3) {
    GPoint dst = proj_of(&pj, g_offers[s_sel].to);
    graphics_context_set_stroke_color(ctx, COL_GOLD);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx, here, dst);
  }

  // --- stations
  for (int i = 0; i < g_n_stations; i++) {
    Station *s = &g_stations[i];
    if (s->deep && !g.deep_license) continue;
    GPoint p = proj_of(&pj, i);
    bool is_target = s_sel < 3 && g_offers[s_sel].to == i;
    bool is_here = (i == (int)g.station);
    bool is_offer = false;
    for (int o = 0; o < 3; o++) if (g_offers[o].to == i) is_offer = true;
    if (s->deep) {
      // signpost on the frame edge — a square, so it never reads as a
      // cluster station sitting at that distance
      int r = (is_target || is_here) ? 3 : 2;
      graphics_context_set_fill_color(ctx, is_here ? COL_GOLD
                                                   : (is_target ? COL_CYAN : COL_DIM));
      graphics_fill_rect(ctx, GRect(p.x - r, p.y - r, r * 2 + 1, r * 2 + 1), 0, GCornerNone);
      if (is_here) {
        graphics_context_set_stroke_color(ctx, COL_GOLD);
        graphics_context_set_stroke_width(ctx, 1);
        graphics_draw_circle(ctx, p, 6);
      }
    } else if (is_here) {
      graphics_context_set_fill_color(ctx, COL_GOLD);
      graphics_fill_circle(ctx, p, 3);
      graphics_context_set_stroke_color(ctx, COL_GOLD);
      graphics_context_set_stroke_width(ctx, 1);
      graphics_draw_circle(ctx, p, 6);
    } else if (is_target) {
      graphics_context_set_fill_color(ctx, COL_CYAN);
      graphics_fill_circle(ctx, p, 4);
    } else if (is_offer) {
      graphics_context_set_fill_color(ctx, COL_BLUE);
      graphics_fill_circle(ctx, p, 3);
    } else {
      graphics_context_set_fill_color(ctx, GColorWhite);   // plain core station
      graphics_fill_circle(ctx, p, 2);
    }
    // label the selected target so the route reads at a glance
    if (is_target) {
      char nm[NAME_LEN];
      station_short_name(i, nm, sizeof nm);
      int lx = p.x < b.size.w / 2 ? p.x + 6 : p.x - 66;
      int ly = p.y < map_area.origin.y + 14 ? p.y + 4 : p.y - 16;
      graphics_context_set_text_color(ctx, COL_CYAN);
      graphics_draw_text(ctx, nm, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                         GRect(lx, ly, 62, 16), GTextOverflowModeTrailingEllipsis,
                         p.x < b.size.w / 2 ? GTextAlignmentLeft : GTextAlignmentRight, NULL);
    }
  }

  // --- bottom panel
  int y = b.size.h - bot_h;
  int px = round ? (compact ? 22 : 30) : 4;
  int pw = b.size.w - 2 * px;
  GTextAlignment align = round ? GTextAlignmentCenter : GTextAlignmentLeft;
  int l2 = y + (compact ? 15 : 17), l3 = y + (compact ? 29 : 33);
  graphics_context_set_stroke_color(ctx, COL_FAINT);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(inset, y), GPoint(b.size.w - inset, y));

  if (s_sel < 3) {
    Contract *c = &g_offers[s_sel];
    char nm[NAME_LEN];
    station_short_name(c->to, nm, sizeof nm);
    snprintf(buf, sizeof buf, "%d/3 %s > %s", s_sel + 1,
             c->type == 0 ? "CARGO" : "PAX", nm);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(px, y - 2, pw, 20), GTextOverflowModeTrailingEllipsis, align, NULL);
    graphics_context_set_text_color(ctx, COL_DIM);
    graphics_draw_text(ctx, c->what, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(px, l2, pw, 16), GTextOverflowModeTrailingEllipsis, align, NULL);
    RunPlan p = game_plan(c);
    bool ok = p.deadline_ok && p.aging_ok && p.retire_ok;
    if (round)   // the circle bottom is narrow — deadline detail lives on the card
      snprintf(buf, sizeof buf, "$%ld  %dg  %dly  %s",
               (long)contract_pay(c), c->g_limit, (int)(c->d + 0.5f), ok ? "OK" : "!!");
    else if (compact)
      snprintf(buf, sizeof buf, "$%ld %dg %dly DL%d %s",
               (long)contract_pay(c), c->g_limit, (int)(c->d + 0.5f),
               (int)(c->deadline + 0.5f), ok ? "OK" : "!!");
    else {
      fmt_years(t1, sizeof t1, c->deadline);
      snprintf(buf, sizeof buf, "$%ld  %dg  %dly  DL %s %s",
               (long)contract_pay(c), c->g_limit, (int)(c->d + 0.5f), t1,
               ok ? "OK" : "!!");
    }
    graphics_context_set_text_color(ctx, ok ? COL_GOOD : COL_BAD);
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(px, l3, pw, 16), GTextOverflowModeTrailingEllipsis, align, NULL);
  } else {
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, "FUEL & OUTFITTING",
                       fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(px, y - 2, pw, 20), GTextOverflowModeTrailingEllipsis, align, NULL);
    const char *ev = g.dock_event >= 0 ? DOCK_EVENTS[g.dock_event].txt
                                       : "Steady trade at this dock.";
    graphics_context_set_text_color(ctx, g.dock_event >= 0 ? COL_GOLD : COL_DIM);
    graphics_draw_text(ctx, ev, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(px, l2, pw, 32), GTextOverflowModeWordWrap, align, NULL);
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
