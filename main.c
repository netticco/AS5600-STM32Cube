/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Tacômetro Digital AS5600 - STM32F411 Blackpill
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "usbd_cdc_if.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
volatile uint32_t ultimoTempo = 0;
volatile uint32_t tempoAtual = 0;
volatile uint32_t deltaTempo = 0;
volatile uint32_t rpmAtual = 0;
/* USER CODE END PV */

/* Protótipos das funções */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
void Error_Handler(void);

/**
  * @brief  The application entry point.
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();

  /* Inicialização de todos os periféricos configurados */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_USB_DEVICE_Init();

  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

  // Liga o Enable do Driver (PA4)
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  /* USER CODE END 2 */

  char buffer_tx[64];

  while (1)
  {
    // Lógica de Timeout (Zerar se motor parar)
    uint32_t agora = __HAL_TIM_GET_COUNTER(&htim2);
    uint32_t tempoSemPulso;
    if (agora >= ultimoTempo) tempoSemPulso = agora - ultimoTempo;
    else tempoSemPulso = (0xFFFFFFFF - ultimoTempo) + agora;

    if (tempoSemPulso > 500000) rpmAtual = 0;

    // Envio USB
    int len = sprintf(buffer_tx, "%lu\r\n", rpmAtual);
    CDC_Transmit_FS((uint8_t*)buffer_tx, len);

    HAL_Delay(50);
  }
}

/* --- DEFINIÇÃO DAS FUNÇÕES DE INICIALIZAÇÃO --- */

static void MX_TIM1_Init(void)
{
  TIM_OC_InitTypeDef sConfigOC = {0};
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 2999; // 60MHz / 3000 = 20kHz
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  HAL_TIM_PWM_Init(&htim1);

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1);
  HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2);

  // Habilita as saídas principais (Necessário para TIM1)
  HAL_TIMEx_MasterConfigSynchronization(&htim1, (TIM_MasterConfigTypeDef*){0});
}

static void MX_TIM2_Init(void)
{
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 59; // 60MHz / 60 = 1MHz (1us tick)
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xFFFFFFFF; // 32-bit max
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  HAL_TIM_Base_Init(&htim2);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // Configuração PA4 como Output (Driver Enable)
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Configuração PB0 como EXTI Falling (Tacômetro)
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_0)
  {
    tempoAtual = __HAL_TIM_GET_COUNTER(&htim2);
    if (tempoAtual >= ultimoTempo) deltaTempo = tempoAtual - ultimoTempo;
    else deltaTempo = (0xFFFFFFFF - ultimoTempo) + tempoAtual;

    if (deltaTempo > 1000) // Gate para 4000 RPM
    {
      rpmAtual = (uint32_t)(60000000.0f / (float)deltaTempo);
      ultimoTempo = tempoAtual;
    }
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) Error_Handler();
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
