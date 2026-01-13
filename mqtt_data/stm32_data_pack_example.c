/**
 * STM32 数据打包使用示例
 * 该文件展示如何在STM32项目中使用数据打包库
 */

#include "stm32_data_pack.h"
#include "stm32f1xx_hal.h"  // 或根据你的STM32型号修改

// ==================== 全局串口句柄 ====================
extern UART_HandleTypeDef huart2;  // 假设使用UART1

// ==================== 用户实现的串口发送函数 ====================
/**
 * 发送单个字节到UART1
 * 这是 stm32_data_pack.h 中声明的必须实现的函数
 */
void UART_SendByte(uint8_t byte) {
  HAL_UART_Transmit(&huart2, &byte, 1, 100);
}

// ==================== 可选：发送整个数据包 ====================
/**
 * 发送缓冲区数据（如果要一次性发送整个帧）
 */
void UART_SendBuffer(uint8_t* buffer, uint8_t len) {
  HAL_UART_Transmit(&huart2, buffer, len, 100);
}

// ==================== 使用示例 ====================

/**
 * 示例1：简单发送数据
 * 
 * 使用 sendSensorData() 函数，它会自动打包并发送
 */
void Example1_SimpleSend(void) {
  // 发送土壤湿度=50%, 光照强度=1500 lux
  sendSensorData(50, 1500);
}

/**
 * 示例2：手动打包再发送
 * 
 * 如果需要更细粒度的控制，可以先打包再发送
 */
void Example2_ManualPack(void) {
  uint8_t buffer[FRAME_MIN_LEN];
  
  // 打包数据
  uint8_t frameLen = packSensorData(75, 2000, buffer, FRAME_MIN_LEN);
  
  // 发送打包好的数据
  if (frameLen > 0) {
    UART_SendBuffer(buffer, frameLen);
  }
}

/**
 * 示例3：定时发送传感器数据
 * 
 * 在定时中断或主循环中定期发送
 */
void sendSensorDataPeriodic(void) {
  static uint32_t lastSendTime = 0;
  uint32_t currentTime = HAL_GetTick();
  
  // 每2秒发送一次
  if (currentTime - lastSendTime >= 2000) {
    lastSendTime = currentTime;
    
    // 这里应该从实际传感器读取数据
    // uint16_t soilMoisture = readSoilMoisture();      // 你的传感器读取函数
    // uint16_t lightIntensity = readLightIntensity();  // 你的传感器读取函数
    
    // 发送数据
    // sendSensorData(soilMoisture, lightIntensity);
  }
}

/**
 * 示例4：带错误检查的发送
 */
void sendSensorDataWithCheck(uint16_t soilMoisture, uint16_t lightIntensity) {
  uint8_t buffer[FRAME_MIN_LEN];
  uint8_t frameLen = packSensorData(soilMoisture, lightIntensity, buffer, FRAME_MIN_LEN);
  
  if (frameLen == FRAME_MIN_LEN) {
    // 打包成功，发送数据
    for (uint8_t i = 0; i < frameLen; i++) {
      if (HAL_UART_Transmit(&huart2, &buffer[i], 1, 100) != HAL_OK) {
        // 发送失败处理
        printf("UART send failed\r\n");
        return;
      }
    }
    printf("Data sent successfully\r\n");
  } else {
    printf("Data pack failed\r\n");
  }
}

/**
 * 示例5：调试打印帧内容
 */
void debugFrameContent(void) {
  // 打印帧内容 (需配置 printf 重定向)
  debugPrintFrame(60, 1800);
  
  // 输出示例：
  // Frame Length: 8 bytes
  // Frame Data: AA 04 00 3C 07 08 6D 55 
  // Soil Moisture: 60%
  // Light Intensity: 1800 lux
}

// ==================== 在STM32 main.c 中的集成示例 ====================
/*
// 在 main.c 中的 main() 函数中：

int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  
  printf("STM32 Data Pack Initialized\r\n");
  
  while (1) {
    // 每2秒发送一次传感器数据
    sendSensorDataPeriodic();
    
    HAL_Delay(100);
  }
}
*/

// ==================== 传感器读取函数（需用户实现） ====================
/**
 * 从ADC读取土壤湿度
 * 需要根据你的硬件配置实现
 */
// uint16_t readSoilMoisture(void) {
//   // 示例：从ADC1读取，转换为百分比
//   // uint16_t adcValue = HAL_ADC_GetValue(&hadc1);
//   // return (adcValue * 100) / 4095;  // 12位ADC转换
//   return 50;  // 临时返回值
// }

// /**
//  * 从ADC读取光照强度
//  * 需要根据你的硬件配置实现
//  */
// uint16_t readLightIntensity(void) {
//   // 示例：从ADC2读取，转换为光照强度值
//   // uint16_t adcValue = HAL_ADC_GetValue(&hadc2);
//   // return (adcValue * 65535) / 4095;  // 12位ADC转换
//   return 1500;  // 临时返回值
// }
