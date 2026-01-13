#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <RingBuf.h>
#include <Wire.h>
#include <stdint.h>

// ==================== 配置参数 ====================
#define SSID "xihua_wifi"           // WiFi SSID
#define PASSWORD "xihua_password"   // WiFi 密码
#define MQTT_SERVER "broker.emqx.io"    // EMQX 公共服务器
#define MQTT_PORT 1883
#define MQTT_TOPIC "xihua/mqtt/sensor"     // MQTT 发布主题
#define UART_BAUD 9600                  // 串口波特率

// ==================== RingBuffer 配置 ====================
#define RINGBUF_SIZE 256                // RingBuffer 大小
RingBuf<uint8_t, RINGBUF_SIZE> ringBuf;

// ==================== 数据包定义 ====================
#define FRAME_HEADER 0xAA               // 帧头
#define FRAME_TAIL 0x55                 // 帧尾
#define FRAME_MAX_LEN 64                // 最大数据包长度

// ==================== MQTT 相关 ====================
WiFiClient espClient;
PubSubClient client(espClient);

// ==================== 传感器数据结构 ====================
struct SensorData {
  uint16_t soilMoisture;  // 土壤湿度 (0-100%)
  uint16_t lightIntensity;// 光照强度 (0-65535, 单位自定义)
};

// ==================== 函数声明 ====================
void initWifi();
void initMqtt();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void handleSerialData();
bool parseDataFrame(uint8_t* buffer, uint8_t len, SensorData* data);
uint8_t calculateChecksum(uint8_t* data, uint8_t len);
void publishToMqtt(SensorData* data);
void reconnectMqtt();

// ==================== Setup 和 Loop ====================
void setup() {
  Serial.begin(UART_BAUD);
  delay(500);
  
  Serial.println("\n\nESP8266 MQTT Sensor Gateway Starting...");
  
  // 初始化WiFi
  initWifi();
  
  // 初始化MQTT
  initMqtt();
  
  Serial.println("Setup Complete!");
}

void loop() {
  // 处理串口接收数据
  handleSerialData();
  
  // 保持MQTT连接
  if (!client.connected()) {
    reconnectMqtt();
  }
  client.loop();
  
  delay(10);
}

// ==================== WiFi 初始化 ====================
void initWifi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Connection Failed!");
  }
}

// ==================== MQTT 初始化 ====================
void initMqtt() {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(mqttCallback);
}

// ==================== MQTT 重连 ====================
void reconnectMqtt() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  static unsigned long lastAttempt = 0;
  if (millis() - lastAttempt < 5000) {
    return;  // 5秒重试一次
  }
  lastAttempt = millis();
  
  Serial.print("Attempting MQTT connection...");
  
  // 使用随机客户端ID
  String clientId = "ESP8266-";
  clientId += String(random(0xffff), HEX);
  
  if (client.connect(clientId.c_str())) {
    Serial.println("Connected to MQTT Broker");
    // 可选：订阅控制主题
    // client.subscribe("esp8266/control");
  } else {
    Serial.print("failed, rc=");
    Serial.println(client.state());
  }
}

