#include "motor_read_enc.h"
#include "motor_crc.h"

volatile int16_t modbus_date[8];      // 存储累计编码器值
volatile uint8_t modbus_rx_frame_done; // 编码器读取完成标志位

#define MODBUS_MAX_REG 8

void Modbus_ParseFrame(uint8_t data)
{
    /* 接收状态机状态 */
    static uint8_t state = 0;
    static uint8_t modbus_reg_num;

    /* 已接收寄存器数量索引 */
    static uint8_t data_idx = 0;

    /* 临时寄存器，用于拼接高低字节 */
    static uint16_t reg_temp = 0;

    /* 接收到的 CRC 低 / 高字节 */
    static uint8_t crc_l = 0;
    static uint8_t crc_h = 0;

    /* 原始接收数据缓存，用于 CRC 校验 */
    static uint8_t modbus_raw_buf[32];

    /* 原始数据索引 */
    static uint8_t raw_idx = 0;

    /* Modbus 数据区字节数 */
    static uint8_t modbus_rx_byte_cnt = 0;

    /* 临时寄存器缓冲区 */
    static uint16_t modbus_reg_buf[MODBUS_MAX_REG];

    /* 保存原始数据用于 CRC 计算 */
    if (raw_idx < sizeof(modbus_raw_buf))
        modbus_raw_buf[raw_idx++] = data;
    else
        raw_idx = 0;

    switch (state)
    {
        case 0: // 接收从站地址
            if (data == 0x0A)
                state = 1;
            else
                raw_idx = 0;
            break;

        case 1: // 接收功能码
            if (data == 0x03)
                state = 2;
            else
            {
                state = 0;
                raw_idx = 0;
            }
            break;

        case 2: // 接收数据字节数
            modbus_rx_byte_cnt = data;

            if ((modbus_rx_byte_cnt & 0x01) ||
                (modbus_rx_byte_cnt / 2 > MODBUS_MAX_REG))
            {
                state = 0;
                raw_idx = 0;
                break;
            }

            modbus_reg_num = modbus_rx_byte_cnt / 2;
            data_idx = 0;
            state = 3;
            break;

        case 3: // 接收寄存器高字节
            reg_temp = ((uint16_t)data << 8);
            state = 4;
            break;

        case 4: // 接收寄存器低字节
            reg_temp |= data;

            if (data_idx < MODBUS_MAX_REG)
                modbus_reg_buf[data_idx++] = reg_temp;

            if ((data_idx * 2) >= modbus_rx_byte_cnt)
                state = 5;
            else
                state = 3;

            break;

        case 5: // CRC低字节
            crc_l = data;
            state = 6;
            break;

        case 6: // CRC高字节
        {
            uint16_t crc_calc;
            uint16_t crc_recv;

            crc_h = data;
            crc_recv = crc_l | ((uint16_t)crc_h << 8);

            crc_calc = CRC16(modbus_raw_buf, 3 + modbus_rx_byte_cnt);

            if (crc_calc == crc_recv)
            {
                modbus_rx_frame_done = 1;

                for (uint8_t i = 0; i < modbus_reg_num; i++)
                {
                    modbus_date[i] = modbus_reg_buf[i];
                }
            }

            state = 0;
            raw_idx = 0;
            break;
        }

        default:
            state = 0;
            raw_idx = 0;
            break;
    }
}