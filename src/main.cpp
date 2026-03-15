#include <Arduino.h>
#include "motor_set_speed.h"
#include "motor_read_enc.h"
#include "motor_position.h"
// 电机串口引脚定义 (使用 Serial1)
#define MOTOR_RX_PIN 18
#define MOTOR_TX_PIN 17
#define MOTOR_BAUD   115200

// 控制周期（秒）
#define POSITION_LOOP_DT 0.02  // 50Hz

// 电机位置环实例
MotorPosition_t motor_pos[4];

// 定时器句柄（用于精确定时）
hw_timer_t *timer = NULL;
volatile uint8_t position_loop_flag = 0;

// 定时器中断服务程序
void IRAM_ATTR onTimer() {
 // printf("Timer interrupt triggered\n");
    position_loop_flag = 1;
}

void setup() {
    // 初始化调试串口 (USB)
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println("ESP32-S3 Motor Driver Test");

    // 初始化电机控制串口
    Serial1.begin(MOTOR_BAUD, SERIAL_8N1, MOTOR_RX_PIN, MOTOR_TX_PIN);

    // 可选：设置电机进入闭环模式
    Motor_Set_ClosedLoop();
    delay(50);

     // 初始化 PID 参数（4个电机）
    // 参数说明：kp, ki, kd, integral_max, output_max (最大速度), dt
    PositionPID_Init(&motor_pos[0], 2.0, 0.1, 0.5, 200, 500, POSITION_LOOP_DT);
    PositionPID_Init(&motor_pos[1], 2.0, 0.1, 0.5, 200, 500, POSITION_LOOP_DT);
    PositionPID_Init(&motor_pos[2], 2.0, 0.1, 0.5, 200, 500, POSITION_LOOP_DT);
    PositionPID_Init(&motor_pos[3], 2.0, 0.1, 0.5, 200, 500, POSITION_LOOP_DT);
    
    // 设置电机速度 (示例)
   
    // 设置目标位置
    // Position_SetTarget(&motor_pos[0], 5000);
    // Position_SetTarget(&motor_pos[1], -3000);
    // Position_SetTarget(&motor_pos[2], 2000);
    // Position_SetTarget(&motor_pos[3], 1000);

      // 设置定时器（50Hz）
    timer = timerBegin(0, 80, true); // 80分频 → 1MHz计数
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 2000000, true); // 20000计数 = 0.02秒
    timerAlarmEnable(timer);
   
     
}

void loop() {
    // 处理从电机返回的 Modbus 数据
    while (Serial1.available()) {
        uint8_t byte = Serial1.read();
        Modbus_ParseFrame(byte);
    }

    // 如果接收到一帧完整数据，打印编码器值
    if (modbus_rx_frame_done) {
        modbus_rx_frame_done = 0; // 清除标志
        Serial.print("Encoder values: ");
        for (int i = 0; i < 8; i++) {
            Serial.print(modbus_date[i]);
            Serial.print(" ");
        }
        Serial.println();
    }
     // 设置 PID 参数 (示例值)
    PID_t pid1 = {40, 4, 0.05};
    PID_t pid2 = {40, 4, 0.04};
    PID_t pid3 = {40, 4, 0.06};
    PID_t pid4 = {40, 4, 0.05};
    Motor_Set_KP_KI_KD(&pid1, &pid2, &pid3, &pid4);
    delay(10);
    Motor_Set_Speeds(10, 20, -10, -10);
    
    // 其他任务...
    delay(10);
}