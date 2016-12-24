#include <pebble.h>
#include "create.h"
#include "messenger.h"

static Window *s_window;
static char stime[6];
static TextLayer *t_time;
static TextLayer *t_once;
static TextLayer *t_set_alarm;
static StatusBarLayer *status_bar;
static Layer *triangle_layer;
#define TRIANGLE_HEIGHT 9
static GPathInfo TRIANGLE_INFO = {
    3, (GPoint[]) {
        {0,0}, {TRIANGLE_HEIGHT*2/3, TRIANGLE_HEIGHT}, {-TRIANGLE_HEIGHT*2/3, TRIANGLE_HEIGHT}
    },
};
static GPath *triangle_path;
static GPath *upside_triangle_path;
static int8_t recurse_index = 0;
static char recurse[4][16] = {
    "Just once", "Weekly", "Weekdays", "Everyday"
};

    #define FONT_SIZE 48
    #define FONT_WIDTH 24
GFont mono;

static int text_origin_y;

#define S_HOUR 0
#define S_MIN_TEN 1
#define S_MIN_UNIT 2
#define S_ONCE 3
#define S_SEND 4

static int8_t selected = 0;
static int8_t hour = 7;
static int8_t minute = 30;

static void triangle_draw(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, GColorBlue);
    int8_t y_bottom = FONT_SIZE + TRIANGLE_HEIGHT * 2 + 6;
    static int8_t x;
    int8_t y = 1;
    if(selected == S_MIN_TEN)
        x = bounds.origin.x + bounds.size.w / 2 + FONT_WIDTH;
    else if(selected == S_MIN_UNIT)
        x = bounds.origin.x + bounds.size.w / 2 + FONT_WIDTH *2;
    else if(selected == S_HOUR)
        x = bounds.origin.x + bounds.size.w / 2 - FONT_WIDTH *3/2;
    else if(selected == S_ONCE) {
        x = bounds.size.w / 2 + bounds.origin.x;
        y = FONT_SIZE + TRIANGLE_HEIGHT*2 + 3;
        y_bottom = y + 34 + TRIANGLE_HEIGHT;
    }
    else return;
    
    gpath_move_to(triangle_path, (GPoint){ x, y });
    gpath_draw_filled(ctx, triangle_path);
    gpath_move_to(upside_triangle_path, (GPoint){ x, y_bottom });
    gpath_draw_filled(ctx, upside_triangle_path);
    
    GRect rect;
    if(selected == S_HOUR)
        rect = (GRect){ {bounds.origin.x + bounds.size.w / 2 - FONT_WIDTH *5/2 -3, TRIANGLE_HEIGHT+5}, {FONT_WIDTH * 2 +5, FONT_SIZE}};
    else if(selected == S_MIN_TEN || selected == S_MIN_UNIT)
        rect = (GRect){ {bounds.origin.x + bounds.size.w / 2 + FONT_WIDTH * (2*(selected-1)+1) /2 - 1, TRIANGLE_HEIGHT+5}, {FONT_WIDTH + 2, FONT_SIZE}};
    else if(selected == S_ONCE)
        rect = (GRect){ {bounds.origin.x + 19, y + TRIANGLE_HEIGHT + 1}, {bounds.size.w - 38, 24} };
    
    if(rect.size.w > 0) {
        graphics_context_set_fill_color(ctx, GColorYellow);
        graphics_context_set_stroke_color(ctx, GColorBlue);
        graphics_fill_rect(ctx, rect, 0, GCornerNone);
        graphics_draw_rect(ctx, rect);
    }
}

static void set_hour_text() {
    bool hourDouble = hour > 9;
    bool minDouble = minute > 9;
    snprintf(stime, 6, "%s%d:%s%d", hourDouble ? "" : "0", hour, minDouble ? "" : "0", minute);
    text_layer_set_text(t_time, stime);
    
    time_t temp = time(NULL);
    struct tm* now = localtime(&temp);
    if(now->tm_hour > hour || (now->tm_hour == hour && now->tm_min > minute )) {
        // Alarm actually set for tomorrow
        now->tm_wday = (now->tm_wday + 1) % 7;
        snprintf(recurse[0], 16, "%s", "Just tomorrow");
    } else
        snprintf(recurse[0], 16, "%s", "Just today");
    strftime(recurse[1], 12, "%As", now);
    if(now->tm_wday == 0 || now->tm_wday == 6) {
        // Weekend
        snprintf(recurse[2], 16, "%s", "Weekends");
    } else snprintf(recurse[2], 16, "%s", "Weekdays");
}

