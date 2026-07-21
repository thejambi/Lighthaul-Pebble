#include "game.h"
#include "rng.h"
#include "fastmath.h"

Game g;
Contract g_offers[3];
RunResult g_last;

const UpgradeDef UPGRADES[N_UPGRADES] = {
  [UP_TANK]      = { "Fuel Cell Array",  "+3 dv tank",          4, {200, 320, 460, 620} },
  [UP_DRIVE]     = { "Drive Efficiency", "-12% fuel per dv",    3, {240, 400, 560} },
  [UP_DAMPER]    = { "Inertial Dampers", "-15% felt load",      3, {260, 420, 600} },
  [UP_BROKER]    = { "Broker License",   "+8% contract pay",    3, {280, 460, 640} },
  [UP_REJUV]     = { "Rejuv Course",     "+6 yr career",        4, {300, 460, 640, 840} },
  [UP_OVERDRIVE] = { "Redline Coils",    "a 9 nearer c",        6, {400, 720, 1150, 1700, 2400, 3300} },
  [UP_AUTOPILOT] = { "Docking Assist",   "auto-brake to dock",  1, {500} },
};

float tank_cap(void)    { return BASE_TANK + g.upgrades[UP_TANK] * 3.0f; }
float retire_age(void)  { return RETIRE_AGE + g.upgrades[UP_REJUV] * 6.0f; }
float fuel_factor(void) { return 1.0f - g.upgrades[UP_DRIVE] * 0.12f; }
static double pay_mult(void) { return 1.0 + g.upgrades[UP_BROKER] * 0.08; }
static double rep_mult(void) {
  double r = g.deliveries * REP_PER_DELIVERY;
  return 1.0 + (r > REP_MAX ? REP_MAX : r);
}

double cap_beta(void) {
  double b = 1.0 - (1.0 - C_CAP) * lh_pow10i(-(int)g.upgrades[UP_OVERDRIVE]);
  return b > 1.0 - 5.1e-13 ? 1.0 - 4.9995e-13 : b;
}

int32_t contract_pay(const Contract *c) {
  return (int32_t)(c->pay * pay_mult() * rep_mult() + 0.5);
}

// ---------------------------------------------------------------------------
// Flight planning & resolution — same clock math as the web game's HUD:
// deadlines on universe time d/β, everyone aboard ages ship time d/(βγ),
// Δv is rapidity and you pay it again to brake.
// ---------------------------------------------------------------------------
RunPlan game_plan(const Contract *c) {
  RunPlan p;
  double phi_avail = g.fuel / (2.0 * fuel_factor());
  double beta = lh_tanh(phi_avail);
  double cap = cap_beta();
  if (beta > cap) beta = cap;
  if (beta < 1e-6) beta = 1e-6;
  double gamma = 1.0 / lh_sqrt((1.0 - beta) * (1.0 + beta));
  p.beta = beta;
  p.gamma = gamma;
  p.dv = (float)(2.0 * lh_atanh(beta) * fuel_factor());
  p.t_uni = (float)(c->d / beta);
  p.t_ship = (float)(c->d / (beta * gamma));
  p.deadline_ok = p.t_uni <= c->deadline;
  p.aging_ok = c->type == 0 || p.t_ship <= c->max_aging;
  p.retire_ok = g.pilot_age + p.t_ship < retire_age();
  return p;
}

// One line of the story the clocks write: how long this dock has waited.
static void make_vignette(char *out, size_t cap, int st_idx, float now) {
  Station *st = &g_stations[st_idx];
  char nm[NAME_LEN];
  station_short_name(st_idx, nm, sizeof nm);
  if (st->last_visit < 0) {
    snprintf(out, cap, "First call at %s - the Guild ledger opens a page for you.", nm);
    return;
  }
  int gap = (int)(now - st->last_visit);
  if (gap < 60)
    snprintf(out, cap, "%d yr since you left - the dockmaster still remembers your name.", gap);
  else if (gap < 300)
    snprintf(out, cap, "%d yr gone. Everyone you dealt with here has retired.", gap);
  else if (gap < 1000)
    snprintf(out, cap, "You left a town and docked at a city - %d yr of history passed in cruise.", gap);
  else
    snprintf(out, cap, "%d yr since you stood here. Your landing is in their founding mural.", gap);
}

void game_resolve(const Contract *c) {
  RunPlan p = game_plan(c);
  memset(&g_last, 0, sizeof g_last);
  g_last.type = c->type;
  g_last.t_uni = p.t_uni;
  g_last.t_ship = p.t_ship;
  g_last.beta = p.beta;
  g_last.gamma = p.gamma;
  strncpy(g_last.to_name, g_stations[c->to].name, NAME_LEN - 1);
  strncpy(g_last.what, c->what, WHAT_LEN - 1);

  int32_t pay = contract_pay(c);
  bool ok = true;
  if (!p.deadline_ok) { ok = false; g_last.late = true; pay = (int32_t)(pay * 0.25 + 0.5); }
  if (c->type == 1 && !p.aging_ok) { ok = false; g_last.aged_out = true; pay = (int32_t)(pay * 0.2 + 0.5); }
  g_last.ok = ok;
  g_last.pay = pay;

  g.credits += pay;
  g.earned += pay;
  if (ok) g.deliveries++; else g.failures++;
  g.fuel -= p.dv;
  if (g.fuel < 0) g.fuel = 0;
  g.pilot_age += p.t_ship;
  g.uni_time += p.t_uni;
  if (p.gamma > g.max_gamma) g.max_gamma = (float)p.gamma;

  if (!g.deep_license && ok && p.gamma >= DEEP_GAMMA) {
    g.deep_license = true;
    g_last.licensed = true;
  }

  if (rand() % 4 == 0) make_vignette(g_last.vignette, sizeof g_last.vignette, c->to, g.uni_time);
  g_stations[c->to].last_visit = g.uni_time;
  g.station = c->to;
  g_last.retired = g.pilot_age >= retire_age();

  // the next dock's economy and offer board come off the same seeded stream
  g.dock_event = world_roll_dock_event();
  uint16_t pay_pct = g.dock_event >= 0 ? DOCK_EVENTS[g.dock_event].pay_pct : 100;
  world_make_contracts(g.station, g.deep_license, pay_pct, g_offers);
  g.rng_state = rng_get_state();
  game_save();
}

