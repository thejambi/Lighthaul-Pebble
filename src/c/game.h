#pragma once
#include <pebble.h>
#include "world.h"

#define START_AGE 22.0f
#define RETIRE_AGE 82.0f
#define BASE_TANK 14.0f
#define FUEL_PRICE 20.0
#define FUEL_BULK_DISC 0.02
#define FUEL_MAX_DISC 0.35
#define C_CAP 0.9999995
#define DEEP_GAMMA 25000.0
#define REP_PER_DELIVERY 0.03
#define REP_MAX 0.75

enum { UP_TANK, UP_DRIVE, UP_DAMPER, UP_BROKER, UP_REJUV, UP_OVERDRIVE, UP_AUTOPILOT, N_UPGRADES };

typedef struct {
  const char *name, *desc;
  uint8_t n_levels;
  uint16_t costs[6];
} UpgradeDef;
extern const UpgradeDef UPGRADES[N_UPGRADES];

typedef struct {
  char seed[10];
  uint32_t rng_state;           // world RNG mid-career (contracts keep rolling)
  int32_t credits, earned;
  float fuel;                   // Δv remaining, in rapidity units
  float pilot_age;              // ship-time years
  float uni_time;               // universe-years elapsed this career
  uint8_t station;
  uint8_t upgrades[N_UPGRADES];
  uint16_t deliveries, failures;
  bool deep_license;
  int8_t dock_event;            // index into DOCK_EVENTS, -1 = none
  float max_gamma;
} Game;

extern Game g;
extern Contract g_offers[3];

// The auto-run flight plan: burn the whole usable tank (pilot time is the real
// currency), capped by the redline governor — then check what that buys.
typedef struct {
  double beta, gamma;
  float dv;                     // fuel this run actually consumes
  float t_uni, t_ship;          // universe-years, ship-years
  bool deadline_ok, aging_ok, retire_ok;
} RunPlan;

typedef struct {
  bool ok, late, aged_out, retired, licensed;
  uint8_t type;
  int32_t pay;
  float t_uni, t_ship;
  double gamma, beta;
  char to_name[NAME_LEN];
  char what[WHAT_LEN];
  char vignette[128];
} RunResult;
extern RunResult g_last;

// derived dials
float tank_cap(void);
float retire_age(void);
float fuel_factor(void);
double cap_beta(void);
int32_t contract_pay(const Contract *c);   // base pay × broker × reputation

RunPlan game_plan(const Contract *c);
void game_resolve(const Contract *c);      // apply plan, move dock, reroll offers

// fuel & shop
int32_t fuel_cost(float qty);              // bulk discount + dock event pricing
void buy_fuel(float qty);                  // clamped to tank space & credits
int32_t upgrade_cost(int id);              // next level, event-adjusted; -1 if maxed
bool buy_upgrade(int id);

const char *rank_for(int32_t balance);

void game_new(const char *seed_or_null);
void game_init(void);                      // load save or start fresh
void game_save(void);
