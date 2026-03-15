#ifndef MOTOR_POSITION_H
#define MOTOR_POSITION_H

#include <stdint.h>
#include "motor_set_speed.h"  // 包含 PID_t 定义

// 位置环 PID 参数结构体
typedef struct {
    float kp;           // 比例系数
    float ki;           // 积分系数
    float kd;           // 微分系数
    float integral_max; // 积分限幅
    float output_max;   // 输出限幅（最大速度）
} PositionPID_t;

// 电机位置控制结构体（每个电机一个实例）
typedef struct {
    // 目标值
    int32_t target_position;      // 目标位置（编码器脉冲数）
    
    // 当前值
    int32_t current_position;      // 当前位置（编码器脉冲数）
    int32_t last_position;         // 上一次的位置（用于计算速度）
    float current_velocity;        // 当前速度（脉冲/控制周期）
    
    // PID 相关
    PositionPID_t pid;             // PID 参数
    float integral;                // 积分项
    float last_error;              // 上一次误差（用于微分项）
    
    // 输出
    int16_t output_speed;          // 输出的目标速度（给速度环）
    
    // 控制周期
    float dt;                      // 控制周期（秒）
} MotorPosition_t;

// 初始化位置环 PID 参数
void PositionPID_Init(MotorPosition_t *motor, float kp, float ki, float kd, 
                      float integral_max, float output_max, float dt);

// 位置环 PID 计算（单次调用）
void PositionPID_Compute(MotorPosition_t *motor);

// 设置目标位置
void Position_SetTarget(MotorPosition_t *motor, int32_t target);

// 更新当前位置（从编码器读取后调用）
void Position_UpdateCurrent(MotorPosition_t *motor, int32_t current_pos);

// 检查是否到达目标位置（带死区）
uint8_t Position_IsAtTarget(MotorPosition_t *motor, int32_t deadband);

// 4个电机整体位置控制（便捷函数）
void Motors_PositionControl(MotorPosition_t *motor1, MotorPosition_t *motor2,
                           MotorPosition_t *motor3, MotorPosition_t *motor4);

#endif