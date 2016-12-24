#include <pebble.h>
#include "vibrate.h"

// The alarm is ringing on the phone!

// Declarations

static TextLayer *vibrate_text;
static AppTimer *revibrate;

static const uint32_t const vibe_segments[] = { 100,400, 200,300,
                                                  400,100, 400,100,
                                                  300,200, 300,200,
                                                  300,200, 300,200,
                                                  1000,
                                                  500, 500,
                                                  1000,
                                                  400,100, 400,100,
                                                  700,100, 100,100,
                                                  300,200, 300,200
                                                 };

void enqueue_vibration(void* ignored) {
    vibes_enqueue_custom_pattern((VibePattern){
          .durations = vibe_segments,
          .num_segments = ARRAY_LENGTH(vibe_segments),
      });
}

void vibe_it(void* ignored) {
	if(revibrate == NULL) {
		enqueue_vibration(NULL);
		revibrate = app_timer_register(15000, vibe_it, NULL);
	}
}

void stop_vibing() {
    vibes_cancel();
	if(revibrate != NULL)
        app_timer_cancel(revibrate);
}

void actually_exit(void* ignored) {
	// Firmware 4.3 crashes if you exit immediately from notification
	window_stack_pop_all(false);
}

#define KEY_VIBE 0
#define KEY_CANCEL 0
#define KEY_WATCH_APP_HAS_STARTED 7

void tell_alive(void* ignored) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Telling phone I am alive!");
	DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_int8(iter, KEY_WATCH_APP_HAS_STARTED, 1);
    app_message_outbox_send();
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    Tuple *cancel_tuple = dict_find(iterator, KEY_VIBE);
    
	if(cancel_tuple) {
		if (cancel_tuple->value->int8 == KEY_CANCEL) {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Phone cancelled vibration!");
			stop_vibing();
			app_timer_register(500, actually_exit, NULL);
		} else {
			tell_alive(NULL);
		}
	}
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
	app_timer_register(500, tell_alive, NULL);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void vibrate_window_load(Window *window) {
	vibe_it(NULL);
    
	Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Load

	window_set_background_color(window, GColorRed);
	
	GRect bounds_vibrate_text = GRect(0, (bounds.size.h/2 - 14), bounds.size.w, 30);
	vibrate_text = text_layer_create(bounds_vibrate_text);
	text_layer_set_font(vibrate_text, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(vibrate_text, GTextAlignmentCenter);
	text_layer_set_background_color(vibrate_text, GColorClear);
	text_layer_set_text_color(vibrate_text, GColorWhite);
	text_layer_set_text(vibrate_text, "Alarmist");

	// Add Layers

	layer_add_child(window_layer, text_layer_get_layer(vibrate_text));

	tell_alive(NULL);

}

static void vibrate_window_unload(Window *window) {
    stop_vibing();
	text_layer_destroy(vibrate_text);
}

Window *vibrate_window_get_window(VibrateWindow *window) {
    return (Window *)window;
}

VibrateWindow *vibrate_window_create() {
    Window *window = window_create();

	app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
	app_message_register_inbox_received(inbox_received_callback);
	app_message_open(128, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
	
    window_set_window_handlers(window, (WindowHandlers) {
            .load = vibrate_window_load,
            .unload = vibrate_window_unload
    });

    return (VibrateWindow *) window;
}

VibrateWindow *vibrate_window_push(bool animated) {
    VibrateWindow *window = vibrate_window_create();
    window_stack_push(vibrate_window_get_window(window), animated);
    return (VibrateWindow *) window;
}

void vibrate_window_destroy(VibrateWindow *window) {
    if(!window)return;
    window_destroy((Window *)window);
}