static void set_recurse_text() {
    text_layer_set_text(t_once, recurse[recurse_index]);  
}

static void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "selected");
    selected++;
    if(selected == S_SEND) {
        set_alarm(hour, minute, recurse_index);
        selected = 0;
        window_stack_pop(true);
        return;
    }
    layer_mark_dirty(triangle_layer);
}

static void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    if(selected == S_HOUR) {
        hour = (hour + 1) % 24;
        set_hour_text();
    } else if(selected == S_MIN_TEN) {
        minute = (minute + 10) % 60;
        set_hour_text();
    } else if(selected == S_MIN_UNIT) {
        if(minute % 10 == 9)
            minute -= 9;
        else
            minute = (minute + 1);
        set_hour_text();
    } else if(selected == S_ONCE) {
        recurse_index = (recurse_index + 1) % ARRAY_LENGTH(recurse);
        set_recurse_text();
    }
}

static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    if(selected == S_HOUR) {
        hour = (hour + 23) % 24;
        set_hour_text();
    } else if(selected == S_MIN_TEN) {
        minute = (minute + 50) % 60;
        set_hour_text();
    } else if(selected == S_MIN_UNIT) {
        if(minute % 10 == 0)
            minute+=9;
        else
            minute = (minute - 1);
        set_hour_text();
    } else if(selected == S_ONCE) {
        recurse_index = (recurse_index - 1 + ARRAY_LENGTH(recurse)) % ARRAY_LENGTH(recurse);
        set_recurse_text();
    }
}

static void back_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    if(selected == 0) {
        window_stack_pop(true);
    } else {
        selected--;
        layer_mark_dirty(triangle_layer);
    }
}

static void config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 500, up_single_click_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 500, down_single_click_handler);
    window_single_click_subscribe(BUTTON_ID_BACK, back_single_click_handler);
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
    text_origin_y = STATUS_BAR_LAYER_HEIGHT + 25;
    t_time = text_layer_create((GRect) {
        .origin = {MARGIN, text_origin_y},
        .size = {bounds.size.w - MARGIN*2, FONT_SIZE + 4}
    });
    set_hour_text();
    text_layer_set_text_alignment(t_time, GTextAlignmentCenter);
    text_layer_set_background_color(t_time, GColorClear);
    text_layer_set_font(t_time, mono);    
    
    triangle_path = gpath_create(&TRIANGLE_INFO);
    upside_triangle_path = gpath_create(&TRIANGLE_INFO);
    gpath_rotate_to(upside_triangle_path, TRIG_MAX_ANGLE / 2);
    triangle_layer = layer_create((GRect) {
        .origin = {bounds.origin.x, text_origin_y - TRIANGLE_HEIGHT + 3},
        .size = {bounds.size.w, bounds.size.h - (text_origin_y - TRIANGLE_HEIGHT + 3)}
    });
    layer_set_update_proc(triangle_layer, triangle_draw);
    
    t_once = text_layer_create((GRect) {
        .origin = {MARGIN, text_origin_y + FONT_SIZE + 20},
        .size = {bounds.size.w - MARGIN*2, 28}
    });
    text_layer_set_text_alignment(t_once, GTextAlignmentCenter);
    text_layer_set_background_color(t_once, GColorClear);
    text_layer_set_font(t_once, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    set_recurse_text();
    
    t_set_alarm = text_layer_create((GRect) {
        .origin = {10, STATUS_BAR_LAYER_HEIGHT},
        .size = {bounds.size.w - 20, 20}
    });
    text_layer_set_text_alignment(t_set_alarm, GTextAlignmentCenter);
    text_layer_set_background_color(t_set_alarm, GColorClear);
    text_layer_set_font(t_set_alarm, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text(t_set_alarm, "Set new phone alarm");
    
    
    layer_add_child(window_layer, triangle_layer);
    layer_add_child(window_layer, text_layer_get_layer(t_time));
    layer_add_child(window_layer, text_layer_get_layer(t_once));
    layer_add_child(window_layer, text_layer_get_layer(t_set_alarm));
    window_set_click_config_provider(s_window, config_provider);
}

static void window_unload(Window *window) {
  // Destroy output TextLayer
  text_layer_destroy(t_time);
    text_layer_destroy(t_once);
    text_layer_destroy(t_set_alarm);
    gpath_destroy(triangle_path);
    gpath_destroy(upside_triangle_path);
    layer_destroy(triangle_layer);
    status_bar_layer_destroy(status_bar);
}


void push_create_window() {
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