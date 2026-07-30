#include "opts.h"

LhOpts g_opts;

#define KEY_OPTS 5        // 0..4 belong to the career, records and offers

void opts_init(void) {
  memset(&g_opts, 0, sizeof g_opts);
  g_opts.version = OPTS_VERSION;
  g_opts.vibe = 1;        // as the game has always behaved; off is a choice
  // Append-only: an older, shorter save reads over the defaults and stops
  // where it ends, leaving anything added since at its default.
  int n = persist_exists(KEY_OPTS) ? persist_get_size(KEY_OPTS) : 0;
  if (n > 0 && n <= (int)sizeof g_opts) {
    LhOpts tmp = g_opts;
    persist_read_data(KEY_OPTS, &tmp, n);
    if (tmp.version == OPTS_VERSION) g_opts = tmp;
  }
}

void opts_save(void) { persist_write_data(KEY_OPTS, &g_opts, sizeof g_opts); }

// Quiet Time outranks the setting — the rest of this family has always been
// silent then, and the game simply never asked.
bool opts_may_vibe(void) { return g_opts.vibe && !quiet_time_is_active(); }