// ---------------------------------------------------------------------------
// Fuel & shop
// ---------------------------------------------------------------------------
static uint16_t event_pct(int which) {
  if (g.dock_event < 0) return 100;
  const DockEvent *e = &DOCK_EVENTS[g.dock_event];
  return which == 0 ? e->fuel_pct : which == 2 ? e->shop_pct : e->pay_pct;
}

int32_t fuel_cost(float qty) {
  if (qty <= 0) return 0;
  double disc = qty * FUEL_BULK_DISC;
  if (disc > FUEL_MAX_DISC) disc = FUEL_MAX_DISC;
  return (int32_t)(qty * FUEL_PRICE * (1.0 - disc) * event_pct(0) / 100.0 + 0.5);
}

void buy_fuel(float qty) {
  float missing = tank_cap() - g.fuel;
  if (qty > missing) qty = missing;
  while (qty > 0.05f && fuel_cost(qty) > g.credits) qty -= 0.1f;  // buy what you can afford
  if (qty <= 0.05f) return;
  g.credits -= fuel_cost(qty);
  g.fuel += qty;
  if (g.fuel > tank_cap()) g.fuel = tank_cap();
  game_save();
}

int32_t upgrade_cost(int id) {
  int lvl = g.upgrades[id];
  if (lvl >= UPGRADES[id].n_levels) return -1;
  return (int32_t)((int64_t)UPGRADES[id].costs[lvl] * event_pct(2) / 100);
}

bool buy_upgrade(int id) {
  int32_t cost = upgrade_cost(id);
  if (cost < 0 || cost > g.credits) return false;
  g.credits -= cost;
  g.upgrades[id]++;
  game_save();
  return true;
}

const char *rank_for(int32_t bal) {
  if (bal >= 128000) return "The Ageless";
  if (bal >= 64000)  return "Deep Space Magnate";
  if (bal >= 32000)  return "Redline Royalty";
  if (bal >= 16000)  return "Lightspeed Legend";
  if (bal >= 10000)  return "Void Baron";
  if (bal >= 6000)   return "Master Courier";
  if (bal >= 3000)   return "Journeyman Courier";
  if (bal >= 1000)   return "Drifter";
  return "Deadhead";
}

// ---------------------------------------------------------------------------
// Careers & persistence
// ---------------------------------------------------------------------------
#define KEY_VERSION 0
#define KEY_GAME 1
#define KEY_OFFERS 2
#define KEY_VISITS 3
#define SAVE_VERSION 1

void game_new(const char *seed_or_null) {
  memset(&g, 0, sizeof g);
  if (seed_or_null && seed_or_null[0]) {
    strncpy(g.seed, seed_or_null, sizeof g.seed - 1);
  } else {
    static const char *b36 = "abcdefghijklmnopqrstuvwxyz0123456789";
    for (int i = 0; i < 6; i++) g.seed[i] = b36[rand() % 36];
    g.seed[6] = 0;
  }
  g.credits = 450;            // Courier-class opening balance
  g.fuel = BASE_TANK;
  g.pilot_age = START_AGE;
  g.max_gamma = 1;
  g.dock_event = -1;
  world_build(g.seed);
  g.dock_event = world_roll_dock_event();
  uint16_t pay_pct = g.dock_event >= 0 ? DOCK_EVENTS[g.dock_event].pay_pct : 100;
  world_make_contracts(0, false, pay_pct, g_offers);
  g.rng_state = rng_get_state();
  game_save();
}

void game_init(void) {
  srand(time(NULL));
  if (persist_exists(KEY_VERSION) && persist_read_int(KEY_VERSION) == SAVE_VERSION &&
      persist_exists(KEY_GAME) && persist_exists(KEY_OFFERS)) {
    persist_read_data(KEY_GAME, &g, sizeof g);
    world_build(g.seed);
    rng_set_state(g.rng_state);   // resume the contract stream where it left off
    persist_read_data(KEY_OFFERS, g_offers, sizeof g_offers);
    if (persist_exists(KEY_VISITS)) {
      float visits[MAX_STATIONS];
      persist_read_data(KEY_VISITS, visits, sizeof visits);
      for (int i = 0; i < g_n_stations; i++) g_stations[i].last_visit = visits[i];
    }
  } else {
    game_new(NULL);
  }
}

void game_save(void) {
  persist_write_int(KEY_VERSION, SAVE_VERSION);
  persist_write_data(KEY_GAME, &g, sizeof g);
  persist_write_data(KEY_OFFERS, g_offers, sizeof g_offers);
  float visits[MAX_STATIONS];
  for (int i = 0; i < MAX_STATIONS; i++)
    visits[i] = i < g_n_stations ? g_stations[i].last_visit : -1;
  persist_write_data(KEY_VISITS, visits, sizeof visits);
}
