#include <pebble.h>
#include "messenger.h"
#include "vibrate.h"

#define KEY_VIBE 0
#define KEY_SEND_NEXT_ALARM 1
#define KEY_NEW_ALARM 2
#define KEY_HOUR 21
#define KEY_MINUTE 22
#define KEY_REPEATING 23
#define KEY_NAME 182
#define KEY_TIME 196
#define KEY_CAN_DISMISS 116

Alarm next;
AlarmUpdated handler;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    // Store incoming information
    Tuple *name_tuple = dict_find(iterator, KEY_NAME);
    Tuple *time_tuple = dict_find(iterator, KEY_TIME);
    Tuple *dismiss_tuple = dict_find(iterator, KEY_CAN_DISMISS);
    Tuple *empty_tuple = dict_find(iterator, KEY_SEND_NEXT_ALARM);
    Tuple *vibe_tuple = dict_find(iterator, KEY_VIBE);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "message received from phone");
    if (time_tuple) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "message has time");
        next = (Alarm){
            .unix_mins = time_tuple->value->int32,
            .dismissable = dismiss_tuple->value->int8 == 1
        };
        APP_LOG(APP_LOG_LEVEL_DEBUG, "time %d", next.unix_mins);
        if(name_tuple) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "name %s", name_tuple->value->cstring);
            snprintf(next.name, 32, "%s", name_tuple->value->cstring);
        }
        if(handler != NULL)
            handler(true, &next);
    } else if(empty_tuple) {
        if(empty_tuple->value->int8 == 0) {
            next = (Alarm){.unix_mins=0};
            if(handler != NULL)
                handler(false, &next);
        }
    } else if(vibe_tuple) {
        if(vibe_tuple->value->int8 == 0) {
            //cancel
            stop_vibing();
        } else {
			tell_alive(NULL);
            vibe_it(NULL);
        }
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void predismiss() {
    if(next.unix_mins > 0) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Predismiss");
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        dict_write_int8(iter, KEY_CAN_DISMISS, 1);  
        dict_write_int32(iter, KEY_TIME, next.unix_mins);
        app_message_outbox_send();
    }
}

void set_alarm(int8_t hour, int8_t min, int8_t repeat) {
    APP_LOG(APP_LOG_LEVEL_INFO, "New alarm");
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_int8(iter, KEY_NEW_ALARM, 1);
    dict_write_int8(iter, KEY_HOUR, hour);
    dict_write_int8(iter, KEY_MINUTE, min);
    dict_write_int8(iter, KEY_REPEATING, repeat);
    dict_write_int32(iter, KEY_TIME, next.unix_mins);
    app_message_outbox_send();
}

void launch_messenger() {
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    app_message_register_inbox_received(inbox_received_callback);
    app_message_open(app_message_inbox_size_maximum(), APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
}

void destroy_messenger() {
    app_message_deregister_callbacks();
}

void update_alarm(AlarmUpdated update_handler) {
    handler = update_handler;
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    // Add a key-value pair
    dict_write_int8(iter, KEY_SEND_NEXT_ALARM, 1);            
    // Send the message!
    app_message_outbox_send();
}