/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"
#include "uart.h"
#include "adc_x.h"
#include "INA219.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FACTOR_DIVISOR 4.0f // (R1+R2)/R2 // Para leer hasta 15V debe ser >= 4.55
#define FORMATO_TELEPLOT 1 
#define UMBRAL_ALARMA 10.0f // Umbral de alarma para el voltaje de la batería (en voltios)
#define HISTERESIS 0.3f // Histeresis para evitar oscilaciones en la alarma (en voltios)
#define LED_ON   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET)
#define LED_OFF  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t adc_value;
char buffer[50];
float voltaje; // Potenciometro en A0
float bateria; // Divisor de voltaje 0-15V (A2)
float voltaje_ina; // Voltaje del bus del 1NA219 (PB6 SCL Y PB7 SDA)
uint8_t ina_ok; // Flag de inicializacion del INA219
uint16_t adc1_codigo[ADC1_NUM_CHANNELS]; // Arreglo para almacenar los codigos de los canales del ADC
uint8_t alarma = 0; // Flag de alarma para el voltaje de la batería

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  uartx_write_text(&huart1, "Iniciando laboratorio de Sistemas Embebidos\r\n");

  ina_ok = INA219_Init();
  if (ina_ok == 0)
  {
    uartx_write_text(&huart1, "INA219  no inicializado correctamente\r\n");
  }
  else
  {
    uartx_write_text(&huart1, "INA219 inicializado correctamente\r\n");
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // -- Lectura ADC
    // rank 1 = canal O (PA0), rank 2 = canal 2 (PA2)
    ADC_Read_All(&hadc1, ADC1_NUM_CHANNELS, adc1_codigo);
    voltaje = (float)adc1_codigo[0] * 3.3 / 4095.0;      // potenciometro
    bateria = (float)adc1_codigo[1] * 3.3 / 4095.0;      // divisor
    bateria = bateria * FACTOR_DIVISOR; 
    if (!alarma && bateria > UMBRAL_ALARMA)
        alarma = 1;
    else if (alarma && bateria < (UMBRAL_ALARMA - HISTERESIS))
        alarma = 0;

    if (alarma) { LED_ON; } else { LED_OFF; }                 // Ajuste segun el divisor de voltaje utilizado
    // Lectura INA219 por I2C (La librería entrega en mV)
    voltaje_ina = INA219_ReadBusVoltage() / 1000.0f;  // Se convierte de mV a V
    // Corriente en mA (Opcional) -> INA219_ReadCurrent();
    
    // Envio por USART1 (9600 baud)
    #if FORMATO_TELEPLOT
    sprintf(buffer, ">Voltaje:%.2f\r\n", voltaje);
    uartx_write_text(&huart1, buffer);
    sprintf(buffer, ">Bateria:%.2f\r\n", bateria);
    uartx_write_text(&huart1, buffer);
    sprintf(buffer, ">INA219:%.2f\r\n", voltaje_ina);
    uartx_write_text(&huart1, buffer);
    sprintf(buffer, ">Alarma:%d\r\n", alarma);
    uartx_write_text(&huart1, buffer);
#else
    sprintf(buffer, "Bateria= %.2f V\r\n", bateria);
    uartx_write_text(&huart1, buffer);
    sprintf(buffer, "Voltaje= %.2f V\r\n", voltaje);
    uartx_write_text(&huart1, buffer);
    sprintf(buffer, "INA219= %.2f V\r\n", voltaje_ina);
    uartx_write_text(&huart1, buffer);
#endif
 
    HAL_Delay(500);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
