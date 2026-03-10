#ifndef S3_WIFI_DRIVERS_TIMER_TICK_H_
#define S3_WIFI_DRIVERS_TIMER_TICK_H_

#include <Arduino.h>

void timer_tick_init(volatile uint32_t* remaining_time, bool* is_running, bool* is_paused);

#endif
