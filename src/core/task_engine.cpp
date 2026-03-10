#include "core/task_engine.h"

#include <Arduino.h>
#include <string.h>

static InfusionTask s_task_mgr;
static PendingAction s_pending_action;

void task_engine_init() {
  memset(&s_task_mgr, 0, sizeof(s_task_mgr));
  s_pending_action.type = ACTION_NONE;
}

InfusionTask* task_engine_get_task() {
  return &s_task_mgr;
}

PendingAction* task_engine_get_pending_action() {
  return &s_pending_action;
}

void task_engine_stop_task_logic() {
  s_task_mgr.isRunning = false;
  s_task_mgr.isPaused = false;
  s_task_mgr.remainingTime = 0;
  s_task_mgr.currentStepIndex = -1;
}

void task_engine_reset_to_default_open() {
  task_engine_stop_task_logic();
  s_task_mgr.isDefaultOpen = true;
  Serial.println("Mode Switched: DEFAULT OPEN (All channels ON)");
}

void task_engine_emergency_stop() {
  s_task_mgr.isDefaultOpen = false;
  task_engine_stop_task_logic();
  s_pending_action.type = ACTION_NONE;
  Serial.println("!!! EMERGENCY STOP: ALL OFF !!!");
}

void task_engine_pause_task() {
  if (s_task_mgr.isRunning && !s_task_mgr.isPaused) {
    s_task_mgr.isPaused = true;
    Serial.println("Task PAUSED.");
  }
}

void task_engine_resume_task() {
  if (s_task_mgr.isRunning && s_task_mgr.isPaused) {
    s_task_mgr.isPaused = false;
    Serial.println("Task RESUMED.");
  }
}

void task_engine_schedule_action(PendingActionType type, unsigned long trigger_time) {
  s_pending_action.type = type;
  s_pending_action.triggerTime = trigger_time;
}

void task_engine_tick_1s() {
  if (s_task_mgr.isRunning && !s_task_mgr.isPaused && s_task_mgr.remainingTime > 0) {
    s_task_mgr.remainingTime--;
  }
}

void task_engine_execute_current_step() {
  int channel_id = s_task_mgr.executionOrder[s_task_mgr.currentStepIndex];
  uint32_t duration = s_task_mgr.durations[channel_id - 1];
  s_task_mgr.remainingTime = duration;
  Serial.printf("Task Step %d: Channel %d for %d sec\n", s_task_mgr.currentStepIndex, channel_id, duration);
}

bool task_engine_process_infusion_logic() {
  if (!s_task_mgr.isRunning) return false;
  if (s_task_mgr.isPaused) return false;
  if (s_task_mgr.remainingTime > 0) return false;

  s_task_mgr.currentStepIndex++;
  if (s_task_mgr.currentStepIndex >= 4) {
    Serial.println("Task sequence finished.");
    s_task_mgr.isRunning = false;
    s_task_mgr.currentStepIndex = -1;
    return true;
  }

  task_engine_execute_current_step();
  return false;
}

void task_engine_handle_pending_actions(unsigned long now_ms) {
  if (s_pending_action.type == ACTION_NONE) return;

  if (now_ms >= s_pending_action.triggerTime) {
    if (s_pending_action.type == ACTION_RESET) {
      task_engine_reset_to_default_open();
    } else if (s_pending_action.type == ACTION_START_TASK) {
      Serial.println("Delay finished. Starting Task...");
      s_task_mgr.currentStepIndex = 0;
      s_task_mgr.isRunning = true;
      s_task_mgr.isPaused = false;
      for (int i = 0; i < 4; i++) {
        if (s_task_mgr.executionOrder[i] == 0) s_task_mgr.executionOrder[i] = i + 1;
      }
      task_engine_execute_current_step();
    }
    s_pending_action.type = ACTION_NONE;
  }
}

int task_engine_add_duration_to_first_empty(uint32_t time_value) {
  for (int i = 0; i < 4; i++) {
    if (s_task_mgr.durations[i] == 0) {
      s_task_mgr.durations[i] = time_value;
      if (s_task_mgr.executionOrder[i] == 0) s_task_mgr.executionOrder[i] = i + 1;
      return i + 1;
    }
  }
  return 0;
}

void task_engine_set_task_config(const uint8_t execution_order[4], const uint32_t durations[4]) {
  for (int i = 0; i < 4; i++) {
    s_task_mgr.executionOrder[i] = execution_order[i];
    s_task_mgr.durations[i] = durations[i];
  }
}
