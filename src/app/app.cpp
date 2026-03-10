#include "app/app.h"

#include "core/command_parser.h"
#include "core/task_engine.h"
#include "drivers/board_io.h"
#include "drivers/timer_tick.h"
#include "services/mqtt_client.h"
#include "services/voice_serial.h"

#include <ArduinoJson.h>
#include <Arduino.h>

static const unsigned long DELAY_BEFORE_TASK_START = 15000;
static const unsigned long DELAY_BEFORE_TASK_START2 = 5000;
static const unsigned long DELAY_BEFORE_RESET = 1000;
static const bool ENABLE_SENSORS[4] = {false, true, true, false};

static const char* GBK_EMERGENCY_STOP = "\xBD\xF4\xBC\xB1\xCD\xA3\xD6\xB9";
static const char* GBK_ENTER_INIT = "\xBD\xF8\xC8\xEB\xB3\xF5\xCA\xBC\xD7\xB4\xCC\xAC";
static const char* GBK_RECV_TASK = "\xCA\xD5\xB5\xBD\xC8\xCE\xCE\xF1";
static const char* GBK_DI = "\xB5\xDA";
static const char* GBK_BU = "\xB2\xBD";
static const char* GBK_CHANNEL = "\xCD\xA8\xB5\xC0";
static const char* GBK_DURATION = "\xCA\xB1\xB3\xA4";
static const char* GBK_SECOND = "\xC3\xEB";
static const char* GBK_COMMA = "\xA3\xAC";
static const char* GBK_PERIOD = "\xA3\xAE";
static const char* GBK_DRUG_ID = "\xD2\xA9\xC6\xB7ID";
static const char* GBK_NURSE_ID = "\xBB\xA4\xCA\xBFID";
static const char* GBK_TIME = "\xCA\xB1\xBC\xE4";
static const char* GBK_MINUTE = "\xB7\xD6";
static const char* GBK_COLON = "\xA3\xBA";
static const char* GBK_PAUSE = "\xD4\xDD\xCD\xA3";
static const char* GBK_RESUME = "\xBC\xCC\xD0\xF8\xCA\xE4\xD2\xBA";
static const char* GBK_ERROR = "\xD2\xEC\xB3\xA3";
static const char* GBK_SETTING = "\xC9\xE8\xD6\xC3";

static String format_time_voice(uint32_t total_seconds) {
  int mins = total_seconds / 60;
  int secs = total_seconds % 60;
  String res = "";
  if (mins > 0) res += String(mins) + String(GBK_MINUTE);
  res += String(secs) + String(GBK_SECOND);
  return res;
}

static void apply_mqtt_command(const ParsedCommand& cmd) {
  InfusionTask* task = task_engine_get_task();
  if (cmd.type == PARSED_MQTT_ID_0) {
    voice_serial_send_gbk(GBK_EMERGENCY_STOP);
    task_engine_emergency_stop();
    return;
  }

  if (cmd.type == PARSED_MQTT_ID_2) {
    voice_serial_send_gbk(GBK_ENTER_INIT);
    task_engine_schedule_action(ACTION_RESET, millis() + DELAY_BEFORE_RESET);
    return;
  }

  if (cmd.type == PARSED_MQTT_ID_1) {
    Serial.println("New Task Received.");
    task->isDefaultOpen = false;
    task_engine_stop_task_logic();
    task_engine_schedule_action(ACTION_NONE, 0);
    task_engine_set_task_config(cmd.channels, cmd.times);

    String voice_msg = String(GBK_RECV_TASK) + String(GBK_PERIOD);
    for (int i = 0; i < 4; i++) {
      voice_msg += String(GBK_DI) + String(i + 1) + String(GBK_BU);
      voice_msg += String(GBK_COMMA);
      voice_msg += String(GBK_CHANNEL) + String((int)cmd.channels[i]);
      voice_msg += String(GBK_COMMA);
      voice_msg += String(GBK_DURATION) + String((int)cmd.times[i]) + String(GBK_SECOND);
      voice_msg += String(GBK_PERIOD);
    }
    voice_serial_send_text(voice_msg);
    task_engine_schedule_action(ACTION_START_TASK, millis() + DELAY_BEFORE_TASK_START);
    return;
  }

  if (cmd.type == PARSED_MQTT_ID_3) {
    task_engine_pause_task();
    voice_serial_send_gbk(GBK_PAUSE);
    return;
  }

  if (cmd.type == PARSED_MQTT_ID_4) {
    task_engine_resume_task();
    voice_serial_send_gbk(GBK_RESUME);
  }
}

static void on_mqtt_command(const String& message) {
  ParsedCommand cmd;
  if (!command_parser_parse_mqtt(message, cmd)) {
    Serial.println("JSON Parse Error");
    return;
  }
  apply_mqtt_command(cmd);
}

