#ifndef S3_WIFI_CORE_TASK_ENGINE_H_
#define S3_WIFI_CORE_TASK_ENGINE_H_

#include <Arduino.h>

enum PendingActionType {
  ACTION_NONE,
  ACTION_RESET,
  ACTION_START_TASK
};

struct PendingAction {
  PendingActionType type;
  unsigned long triggerTime;
};

struct InfusionTask {
  bool isRunning;
  bool isPaused;
  bool isDefaultOpen;
  uint8_t executionOrder[4];
  uint32_t durations[4];
  int currentStepIndex;
  volatile uint32_t remainingTime;
};

void task_engine_init();
InfusionTask* task_engine_get_task();
PendingAction* task_engine_get_pending_action();
void task_engine_stop_task_logic();
void task_engine_reset_to_default_open();
void task_engine_emergency_stop();
void task_engine_pause_task();
void task_engine_resume_task();
void task_engine_schedule_action(PendingActionType type, unsigned long trigger_time);
void task_engine_tick_1s();
void task_engine_execute_current_step();
bool task_engine_process_infusion_logic();
void task_engine_handle_pending_actions(unsigned long now_ms);
int task_engine_add_duration_to_first_empty(uint32_t time_value);
void task_engine_set_task_config(const uint8_t execution_order[4], const uint32_t durations[4]);

#endif
