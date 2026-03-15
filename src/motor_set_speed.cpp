#include "motor_set_speed.h"
#include "motor_crc.h"
#include <Arduino.h> // 用于 Serial 对象

// 定义电机控制使用的串口（可修改引脚和波特率）
#define MOTOR_SERIAL Serial1
// 注意：需要在 setup() 中初始化 MOTOR_SERIAL.begin(baud, SERIAL_8N1, rxPin, txPin)

void Motor_Set_ClosedLoop(void)
{
    uint8_t idx = 0;
    uint8_t frame[20];

    frame[idx++] = 0x0A; // 从站地址
    frame[idx++] = 0x06; // 功能码：写单个保持寄存器

    frame[idx++] = 0x00; // 起始寄存器高字节
    frame[idx++] = 0x08; // 起始寄存器低字节

    // 寄存器值：0x0001
    frame[idx++] = 0x00;
    frame[idx++] = 0x01;

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;

   MOTOR_SERIAL.write(frame,idx);
    // for (uint8_t i = 0; i < idx; i++)
    //     MOTOR_SERIAL.write(frame[i]);
    // 可选：等待发送完成
    // 
}

void Motor_Set_Speeds(int16_t v0, int16_t v1, int16_t v2, int16_t v3)
{
    uint8_t idx = 0;
    uint8_t frame[20];

    frame[idx++] = 0x0A; // 从站地址
    frame[idx++] = 0x10; // 功能码：写多个保持寄存器

    frame[idx++] = 0x00; // 起始寄存器高字节
    frame[idx++] = 0x00; // 起始寄存器低字节

    frame[idx++] = 0x00; // 寄存器数量高字节
    frame[idx++] = 0x04; // 寄存器数量低字节（4 个）

    frame[idx++] = 0x08; // 数据字节数 = 4 × 2 = 8

    frame[idx++] = (v0 >> 8) & 0xFF;
    frame[idx++] = (v0 >> 0) & 0xFF;

    frame[idx++] = (v1 >> 8) & 0xFF;
    frame[idx++] = (v1 >> 0) & 0xFF;

    frame[idx++] = (v2 >> 8) & 0xFF;
    frame[idx++] = (v2 >> 0) & 0xFF;

    frame[idx++] = (v3 >> 8) & 0xFF;
    frame[idx++] = (v3 >> 0) & 0xFF;

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;

    // 发送
    MOTOR_SERIAL.write(frame,idx);
    // for (uint8_t i = 0; i < idx; i++)
    //     MOTOR_SERIAL.write(frame[i]);
}

void Motor_Set_Enc1_A(void)
{
    uint8_t idx = 0;
    uint8_t frame[20];

    frame[idx++] = 0x0A;
    frame[idx++] = 0x06;
    frame[idx++] = 0x00;
    frame[idx++] = 0x09; // 寄存器地址 0x09
    frame[idx++] = 0x00;
    frame[idx++] = 0x01;

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;

    MOTOR_SERIAL.write(frame,idx);
    // for (uint8_t i = 0; i < idx; i++)
    //     MOTOR_SERIAL.write(frame[i]);
}

void Motor_Set_Enc1_B(void)
{
    uint8_t idx = 0;
    uint8_t frame[20];

    frame[idx++] = 0x0A;
    frame[idx++] = 0x06;
    frame[idx++] = 0x00;
    frame[idx++] = 0x0A;
    frame[idx++] = 0x00;
    frame[idx++] = 0x01;

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;
    MOTOR_SERIAL.write(frame,idx);
    // for (uint8_t i = 0; i < idx; i++)
    //     MOTOR_SERIAL.write(frame[i]);
        
}

void Motor_Set_Enc1_C(void)
{
    uint8_t idx = 0;
    uint8_t frame[20];

    frame[idx++] = 0x0A;
    frame[idx++] = 0x06;
    frame[idx++] = 0x00;
    frame[idx++] = 0x0B;
    frame[idx++] = 0x00;
    frame[idx++] = 0x01;

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;

    for (uint8_t i = 0; i < idx; i++)
        MOTOR_SERIAL.write(frame[i]);
}