// ==================== MQTT 回调处理 ====================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// ==================== 串口数据处理 ====================
void handleSerialData() {
  // 从串口读取数据到RingBuffer
  while (Serial.available() > 0) {
    uint8_t byte = Serial.read();
    ringBuf.push(byte);
    
    // 调试信息
    Serial.print("RX: 0x");
    if (byte < 0x10) Serial.print("0");
    Serial.print(byte, HEX);
    Serial.print(" (Buffer: ");
    Serial.print(ringBuf.size());
    Serial.println(")");
  }
  
  // 解析完整的数据帧
  while (ringBuf.size() > 0) {
    // 查找帧头
    uint8_t firstByte;
    if (!ringBuf.peek(firstByte, 0)) {
      break;
    }
    
    if (firstByte != FRAME_HEADER) {
      uint8_t dummy;
      ringBuf.pop(dummy);
      Serial.println("Dropped invalid byte");
      continue;
    }
    
    // 检查是否有足够的数据 (最少: 帧头+长度+帧尾=3)
    if (ringBuf.size() < 3) {
      break;
    }
    
    // 获取长度字节
    uint8_t lenByte;
    ringBuf.peek(lenByte, 1);
    uint8_t frameLen = lenByte + 3;  // 帧头 + 数据长度 + 校验和 + 帧尾
    
    // 检查长度合法性
    if (frameLen > FRAME_MAX_LEN || lenByte == 0) {
      uint8_t dummy;
      ringBuf.pop(dummy);
      Serial.println("Invalid frame length");
      continue;
    }
    
    // 检查是否收到完整帧
    if (ringBuf.size() < frameLen) {
      break;
    }
    
    // 从RingBuffer提取完整帧
    uint8_t frame[FRAME_MAX_LEN];
    for (uint8_t i = 0; i < frameLen; i++) {
      ringBuf.peek(frame[i], i);
    }
    
    // 验证帧尾
    if (frame[frameLen - 1] != FRAME_TAIL) {
      uint8_t dummy;
      ringBuf.pop(dummy);
      Serial.println("Invalid frame tail");
      continue;
    }
    
    // 验证校验和
    uint8_t checksum = calculateChecksum(&frame[1], lenByte + 1);
    if (checksum != frame[frameLen - 2]) {
      uint8_t dummy;
      ringBuf.pop(dummy);
      Serial.println("Checksum error");
      continue;
    }
    
    // 解析数据
    SensorData data;
    if (parseDataFrame(frame, frameLen, &data)) {
      Serial.println("Frame parsed successfully!");
      publishToMqtt(&data);
    }
    
    // 移除已处理的帧
    for (uint8_t i = 0; i < frameLen; i++) {
      uint8_t dummy;
      ringBuf.pop(dummy);
    }
  }
}

// ==================== 数据帧解析 ====================
bool parseDataFrame(uint8_t* buffer, uint8_t len, SensorData* data) {
  if (len < 7) {  // 最小帧长度验证 (帧头+长度+4字节数据+校验和+帧尾=8)
    return false;
  }
  
  // 帧格式: [0xAA] [长度] [数据] [校验和] [0x55]
  // 数据长度必须为4字节 (土壤湿度2 + 光照强度2)
  uint8_t dataLen = buffer[1];
  if (dataLen != 4) {
    return false;
  }
  
  // 从缓冲区提取数据 (大端模式)
  data->soilMoisture = (buffer[2] << 8) | buffer[3];
  data->lightIntensity = (buffer[4] << 8) | buffer[5];
  
  Serial.print("Parsed Data - Soil Moisture: ");
  Serial.print(data->soilMoisture);
  Serial.print("%, Light: ");
  Serial.print(data->lightIntensity);
  Serial.println(" lux");
  
  return true;
}

// ==================== 校验和计算 ====================
uint8_t calculateChecksum(uint8_t* data, uint8_t len) {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return sum & 0xFF;
}

// ==================== 发布到MQTT ====================
void publishToMqtt(SensorData* data) {
  if (!client.connected()) {
    Serial.println("MQTT not connected, reconnecting...");
    reconnectMqtt();
  }
  
  // 创建JSON文档
  JsonDocument doc;
  doc["soilMoisture"] = data->soilMoisture;
  doc["lightIntensity"] = data->lightIntensity;
  doc["timestamp"] = millis();
  
  // 序列化JSON
  String jsonString;
  serializeJson(doc, jsonString);
  
  // 发布到MQTT
  if (client.publish(MQTT_TOPIC, jsonString.c_str())) {
    Serial.print("MQTT Published: ");
    Serial.println(jsonString);
  } else {
    Serial.println("MQTT Publish Failed!");
  }
}