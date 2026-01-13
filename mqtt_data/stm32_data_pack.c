#include "stm32_data_pack.h"
#include <string.h>

/**
 * 计算校验和
 * @param data 数据指针
 * @param len 数据长度
 * @return 校验和(8位)
 */
uint8_t calculateChecksum(uint8_t* data, uint8_t len) {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return sum & 0xFF;
}

/**
 * 打包传感器数据成串口帧格式
 * 帧格式: [0xAA] [长度] [数据] [校验和] [0x55]
 * 
 * @param soilMoisture 土壤湿度 (0-100%)
 * @param lightIntensity 光照强度 (0-65535)
 * @param buffer 输出缓冲区
 * @param bufferSize 缓冲区大小
 * @return 打包后的数据长度，0表示失败
 */
uint8_t packSensorData(uint16_t soilMoisture, uint16_t lightIntensity, 
                       uint8_t* buffer, uint8_t bufferSize) {
  // 检查缓冲区大小
  if (bufferSize < FRAME_MIN_LEN) {
    return 0;  // 缓冲区过小
  }
  
  uint8_t frameLen = 0;
  
  // [0] 帧头
  buffer[frameLen++] = FRAME_HEADER;  // 0xAA
  
  // [1] 数据长度 (始终为4字节: 土壤湿度2 + 光照强度2)
  uint8_t dataLen = 4;
  buffer[frameLen++] = dataLen;
  
  // [2-3] 土壤湿度 (大端模式)
  buffer[frameLen++] = (uint8_t)(soilMoisture >> 8);
  buffer[frameLen++] = (uint8_t)(soilMoisture & 0xFF);
  
  // [4-5] 光照强度 (大端模式)
  buffer[frameLen++] = (uint8_t)(lightIntensity >> 8);
  buffer[frameLen++] = (uint8_t)(lightIntensity & 0xFF);
  
  // [6] 校验和 (长度+数据的和)
  uint8_t checksum = calculateChecksum(&buffer[1], dataLen + 1);
  buffer[frameLen++] = checksum;
  
  // [7] 帧尾
  buffer[frameLen++] = FRAME_TAIL;  // 0x55
  
  return frameLen;  // 返回总长度 (8字节)
}

/**
 * 通过串口发送打包好的数据
 * 
 * @param soilMoisture 土壤湿度
 * @param lightIntensity 光照强度
 * @return 1表示成功，0表示失败
 */
uint8_t sendSensorData(uint16_t soilMoisture, uint16_t lightIntensity) {
  uint8_t buffer[FRAME_MIN_LEN];
  uint8_t frameLen = packSensorData(soilMoisture, lightIntensity, buffer, FRAME_MIN_LEN);
  
  if (frameLen == 0) {
    return 0;
  }
  
  // 通过串口发送数据
  for (uint8_t i = 0; i < frameLen; i++) {
    // 需要用户实现 UART_SendByte() 函数
    UART_SendByte(buffer[i]);
  }
  
  return 1;
}

/**
 * 调试函数：打印打包后的数据(需要配置 printf 重定向)
 * 
 * @param soilMoisture 土壤湿度
 * @param lightIntensity 光照强度
 */
void debugPrintFrame(uint16_t soilMoisture, uint16_t lightIntensity) {
  uint8_t buffer[FRAME_MIN_LEN];
  uint8_t frameLen = packSensorData(soilMoisture, lightIntensity, buffer, FRAME_MIN_LEN);
  
  printf("Frame Length: %d bytes\r\n", frameLen);
  printf("Frame Data: ");
  for (uint8_t i = 0; i < frameLen; i++) {
    printf("%02X ", buffer[i]);
  }
  printf("\r\n");
  printf("Soil Moisture: %d%%\r\n", soilMoisture);
  printf("Light Intensity: %d lux\r\n", lightIntensity);
}