static void handle_serial_input() {
  String input;
  if (!voice_serial_read_line(input)) return;

  if (input.startsWith("command:add")) {
    board_io_trigger_scan_pulse();
  }

  ParsedCommand cmd;
  if (!command_parser_parse_serial(input, cmd)) return;

  InfusionTask* task = task_engine_get_task();
  if (cmd.type == PARSED_SERIAL_ADD) {
    int target_channel = task_engine_add_duration_to_first_empty(cmd.timeValue);
    if (target_channel > 0) {
      Serial.printf("Added to Channel %d: Time=%d, ID=%s\n", target_channel, cmd.timeValue, cmd.textId.c_str());
      String voice_msg = String(GBK_SETTING) + String(GBK_CHANNEL) + String(target_channel) + String(GBK_COMMA);
      voice_msg += String(GBK_DRUG_ID) + String(GBK_COLON) + cmd.textId + String(GBK_COMMA);
      voice_msg += String(GBK_TIME) + format_time_voice(cmd.timeValue);
      voice_serial_send_text(voice_msg);
    } else {
      Serial.println("Full! No empty channels available.");
    }
    return;
  }

  if (cmd.type == PARSED_SERIAL_START) {
    Serial.println("Start Command Received via Serial. Waiting delay...");
    task->isDefaultOpen = false;
    task_engine_stop_task_logic();
    task_engine_schedule_action(ACTION_START_TASK, millis() + DELAY_BEFORE_TASK_START2);
    String voice_msg = String(GBK_NURSE_ID) + String(GBK_COLON) + cmd.textId;
    voice_serial_send_text(voice_msg);
  }
}

static void check_sensors() {
  InfusionTask* task = task_engine_get_task();
  if (task->isPaused || !task->isRunning) return;

  int error_channel = board_io_find_sensor_error_channel(task->durations, ENABLE_SENSORS);
  if (error_channel > 0) {
    task_engine_pause_task();
    String msg = String(GBK_CHANNEL) + String(error_channel) + String(GBK_ERROR);
    voice_serial_send_text(msg);
    Serial.printf("Sensor Error on Ch %d\n", error_channel);
  }
}

static void sync_status() {
  static unsigned long last_update = 0;
  if (millis() - last_update < 500) return;
  last_update = millis();

  InfusionTask* task = task_engine_get_task();
  bool ch_status[4] = {false, false, false, false};
  int current_active_ch = (task->isRunning && task->currentStepIndex >= 0) ? task->executionOrder[task->currentStepIndex] : 0;

  if (!task->isPaused) {
    for (int i = 0; i < 4; i++) {
      if (task->isDefaultOpen || (task->isRunning && current_active_ch == (i + 1))) {
        ch_status[i] = true;
      }
    }
  }

  JsonDocument status_doc;
  status_doc["id"] = "1";
  JsonObject bottles = status_doc.createNestedObject("bolltes");
  bottles["status1"] = ch_status[0] ? "open" : "close";
  bottles["status2"] = ch_status[1] ? "open" : "close";
  bottles["status3"] = ch_status[2] ? "open" : "close";
  bottles["status4"] = ch_status[3] ? "open" : "close";
  if (task->isPaused) {
    bottles["statusNow"] = "PAUSE";
  } else {
    bottles["statusNow"] = task->isDefaultOpen ? "ALL" : String(current_active_ch);
  }
  bottles["timeLeft"] = task->remainingTime;

  String output;
  serializeJson(status_doc, output);
  mqtt_client_publish_data(output);
  board_io_send_channel_state_packets(ch_status);
}

void app_init() {
  Serial.begin(115200);
  voice_serial_init();
  board_io_init();
  task_engine_init();
  task_engine_reset_to_default_open();

  InfusionTask* task = task_engine_get_task();
  timer_tick_init(&task->remainingTime, &task->isRunning, &task->isPaused);

  mqtt_client_init("FakeGDUT", "gdut_404", "8.138.244.66", 1883, on_mqtt_command);
}

void app_loop() {
  mqtt_client_ensure_connected();
  mqtt_client_loop();

  task_engine_handle_pending_actions(millis());
  handle_serial_input();
  check_sensors();

  bool task_finished = task_engine_process_infusion_logic();
  if (task_finished) {
    board_io_set_scan_ctrl(false);
    delay(100);
    board_io_trigger_scan_pulse();
  }

  InfusionTask* task = task_engine_get_task();
  if (task->isRunning && !task->isPaused) {
    board_io_set_scan_ctrl(true);
  } else {
    board_io_set_scan_ctrl(false);
  }

  sync_status();
  delay(10);
}
