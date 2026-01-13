#ifndef STM32_DATA_PACK_H
#define STM32_DATA_PACK_H

#include <stdint.h>
#include <stdio.h>

// ==================== 帧格式定义 ====================
#define FRAME_HEADER        0xAA        // 帧头
#define FRAME_TAIL          0x55        // 帧尾
#define FRAME_MIN_LEN       8           // 最小帧长度 (1+1+4+1+1)
#define FRAME_DATA_LEN      4           // 数据段长度 (2+2)

// ==================== 用户需要实现的函数声明 ====================
/**
 * 发送单个字节到串口
 * 用户需要根据自己的硬件配置实现此函数
 * 
 * @param byte 要发送的字节
 */
extern void UART_SendByte(uint8_t byte);

// ==================== 对外接口函数 ====================

/**
 * 计算校验和
 * @param data 数据指针
 * @param len 数据长度
 * @return 校验和(8位)
 */
uint8_t calculateChecksum(uint8_t* data, uint8_t len);

/**
 * 打包传感器数据成串口帧格式
 * 帧格式: [0xAA] [长度] [土壤湿度] [光照强度] [校验和] [0x55]
 *
 * 具体字节：
 *  [0]     帧头 0xAA
 *  [1]     数据长度 0x04
 *  [2-3]   土壤湿度(大端 uint16_t)
 *  [4-5]   光照强度(大端 uint16_t)
 *  [6]     校验和 (字节1-5的和 & 0xFF)
 *  [7]     帧尾 0x55
 * 
 * @param soilMoisture 土壤湿度 (0-100%)
 * @param lightIntensity 光照强度 (0-65535)
 * @param buffer 输出缓冲区
 * @param bufferSize 缓冲区大小
 * @return 打包后的数据长度(8字节)，0表示失败
 */
uint8_t packSensorData(uint16_t soilMoisture, uint16_t lightIntensity,
                       uint8_t* buffer, uint8_t bufferSize);

/**
 * 通过串口发送打包好的数据
 * 
 * @param soilMoisture 土壤湿度 (0-100%)
 * @param lightIntensity 光照强度 (0-65535)
 * @return 1表示成功，0表示失败
 */
uint8_t sendSensorData(uint16_t soilMoisture, uint16_t lightIntensity);

/**
 * 调试函数：打印打包后的数据
 * (需要配置 printf 重定向到串口)
 * 
 * @param soilMoisture 土壤湿度
 * @param lightIntensity 光照强度
 */
void debugPrintFrame(uint16_t soilMoisture, uint16_t lightIntensity);

#endif  // STM32_DATA_PACK_H
