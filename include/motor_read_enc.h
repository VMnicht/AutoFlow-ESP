#ifndef MOTOR_READ_ENC_H
#define MOTOR_READ_ENC_H

#include <stdint.h>

// 声明全局变量（若需在其他文件中使用）
extern volatile int16_t modbus_date[8];
extern volatile uint8_t modbus_rx_frame_done;

void Modbus_ParseFrame(uint8_t data);

#endif