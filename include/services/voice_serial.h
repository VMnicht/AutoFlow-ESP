#ifndef S3_WIFI_SERVICES_VOICE_SERIAL_H_
#define S3_WIFI_SERVICES_VOICE_SERIAL_H_

#include <Arduino.h>

void voice_serial_init();
bool voice_serial_read_line(String& out_line);
void voice_serial_send_text(const String& text);
void voice_serial_send_gbk(const char* gbk_text);
void voice_serial_send_channel_packet(uint8_t channel_index, bool is_open);

#endif