void Motor_Set_Enc1_D(void)
{
    uint8_t idx = 0;
    uint8_t frame[20];

    frame[idx++] = 0x0A;
    frame[idx++] = 0x06;
    frame[idx++] = 0x00;
    frame[idx++] = 0x0C;
    frame[idx++] = 0x00;
    frame[idx++] = 0x01;

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;

    for (uint8_t i = 0; i < idx; i++)
        MOTOR_SERIAL.write(frame[i]);
}

void Motor_Set_KP_KI_KD(PID_t *Motor1, PID_t *Motor2, PID_t *Motor3, PID_t *Motor4)
{
    uint16_t Kp_Temp;
    uint16_t Ki_Temp;
    uint16_t Kd_Temp;

    uint8_t idx = 0;
    uint8_t frame[34];

    frame[idx++] = 0x0A; // 从站地址
    frame[idx++] = 0x10; // 功能码：写多个保持寄存器

    frame[idx++] = 0x00; // 起始寄存器高字节
    frame[idx++] = 0x15; // 起始寄存器低字节 (21)

    frame[idx++] = 0x00; // 寄存器数量高字节
    frame[idx++] = 0x0C; // 寄存器数量低字节（12 个）

    frame[idx++] = 0x18; // 数据字节数 = 12 × 2 = 24

    // Motor1
    Kp_Temp = (uint16_t)(Motor1->kp * 1000);
    Ki_Temp = (uint16_t)(Motor1->ki * 1000);
    Kd_Temp = (uint16_t)(Motor1->kd * 1000);
    frame[idx++] = (Kp_Temp >> 8) & 0xFF;
    frame[idx++] = (Kp_Temp >> 0) & 0xFF;
    frame[idx++] = (Ki_Temp >> 8) & 0xFF;
    frame[idx++] = (Ki_Temp >> 0) & 0xFF;
    frame[idx++] = (Kd_Temp >> 8) & 0xFF;
    frame[idx++] = (Kd_Temp >> 0) & 0xFF;

    // Motor2
    Kp_Temp = (uint16_t)(Motor2->kp * 1000);
    Ki_Temp = (uint16_t)(Motor2->ki * 1000);
    Kd_Temp = (uint16_t)(Motor2->kd * 1000);
    frame[idx++] = (Kp_Temp >> 8) & 0xFF;
    frame[idx++] = (Kp_Temp >> 0) & 0xFF;
    frame[idx++] = (Ki_Temp >> 8) & 0xFF;
    frame[idx++] = (Ki_Temp >> 0) & 0xFF;
    frame[idx++] = (Kd_Temp >> 8) & 0xFF;
    frame[idx++] = (Kd_Temp >> 0) & 0xFF;

    // Motor3
    Kp_Temp = (uint16_t)(Motor3->kp * 1000);
    Ki_Temp = (uint16_t)(Motor3->ki * 1000);
    Kd_Temp = (uint16_t)(Motor3->kd * 1000);
    frame[idx++] = (Kp_Temp >> 8) & 0xFF;
    frame[idx++] = (Kp_Temp >> 0) & 0xFF;
    frame[idx++] = (Ki_Temp >> 8) & 0xFF;
    frame[idx++] = (Ki_Temp >> 0) & 0xFF;
    frame[idx++] = (Kd_Temp >> 8) & 0xFF;
    frame[idx++] = (Kd_Temp >> 0) & 0xFF;

    // Motor4
    Kp_Temp = (uint16_t)(Motor4->kp * 1000);
    Ki_Temp = (uint16_t)(Motor4->ki * 1000);
    Kd_Temp = (uint16_t)(Motor4->kd * 1000);
    frame[idx++] = (Kp_Temp >> 8) & 0xFF;
    frame[idx++] = (Kp_Temp >> 0) & 0xFF;
    frame[idx++] = (Ki_Temp >> 8) & 0xFF;
    frame[idx++] = (Ki_Temp >> 0) & 0xFF;
    frame[idx++] = (Kd_Temp >> 8) & 0xFF;
    frame[idx++] = (Kd_Temp >> 0) & 0xFF;

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;

    MOTOR_SERIAL.write(frame,idx);
    // for (uint8_t i = 0; i < idx; i++)
    //     MOTOR_SERIAL.write(frame[i]);
}