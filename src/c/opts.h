#pragma once
#include <pebble.h>

// In-app options. This game has buttons and no phone-side config page, so its
// settings live on the watch. Persisted under their own key, well clear of the
// career blobs, so starting a new career never touches your preferences.

#define OPTS_VERSION 1
typedef struct {
  uint8_t version;
  uint8_t vibe;        // the haptic pats: arrival, and a purchase going through
} LhOpts;
extern LhOpts g_opts;

void opts_init(void);
void opts_save(void);
// true when a buzz is both wanted and polite right now
bool opts_may_vibe(void);
