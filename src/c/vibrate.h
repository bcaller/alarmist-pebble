#pragma once
#include "pebble.h"

typedef struct VibrateWindow VibrateWindow;
Window *vibrate_window_get_window(VibrateWindow *window);
VibrateWindow *vibrate_window_create();
VibrateWindow *vibrate_window_push(bool animated);
void vibrate_window_destroy(VibrateWindow *window);
void vibe_it(void* ignored);
void tell_alive(void* ignored);
void stop_vibing();