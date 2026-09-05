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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tm1637.h"
#include "ds1302.h"
#include "hx1838.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart3;

osThreadId LED_TaskHandle;
osThreadId LED2_TaskHandle;
/* USER CODE BEGIN PV */
static volatile uint8_t ir_key = 0; // 红外按键标志：0=无，1/2/3=调时/分/秒，4=同步网络时间到 DS1302

// ESP8266 串口接收缓冲（USART3，中断写 / 任务读）
static uint8_t uart_rx_byte = 0;             // 单字节中断接收缓冲
static char uart_line[32];                   // 一行接收缓冲
static volatile uint8_t uart_line_len = 0;   // 已接收字节数
static volatile uint8_t uart_line_ready = 0; // 1=收到完整一行
static volatile uint8_t rx_led_flag = 0;     // 1=收到一行 ESP 数据，LED2 任务消费后清零

// ESP8266 解析出的时间（仅任务上下文访问）
static uint8_t esp_hour = 0;
static uint8_t esp_minute = 0;
static uint8_t esp_year = 0;           // 年（2位，0-99，表示 2000-2099）
static uint8_t esp_month = 0;
static uint8_t esp_date = 0;
static uint8_t esp_second = 0;
static uint8_t esp_weekday = 0;        // 星期（1=周日 .. 7=周六）
static uint8_t esp_has_time = 0;       // 1=已收到过 ESP 时间
static uint32_t esp_last_rx_tick = 0;  // 最后收到 ESP 时间的时间戳(ms)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART3_UART_Init(void);
void StartLedTask(void const * argument);
void StartLed2Task(void const * argument);

/* USER CODE BEGIN PFP */
void StartClockTask(void const * argument);
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
  MX_TIM3_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  TM1637_Init();
  TM1637_SetBrightness(TM1637_BRIGHTNESS_7);
  DS1302_Init();
  HX1838_Init();
  HAL_UART_Receive_IT(&huart3, &uart_rx_byte, 1); // 启动中断接收（每次1字节）
  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of LED_Task */
  osThreadDef(LED_Task, StartLedTask, osPriorityIdle, 0, 128);
  LED_TaskHandle = osThreadCreate(osThread(LED_Task), NULL);

  /* definition and creation of LED2_Task */
  osThreadDef(LED2_Task, StartLed2Task, osPriorityIdle, 0, 128);
  LED2_TaskHandle = osThreadCreate(osThread(LED2_Task), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  osThreadDef(clockTask, StartClockTask, osPriorityNormal, 0, 128);
  osThreadCreate(osThread(clockTask), NULL);
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 71;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, TM1637_CLK_Pin|TM1637_DIO_Pin|DS1302_CLK_Pin|DS1302_DAT_Pin
                          |DS1302_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED1_Pin|LED2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : TM1637_CLK_Pin TM1637_DIO_Pin DS1302_CLK_Pin DS1302_DAT_Pin
                           DS1302_RST_Pin */
  GPIO_InitStruct.Pin = TM1637_CLK_Pin|TM1637_DIO_Pin|DS1302_CLK_Pin|DS1302_DAT_Pin
                          |DS1302_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED1_Pin LED2_Pin */
  GPIO_InitStruct.Pin = LED1_Pin|LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// USART3 接收完成回调（中断上下文，务必简短）
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART3)
  {
    if (uart_rx_byte == '\n')
    {
      // 收到换行符：一行结束
      rx_led_flag = 1;   // 通知 LED2 任务：收到一行数据，闪一下
      if (uart_line_len >= 19)   // "YYYY-MM-DD HH:MM:SS" 至少 19 字符
      {
        uart_line[uart_line_len] = '\0';
        uart_line_ready = 1;
      }
      uart_line_len = 0;
    }
    else if (uart_rx_byte != '\r')   // 忽略回车符
    {
      if (uart_line_len < sizeof(uart_line) - 1)
      {
        uart_line[uart_line_len++] = (char)uart_rx_byte;
      }
      else
      {
        uart_line_len = 0;   // 溢出丢弃，重新开始
      }
    }
    // 重新启动下一次接收
    HAL_UART_Receive_IT(&huart3, &uart_rx_byte, 1);
  }
}

// 计算星期（Sakamoto 算法），返回 DS1302 的 day 字段（1=周日 ... 7=周六）
static uint8_t ESP_CalcWeekday(uint16_t y, uint8_t m, uint8_t d)
{
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  int yy = y;
  yy -= (m < 3);
  int w = (yy + yy / 4 - yy / 100 + yy / 400 + t[m - 1] + d) % 7; // 0=周日
  return (uint8_t)(w + 1);
}

// 解析 "YYYY-MM-DD HH:MM:SS"，成功返回 1
static uint8_t ESP_ParseTime(uint16_t *year, uint8_t *month, uint8_t *date,
                             uint8_t *hour, uint8_t *minute, uint8_t *second)
{
  const char *s = uart_line;
  // 校验分隔符位置
  if (s[4] != '-' || s[7] != '-' || s[10] != ' ' || s[13] != ':' || s[16] != ':')
    return 0;

  *year   = (uint16_t)((s[0]-'0')*1000 + (s[1]-'0')*100 + (s[2]-'0')*10 + (s[3]-'0'));
  *month  = (uint8_t)((s[5]-'0')*10 + (s[6]-'0'));
  *date   = (uint8_t)((s[8]-'0')*10 + (s[9]-'0'));
  *hour   = (uint8_t)((s[11]-'0')*10 + (s[12]-'0'));
  *minute = (uint8_t)((s[14]-'0')*10 + (s[15]-'0'));
  *second = (uint8_t)((s[17]-'0')*10 + (s[18]-'0'));

  // 范围校验
  if (*month < 1 || *month > 12) return 0;
  if (*date < 1 || *date > 31)   return 0;
  if (*hour > 23 || *minute > 59 || *second > 59) return 0;
  return 1;
}

