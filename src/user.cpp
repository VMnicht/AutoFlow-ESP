#include "user.h"
#include <Arduino.h> // 包含 Arduino 核心，以便使用 Serial

int fputc(int ch, FILE *stream)
{
    Serial.write(ch); // 将字符输出到 USB 串口
    return ch;
}