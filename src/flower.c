#include "flower.h"
#include "ringbuffer.h"
#include "main.h"
#include "stdbool.h"
#include "tim.h"
#include "adc.h"
#include "driver_bh1750fvi_basic.h"
#include "cJSON.h"
#include "printf.h"
#include "usart.h"

// 水泵开关
#define PUMP_ON() do{ HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, GPIO_PIN_SET); } while(0)
#define PUMP_OFF() do{ HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, GPIO_PIN_RESET); } while(0)
// 光照强度设定 : 0 ~ 100
#define LIGHT_SET(light) do{ HAL_TIM_PWM_SetCompare(&htim1, TIM_CHANNEL_1, (light) * (100)); } while(0)

#define ADC_CHN (1)
#define ADC_DATA_NBYTE (2)
#define BUFFER_SIZE (16 * ADC_DATA_NBYTE)

static char mc_buf[BUFFER_SIZE]; // 湿度数据缓存区
static ring_buffer_t mc_rb; // 湿度ring buffer
static uint16_t adc_data[ADC_CHN];
static bool adc_ready = false;

void _putchar(char character)
{
    HAL_StatusTypeDef ret;
    ret =  HAL_UART_Transmit(&huart1, (uint8_t*)&character, 1, 0xFFFF);
    if (ret != HAL_OK) {
        printf("UART Transmit Error: %d\n", ret);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc == &hadc1) {
        ring_buffer_queue_arr(&mc_rb, (char*)&adc_data[0], ADC_DATA_NBYTE);
        adc_ready = true;
    }
}

static int get_mc_val(float* val)
{   
    if (!adc_ready) {
        return -1;
    }
    uint16_t raw_val = 0;
    ring_buffer_dequeue_arr(&mc_rb, (char*)&raw_val, ADC_DATA_NBYTE);
    // 0 ~ 4095
    // 0 ~ 100
    *val = (float)(raw_val * 100) / 4095;
    return 0;
}


int init(void)
{   
    // ring buffer init
    ring_buffer_init(&mc_rb, mc_buf, sizeof(mc_buf));

    // pwm init
    HAL_TIM_Base_Init(&htim1);
    HAL_TIM_PWM_Init(&htim1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    // light init
    bh1750fvi_basic_init(BH1750FVI_ADDRESS_LOW);
    
    // adc init
    HAL_ADC_Init(&hadc1);
    HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_data, ADC_CHN);

    return 0;
}

int loop(void)
{
    float mc_val = 0.0f;
    float la_val = 0.0f;
    get_mc_val(&mc_val);
    bh1750fvi_basic_read(&la_val);
    return 0;
}
