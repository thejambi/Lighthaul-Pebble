#include "ui.h"

// Fuel & outfitting — a MenuLayer over the dock's fuel pump and its two-item
// upgrade shop. Bulk fuel discount and dock-event pricing both apply.

static Window *s_win;
static MenuLayer *s_menu;

#define SEC_FUEL 0
#define SEC_SHOP 1

static uint16_t get_num_sections(MenuLayer *m, void *ctx) { return 2; }

static uint16_t get_num_rows(MenuLayer *m, uint16_t section, void *ctx) {
  if (section == SEC_FUEL) return tow_available() ? 3 : 2;
  return 2;
}

static int16_t get_header_height(MenuLayer *m, uint16_t section, void *ctx) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void draw_header(GContext *ctx, const Layer *cell, uint16_t section, void *data) {
  if (section == SEC_FUEL) {
    char buf[40] = "FUEL";
    if (g.dock_event >= 0 && DOCK_EVENTS[g.dock_event].fuel_pct != 100)
      snprintf(buf, sizeof buf, "FUEL  (dock: x%d%%)", DOCK_EVENTS[g.dock_event].fuel_pct);
    menu_cell_basic_header_draw(ctx, cell, buf);
  } else {
    char buf[40] = "OUTFITTER";
    if (g.dock_event >= 0 && DOCK_EVENTS[g.dock_event].shop_pct != 100)
      snprintf(buf, sizeof buf, "OUTFITTER  (-25%% today)");
    menu_cell_basic_header_draw(ctx, cell, buf);
  }
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *idx, void *data) {
  char title[40], sub[44], t1[16], t2[16];
  float missing = tank_cap() - g.fuel;

  if (idx->section == SEC_FUEL) {
    if (idx->row == 0) {
      float q = missing < 2.0f ? missing : 2.0f;
      fmt1(t1, sizeof t1, q);
      snprintf(title, sizeof title, "TOP UP +%s dv", t1);
      if (q < 0.05f) snprintf(sub, sizeof sub, "tank is full");
      else snprintf(sub, sizeof sub, "$%ld", (long)fuel_cost(q));
    } else if (idx->row == 1) {
      snprintf(title, sizeof title, "FILL TANK");
      if (missing < 0.05f) snprintf(sub, sizeof sub, "tank is full");
      else {
        fmt1(t1, sizeof t1, missing);
        snprintf(sub, sizeof sub, "+%s dv for $%ld (bulk rate)", t1, (long)fuel_cost(missing));
      }
    } else {
      snprintf(title, sizeof title, "RECOVERY TOW");
      snprintf(sub, sizeof sub, "half tank: $%ld + 4yr of life", (long)tow_cost());
    }
  } else {
    int id = g_stations[g.station].shop[idx->row];
    const UpgradeDef *u = &UPGRADES[id];
    int lvl = g.upgrades[id];
    snprintf(title, sizeof title, "%s %d/%d", u->name, lvl, u->n_levels);
    int32_t cost = upgrade_cost(id);
    if (cost < 0) snprintf(sub, sizeof sub, "%s - MAXED", u->desc);
    else snprintf(sub, sizeof sub, "%s - $%ld", u->desc, (long)cost);
  }
  (void)t2;
  menu_cell_basic_draw(ctx, cell, title, sub, NULL);
}

static void select_row(MenuLayer *m, MenuIndex *idx, void *data) {
  float missing = tank_cap() - g.fuel;
  bool bought = false;
  if (idx->section == SEC_FUEL && idx->row == 2) {
    guild_tow();
    bought = true;
  } else if (idx->section == SEC_FUEL) {
    float q = idx->row == 0 ? (missing < 2.0f ? missing : 2.0f) : missing;
    if (q > 0.05f && g.credits >= fuel_cost(0.1f)) {
      buy_fuel(q);
      bought = true;
    }
  } else {
    bought = buy_upgrade(g_stations[g.station].shop[idx->row]);
  }
  if (bought) vibes_short_pulse();
  menu_layer_reload_data(s_menu);
}

static void win_load(Window *w) {
  Layer *root = window_get_root_layer(w);
  GRect b = layer_get_bounds(root);
  s_menu = menu_layer_create(b);
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_sections = get_num_sections,
    .get_num_rows = get_num_rows,
    .get_header_height = get_header_height,
    .draw_header = draw_header,
    .draw_row = draw_row,
    .select_click = select_row,
  });
  menu_layer_set_normal_colors(s_menu, GColorBlack, GColorWhite);
  menu_layer_set_highlight_colors(s_menu, PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite), GColorBlack);
  menu_layer_set_click_config_onto_window(s_menu, w);
  layer_add_child(root, menu_layer_get_layer(s_menu));
}

static void win_unload(Window *w) { menu_layer_destroy(s_menu); s_menu = NULL; }

void win_outfit_push(void) {
  if (!s_win) {
    s_win = window_create();
    window_set_window_handlers(s_win, (WindowHandlers){ .load = win_load, .unload = win_unload });
  }
  window_stack_push(s_win, true);
}