// 红外接收完成回调（在 TIM3 中断上下文执行，务必简短）
void HX1838_OnReceive(uint8_t addr, uint8_t cmd, uint8_t repeat)
{
    (void)addr;
    if (repeat) return; // 忽略重复码

    switch (cmd) {
        case IR_KEY_1: ir_key = 1; break;
        case IR_KEY_2: ir_key = 2; break;
        case IR_KEY_3: ir_key = 3; break;
        case IR_KEY_4: ir_key = 4; break;
        default: break;
    }
}

// 把缓存的 ESP 网络时间写回 DS1302（23:50 自动校准与按键4 手动同步共用）
static void ESP_SyncToDS1302(void)
{
  if (!esp_has_time) return; // 还没收到过网络时间，无法同步

  DS1302_Time_t t;
  t.year   = esp_year;
  t.month  = esp_month;
  t.date   = esp_date;
  t.hour   = esp_hour;
  t.minute = esp_minute;
  t.second = esp_second;
  t.day    = esp_weekday;
  DS1302_SetTime(&t);
}

// 主时钟任务：显示 HH:MM，ESP 时间优先，DS1302 兜底，每天 23:50 校准一次
void StartClockTask(void const * argument)
{
  DS1302_Time_t time;
  uint8_t hour, minute;
  static uint8_t colon_state = 0;          // 冒号当前亮灭
  static uint8_t colon_tick = 0;           // 100ms 计数，5 次 = 500ms 翻转一次
  static uint8_t esp_calibrated_today = 0; // 每天 23:50 是否已校准

  for(;;)
  {
    // ---- 1. 处理 ESP8266 发来的时间 ----
    if (uart_line_ready)
    {
      uint16_t year;
      uint8_t month, date, h, m, s;
      uart_line_ready = 0;
      if (ESP_ParseTime(&year, &month, &date, &h, &m, &s))
      {
        esp_hour    = h;
        esp_minute  = m;
        esp_year    = (uint8_t)(year % 100);
        esp_month   = month;
        esp_date    = date;
        esp_second  = s;
        esp_weekday = ESP_CalcWeekday(year, month, date);
        esp_has_time = 1;
        esp_last_rx_tick = HAL_GetTick();

        // 每天 23:50 用 ESP 时间校准一次 DS1302
        if (h == 23 && m == 50)
        {
          if (!esp_calibrated_today)
          {
            ESP_SyncToDS1302();
            esp_calibrated_today = 1;
          }
        }
        else
        {
          esp_calibrated_today = 0;   // 离开 23:50，复位标志供下一天使用
        }
      }
    }

    // ---- 1.5 按键4：手动把网络时间同步到 DS1302（仅当 3 秒内收到过，保证时间新鲜） ----
    if (ir_key == 4)
    {
      ir_key = 0;
      if (esp_has_time && (HAL_GetTick() - esp_last_rx_tick) < 3000U)
      {
        ESP_SyncToDS1302();
      }
    }

    // ---- 2. 选择时间源：ESP 在发(3秒内有数据)就用 ESP，否则用 DS1302 ----
    if (esp_has_time && (HAL_GetTick() - esp_last_rx_tick) < 3000U)
    {
      hour   = esp_hour;
      minute = esp_minute;
      ir_key = 0; // 丢弃 ESP 在线期间误按的红外键，避免标志滞留到掉线后突然调时
    }
    else
    {
      DS1302_GetTime(&time);
      hour   = time.hour;
      minute = time.minute;

      // 红外按键调整（仅当 DS1302 作为显示源时生效）
      if (ir_key != 0)
      {
        uint8_t key = ir_key;
        ir_key = 0;

        if (key == 1) {
          time.hour++;
          if (time.hour >= 24) time.hour = 0;
        } else if (key == 2) {
          time.minute++;
          if (time.minute >= 60) time.minute = 0;
        } else if (key == 3) {
          time.second = 0;
        }
        DS1302_SetTime(&time);
        hour   = time.hour;
        minute = time.minute;
      }
    }

    // ---- 3. 冒号闪烁：本地计数，与时间源解耦 ----
    colon_tick++;
    if (colon_tick >= 5)
    {
      colon_tick = 0;
      colon_state = !colon_state;
    }

    // ---- 4. 显示 HH:MM ----
    TM1637_SetDigit(0, hour / 10, 0);
    TM1637_SetDigit(1, hour % 10, colon_state);
    TM1637_SetDigit(2, minute / 10, 0);
    TM1637_SetDigit(3, minute % 10, 0);

    osDelay(100);
  }
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartLedTask */
/**
  * @brief  Function implementing the LED_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartLedTask */
void StartLedTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    osDelay(500);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartLed2Task */
/**
* @brief Function implementing the LED2_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLed2Task */
void StartLed2Task(void const * argument)
{
  /* USER CODE BEGIN StartLed2Task */
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);   // 初始灭（LED 低电平点亮）

  /* Infinite loop */
  for(;;)
  {
    if (rx_led_flag)
    {
      rx_led_flag = 0;
      HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); // 亮（低电平）
      osDelay(500);
      HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);   // 灭（高电平）
    }
    osDelay(10);   // 轮询间隔
  }
  /* USER CODE END StartLed2Task */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
