#ifndef S3_WIFI_DRIVERS_BOARD_IO_H_
#define S3_WIFI_DRIVERS_BOARD_IO_H_

#include <Arduino.h>

void board_io_init();
void board_io_set_scan_ctrl(bool high_level);
void board_io_trigger_scan_pulse();
int board_io_find_sensor_error_channel(const uint32_t durations[4], const bool enable_sensors[4]);
void board_io_send_channel_state_packets(const bool ch_status[4]);

#endif
