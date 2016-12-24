#include <pebble.h>
#include "next.h"
#include "create.h"
#include "messenger.h"

static Window *s_window;
static char stime[6];
static char sname[32];
static char ssoon[32];
static TextLayer *t_time;
static TextLayer *t_once;
static TextLayer *t_soon;
static TextLayer *t_set_alarm;
static TextLayer *t_dismiss;
static StatusBarLayer *status_bar;
static int unix_mins = 0;

#define FONT_SIZE 48
#define FONT_WIDTH 24

static void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "selected");
    push_create_window();
}

static void update_soon() {
    if(unix_mins > 0) {
        int diffMins = unix_mins - (time(NULL)/60);
        if(diffMins == 1)
            snprintf(ssoon, 32, "in %d minutes", diffMins);
        if(diffMins < 60)
            snprintf(ssoon, 32, "in %d minutes", diffMins);
        else if(diffMins < 1440)
            snprintf(ssoon, 32, "in %d hrs %d mins", diffMins / 60, diffMins % 60);
        else
            snprintf(ssoon, 32, "in %d d %d h %d m", diffMins / 1440, (diffMins % 1440) / 60, diffMins % 60);
    } else
        snprintf(ssoon, 32, "%s", "");
    
    text_layer_set_text(t_soon, ssoon);
}

static void ontick(struct tm *tick_time, TimeUnits units_changed) {
    update_soon();
}

void alarm_updated_handler (bool exists, Alarm *next_alarm) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "new alarm received");
    if(exists) {
        time_t t_sec = next_alarm->unix_mins * 60;
        struct tm* alarmTime = localtime(&t_sec);
        strftime(stime, 6, "%H:%M", alarmTime);
        text_layer_set_text(t_time, stime);
        snprintf(sname, 32, "%s", next_alarm->name);
        text_layer_set_text(t_once, sname);
        unix_mins = next_alarm->unix_mins;
        layer_set_hidden(text_layer_get_layer(t_dismiss), !next_alarm->dismissable);
    } else {
        unix_mins = 0;
        snprintf(stime, 6, "%s", ":");
        text_layer_set_text(t_time, stime);
        snprintf(sname, 32, "%s", "No alarm set");
        text_layer_set_text(t_once, sname);
        layer_set_hidden(text_layer_get_layer(t_dismiss), true);
    }
    update_soon();
}

static void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    layer_set_hidden(text_layer_get_layer(t_dismiss), true);
    predismiss();
}

/*
static void fake_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    Alarm fake = (Alarm) {
        .dismissable = true,
        .unix_mins = 1451544000 / 60,
    };
    snprintf(fake.name, 32, "%s", "Catch flight");
    alarm_updated_handler(true, &fake);
}*/

static void config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
  //window_single_click_subscribe(BUTTON_ID_UP, fake_single_click_handler);
}

static void request_update(void *ignored) {
    update_alarm(alarm_updated_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
    
    status_bar = status_bar_layer_create();
    layer_set_frame(status_bar_layer_get_layer(status_bar), GRect(0, 0, bounds.size.w, STATUS_BAR_LAYER_HEIGHT));
    layer_add_child(window_layer, status_bar_layer_get_layer(status_bar));
    
    #define MARGIN 10
    #define T_BG_C GColorVividCerulean
    #define T_C GColorBlack
    static int8_t text_origin_y = STATUS_BAR_LAYER_HEIGHT + 20;
    t_time = text_layer_create((GRect) {
        .origin = {MARGIN, text_origin_y},
        .size = {bounds.size.w - MARGIN*2, FONT_SIZE + 4}
    });
    text_layer_set_text_alignment(t_time, GTextAlignmentCenter);
    text_layer_set_background_color(t_time, GColorClear);
    text_layer_set_text_color(t_time, GColorDarkGreen);
    text_layer_set_text(t_time, "");
    mono = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MONO_48));
    text_layer_set_font(t_time, mono);
    
    t_once = text_layer_create((GRect) {
        .origin = {1, text_origin_y + FONT_SIZE},
        .size = {bounds.size.w - 2, 28}
    });
    text_layer_set_text_alignment(t_once, GTextAlignmentCenter);
    text_layer_set_background_color(t_once, GColorClear);
    text_layer_set_font(t_once, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text(t_time, "Wake up");
    
    t_soon = text_layer_create((GRect) {
        .origin = {MARGIN, text_origin_y + FONT_SIZE + 26},
        .size = {bounds.size.w - MARGIN*2, 28}
    });
    text_layer_set_text_alignment(t_soon, GTextAlignmentCenter);
    text_layer_set_background_color(t_soon, GColorClear);
    text_layer_set_font(t_soon, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text(t_soon, "Wake up");
    
    t_set_alarm = text_layer_create((GRect) {
        .origin = {10, STATUS_BAR_LAYER_HEIGHT},
        .size = {bounds.size.w - 20, 20}
    });
    text_layer_set_text_alignment(t_set_alarm, GTextAlignmentCenter);
    text_layer_set_background_color(t_set_alarm, GColorClear);
    text_layer_set_font(t_set_alarm, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text(t_set_alarm, "Next alarm");
    
    t_dismiss = text_layer_create((GRect) {
        .origin = {PBL_IF_ROUND_ELSE(bounds.size.w /2-17, bounds.size.w - 57), bounds.size.h - 29},
        .size = {PBL_IF_ROUND_ELSE(bounds.size.w /2,57), 24}
    });
    text_layer_set_text_alignment(t_dismiss, PBL_IF_ROUND_ELSE(GTextAlignmentLeft, GTextAlignmentRight));
    text_layer_set_background_color(t_dismiss, GColorBlack);
    text_layer_set_text_color(t_dismiss, PBL_IF_COLOR_ELSE(GColorRed, GColorWhite));
    text_layer_set_font(t_dismiss, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text(t_dismiss, PBL_IF_ROUND_ELSE(" Dismiss", "Dismiss "));
    
    layer_add_child(window_layer, text_layer_get_layer(t_time));
    layer_add_child(window_layer, text_layer_get_layer(t_once));
    layer_add_child(window_layer, text_layer_get_layer(t_soon));
    layer_add_child(window_layer, text_layer_get_layer(t_set_alarm));
    layer_add_child(window_layer, text_layer_get_layer(t_dismiss));
    window_set_click_config_provider(s_window, config_provider);
    
    launch_messenger();
    alarm_updated_handler(false, NULL);
    app_timer_register(100, request_update, NULL);
    app_timer_register(2000, request_update, NULL);
    tick_timer_service_subscribe(MINUTE_UNIT, ontick);
}

static void window_unload(Window *window) {
  // Destroy output TextLayer
    text_layer_destroy(t_time);
    text_layer_destroy(t_once);
    text_layer_destroy(t_soon);
    text_layer_destroy(t_set_alarm);
    status_bar_layer_destroy(status_bar);
    fonts_unload_custom_font(mono);
    destroy_messenger();
}


void push_next_window() {
	s_window = window_create();

	#ifdef PBL_SDK_2
		window_set_fullscreen(s_window, true);
	#endif
    
	window_set_window_handlers(s_window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});

	window_stack_push(s_window, true);
    window_set_click_config_provider(s_window, config_provider);
}