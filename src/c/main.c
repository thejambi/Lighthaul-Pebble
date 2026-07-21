#include <pebble.h>
#include "game.h"
#include "ui.h"

int main(void) {
  game_init();
  win_title_push();
  app_event_loop();
  game_save();
}
