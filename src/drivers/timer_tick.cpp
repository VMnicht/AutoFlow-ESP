#include "drivers/timer_tick.h"

#include <Arduino.h>

static hw_timer_t* s_timer = nullptr;
static volatile uint32_t* s_remaining_time = nullptr;
static bool* s_is_running = nullptr;
static bool* s_is_paused = nullptr;

void IRAM_ATTR on_timer_tick() {
  if (s_remaining_time == nullptr || s_is_running == nullptr || s_is_paused == nullptr) return;
  if (*s_is_running && !(*s_is_paused) && *s_remaining_time > 0) {
    (*s_remaining_time)--;
  }
}

void timer_tick_init(volatile uint32_t* remaining_time, bool* is_running, bool* is_paused) {
  s_remaining_time = remaining_time;
  s_is_running = is_running;
  s_is_paused = is_paused;

  s_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(s_timer, &on_timer_tick, true);
  timerAlarmWrite(s_timer, 1000000, true);
  timerAlarmEnable(s_timer);
}
