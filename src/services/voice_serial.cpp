#include "services/voice_serial.h"

#include <Arduino.h>

void voice_serial_init() {
  Serial1.begin(115200, SERIAL_8N1, 16, 17);
  Serial2.begin(9600, SERIAL_8N1, 38, 39);
}

bool voice_serial_read_line(String& out_line) {
  if (!Serial2.available()) return false;
  out_line = Serial2.readStringUntil('\n');
  out_line.trim();
  return out_line.length() > 0;
}

void voice_serial_send_text(const String& text) {
  Serial2.println(text);
}

void voice_serial_send_gbk(const char* gbk_text) {
  Serial2.println(gbk_text);
}

void voice_serial_send_channel_packet(uint8_t channel_index, bool is_open) {
  uint8_t packet[6] = {0xAA, 0x55, 0x02, channel_index, (uint8_t)(is_open ? 1 : 0), 0x0D};
  Serial1.write(packet, 6);
}
