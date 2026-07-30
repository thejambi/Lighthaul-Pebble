#include <pebble.h>
#include "game.h"
#include "ui.h"
#include "opts.h"

int main(void) {
  opts_init();
  game_init();
  win_title_push();
  app_event_loop();
  game_save();
}
