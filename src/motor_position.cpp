#include "motor_position.h"
#include <math.h>

// 初始化位置环 PID 参数
void PositionPID_Init(MotorPosition_t *motor, float kp, float ki, float kd, 
                      float integral_max, float output_max, float dt)
{
    motor->pid.kp = kp;
    motor->pid.ki = ki;
    motor->pid.kd = kd;
    motor->pid.integral_max = integral_max;
    motor->pid.output_max = output_max;
    motor->dt = dt;
    
    motor->integral = 0;
    motor->last_error = 0;
    motor->output_speed = 0;
    motor->target_position = 0;
    motor->current_position = 0;
    motor->last_position = 0;
    motor->current_velocity = 0;
}

// 更新当前位置（每次读取编码器后调用）
void Position_UpdateCurrent(MotorPosition_t *motor, int32_t current_pos)
{
    motor->last_position = motor->current_position;
    motor->current_position = current_pos;
    
    // 计算当前速度（脉冲/秒）
    if (motor->dt > 0) {
        motor->current_velocity = (float)(motor->current_position - motor->last_position) / motor->dt;
    }
}

// 设置目标位置
void Position_SetTarget(MotorPosition_t *motor, int32_t target)
{
    motor->target_position = target;
}

// 位置环 PID 计算
void PositionPID_Compute(MotorPosition_t *motor)
{
    float error, p_term, i_term, d_term, output;
    
    // 计算误差（目标 - 当前）
    error = (float)(motor->target_position - motor->current_position);
    
    // 比例项
    p_term = motor->pid.kp * error;
    
    // 积分项（带限幅）
    motor->integral += error * motor->dt;
    
    // 积分限幅
    if (motor->integral > motor->pid.integral_max)
        motor->integral = motor->pid.integral_max;
    else if (motor->integral < -motor->pid.integral_max)
        motor->integral = -motor->pid.integral_max;
    
    i_term = motor->pid.ki * motor->integral;
    
    // 微分项（使用误差变化率）
    d_term = motor->pid.kd * ((error - motor->last_error) / motor->dt);
    
    // PID 总和
    output = p_term + i_term + d_term;
    
    // 输出限幅（转换为 int16_t 速度值）
    if (output > motor->pid.output_max)
        output = motor->pid.output_max;
    else if (output < -motor->pid.output_max)
        output = -motor->pid.output_max;
    
    motor->output_speed = (int16_t)output;
    
    // 保存当前误差用于下一次微分计算
    motor->last_error = error;
}

// 检查是否到达目标位置（带死区）
uint8_t Position_IsAtTarget(MotorPosition_t *motor, int32_t deadband)
{
    int32_t diff = motor->target_position - motor->current_position;
    return (abs(diff) <= deadband);
}

// 4个电机整体位置控制（便捷函数）
void Motors_PositionControl(MotorPosition_t *motor1, MotorPosition_t *motor2,
                           MotorPosition_t *motor3, MotorPosition_t *motor4)
{
    // 计算每个电机的位置环输出
    PositionPID_Compute(motor1);
    PositionPID_Compute(motor2);
    PositionPID_Compute(motor3);
    PositionPID_Compute(motor4);
    
    // 发送速度指令给电机
    Motor_Set_Speeds(motor1->output_speed, 
                     motor2->output_speed,
                     motor3->output_speed, 
                     motor4->output_speed);
}