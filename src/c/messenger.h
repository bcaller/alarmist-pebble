#pragma once

typedef struct Alarm {
    bool dismissable;
    int unix_mins;
    char name[32];
} Alarm;

typedef void(* AlarmUpdated)(bool exists, Alarm *next_alarm);

void launch_messenger();

void predismiss();

void set_alarm(int8_t hour, int8_t min, int8_t repeat);

void destroy_messenger();

void update_alarm(AlarmUpdated handler);