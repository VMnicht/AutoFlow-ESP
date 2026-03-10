#include "drivers/board_io.h"

#include <Arduino.h>

static const int PIN_SENSOR_1 = 20;
static const int PIN_SENSOR_2 = 21;
static const int PIN_SENSOR_3 = 47;
static const int PIN_SENSOR_4 = 48;
static const int PIN_SCAN_CTRL = 45;

void board_io_init() {
  pinMode(PIN_SENSOR_1, INPUT_PULLUP);
  pinMode(PIN_SENSOR_2, INPUT_PULLUP);
  pinMode(PIN_SENSOR_3, INPUT_PULLUP);
  pinMode(PIN_SENSOR_4, INPUT_PULLUP);
  pinMode(PIN_SCAN_CTRL, OUTPUT);
  digitalWrite(PIN_SCAN_CTRL, LOW);
}

void board_io_set_scan_ctrl(bool high_level) {
  digitalWrite(PIN_SCAN_CTRL, high_level ? HIGH : LOW);
}

void board_io_trigger_scan_pulse() {
  digitalWrite(PIN_SCAN_CTRL, HIGH);
  delay(200);
  digitalWrite(PIN_SCAN_CTRL, LOW);
}

int board_io_find_sensor_error_channel(const uint32_t durations[4], const bool enable_sensors[4]) {
  const int sensor_pins[4] = {PIN_SENSOR_1, PIN_SENSOR_2, PIN_SENSOR_3, PIN_SENSOR_4};
  for (int i = 0; i < 4; i++) {
    if (durations[i] > 0 && enable_sensors[i]) {
      if (digitalRead(sensor_pins[i]) == HIGH) return i + 1;
    }
  }
  return 0;
}

void board_io_send_channel_state_packets(const bool ch_status[4]) {
  for (int i = 0; i < 4; i++) {
    uint8_t packet[6] = {0xAA, 0x55, 0x02, (uint8_t)i, (uint8_t)(ch_status[i] ? 1 : 0), 0x0D};
    Serial1.write(packet, 6);
    delay(10);
  }
}
