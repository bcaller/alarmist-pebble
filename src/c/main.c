#include <pebble.h>
#include "next.h"
#include "vibrate.h"

static Window *s_main_window;

static void init() {
  // Create main Window
    if(launch_reason() == APP_LAUNCH_PHONE) {
        // Launching to vibrate for alarm only
      vibrate_window_push(true);
    } else {
        push_next_window();
    }
}

static void deinit() {
  // Destroy main Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}