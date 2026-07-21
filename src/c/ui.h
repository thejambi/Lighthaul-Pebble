#pragma once
#include <pebble.h>
#include "game.h"

// Pebble's snprintf has no %f — small fixed-point formatters instead.
void fmt1(char *buf, size_t cap, double v);            // "12.3"
void fmt_years(char *buf, size_t cap, double y);       // "88.3yr" / "1.23kyr"
void fmt_beta(char *buf, size_t cap, double beta);     // "0.9973c"
void fmt_gamma(char *buf, size_t cap, double gamma);   // "13.6" / "25k"

void win_title_push(void);
void win_about_push(void);
void win_map_push(void);
void win_map_refresh(void);
void win_contract_push(int offer_idx);
void win_flight_push(int offer_idx);
void win_results_push(void);
void win_outfit_push(void);
