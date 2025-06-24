/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "crc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdlib.h>
#include "esp_at.h"
#include "bl.h"
#include <stdio.h>
#include <errno.h>
#include "nvs.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UPDATE_URL "http://192.168.100.88:8000/latest"

#define WRITE_CMD 0x31
#define ERASE_CMD 0x43
#define GO_CMD 0x20
#define UPDATE_CMD 0x99
#define ACK_CMD 0x79
#define NACK_CMD 0x1F
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define min(a,b) \
	({ __typeof__ (a) _a = (a); \
	   __typeof__ (b) _b = (b); \
	   _a < _b ? _a : _b; })
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static FLASH_EraseInitTypeDef EraseInitStruct;
static uint32_t PageError;
char rx_cmd[2] = {0,0};
struct app_vectable_ {
	uint32_t Initial_SP;
	void (*Reset_Handler)(void);
};

uint32_t write_addr;
uint32_t write_filesize;
char bin_file[32768/2]; // Allocated pre-emptively
uint32_t go_addr;
uint8_t ack_cmd = ACK_CMD;
uint8_t nack_cmd = NACK_CMD;
uint8_t* ACK = &ack_cmd;
uint8_t* NACK = &nack_cmd;
__IO uint32_t uwCRCValue = 0;
uint32_t crc_recv = 0;

extern uint64_t _bdata;
char resp_buff[512] = {0};
char* resp_tokens[20] = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int bl_erase() {
	uint32_t page_count;
	uint32_t initial_page;
	HAL_UART_Receive(&huart2, &page_count, 4, -1);
	HAL_UART_Receive(&huart2, &initial_page, 4, -1);
	/* Unlock the Flash to enable the flash control register access */
	if (HAL_FLASH_Unlock() != HAL_OK) {
		return -1;
	}

	/* TODO: Add NACK for invalid page count or initial page */
	if(initial_page > 255) {
		EraseInitStruct.Banks = FLASH_BANK_2;
	} else
		EraseInitStruct.Banks = FLASH_BANK_1;

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase 	= FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Page 		= initial_page;
	EraseInitStruct.NbPages 	= page_count;

	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
	    HAL_FLASH_Lock();
		return -1;
	}
	HAL_FLASH_Lock();
	return 0;
}

int bl_write() {
	HAL_UART_Receive(&huart2, &write_addr, 4, -1);
	// TODO: Check for valid address
	HAL_UART_Transmit(&huart2, ACK, 1, -1);
	uint32_t file_size = 0;
	HAL_UART_Receive(&huart2, &file_size, sizeof(file_size), -1);
	// Receive checksum
	HAL_UART_Receive(&huart2, &crc_recv, sizeof(crc_recv), -1);

	if(file_size > sizeof(bin_file)) {
		return -1;
	}
	HAL_UART_Receive(&huart2, bin_file, file_size, -1);

	// Calculate checksum
	uwCRCValue = HAL_CRC_Calculate(&hcrc, (uint32_t *) bin_file, file_size);
	uwCRCValue = ~uwCRCValue;

	if (uwCRCValue != crc_recv) {
		return -1;
	}

	uint64_t* file_ptr = bin_file;
	uint32_t user_addr = write_addr;
	/* Unlock the Flash to enable the flash control register access */
	if (HAL_FLASH_Unlock() != HAL_OK) {
		return -1;
	}

	for(int i = 0; i < (file_size / 8); i++) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, user_addr, *file_ptr) == HAL_ERROR) {
			uint32_t err = HAL_FLASH_GetError();
			HAL_FLASH_Lock();
			return -1;
		}
		user_addr += 8;
		file_ptr += 1;
	}
	HAL_FLASH_Lock();
	HAL_UART_Transmit(&huart2, ACK, 1, -1);
	return 0;
}

int bl_go() {
	HAL_UART_Receive(&huart2, &go_addr, 4, -1);
	uint8_t bank_swp = 0;
	uint32_t vtor_addr = go_addr;
	// TODO: Check for valid address
	if (go_addr >= 0x08080000) {
		bank_swp = 1;
		go_addr -= 0x00080000;
	}
	HAL_UART_Transmit(&huart2, ACK, 1, -1);
	/* Begin preparing for application jump */
	HAL_RCC_DeInit();
	HAL_DeInit();
	__disable_irq();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;
	if(bank_swp == 1) {
		// disable ART
		CLEAR_BIT(FLASH->ACR, FLASH_ACR_PRFTEN);
		CLEAR_BIT(FLASH->ACR, FLASH_ACR_ICEN | FLASH_ACR_DCEN);
		// clear cache
		SET_BIT(FLASH->ACR, FLASH_ACR_ICRST | FLASH_ACR_DCRST);
		SET_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE);

		SET_BIT(FLASH->ACR, FLASH_ACR_PRFTEN);
		SET_BIT(FLASH->ACR, FLASH_ACR_ICEN | FLASH_ACR_DCEN);
	}
	SCB->VTOR = vtor_addr;
	//__set_MSP(_estack); // Verify whether this is actually necessary
	__DSB();
	__ISB();
	__enable_irq();
	uint32_t Reset_Handler_addr = go_addr + 4;
	void (*Reset_Handler)(void) = *((uint32_t*) Reset_Handler_addr);
	Reset_Handler();
	/* This should never execute */
	while(1) {
		// TODO: Add warning that execution failed
	}
}

int bl_update() {
	bl_erase();
	bl_write();
	/* Write which bank is being used to flash */
	if (_bdata == 1)
	bl_go();
	return -1;
}
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
  MX_USART2_UART_Init();
  MX_CRC_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  {
  /* Initialize boot info structs */
  struct App_Data app1 = *((struct App_Data*) (&_nvs_start));
  struct App_Data app2 = *((struct App_Data*) (&_nvs_start + sizeof(struct App_Data)));

  /* Enable WiFi on slave */
  char* args[] = {"1", "0"};
  struct AT_Command enable_wifi = {
	  .cmd = "CWMODE",
	  .args = args,
	  .arg_count = 2
  };
  if(at_exchange(&huart1, enable_wifi, resp_buff, sizeof(resp_buff), resp_tokens, sizeof(resp_tokens)) != AT_OK) {
	  _bl_boot();
	  goto at_end;
  }
  struct AP_Settings settings = {
    .ssid = "hotspot-m5",
	.pwd = "guak4768"
  };
  if(at_connect_wifi(&huart1, settings) != 0) {
	  _bl_boot();
	  goto at_end;
  }
  char url_target[] = UPDATE_URL;
  if(at_head(&huart1, url_target, bin_file, sizeof(bin_file), resp_tokens, sizeof(resp_tokens)) != AT_OK) {
	  _bl_boot();
	  goto at_end;
  }
  uint32_t crc32 = 0;
  uint8_t force_upd = 0;
  char* crc32_str;
  char* force_upd_str;
  for (int i = 0; i < sizeof(resp_tokens)/sizeof(resp_tokens[0]); i++) {
	  if(crc32_str = strstr(resp_tokens[i], "x-crc32")) {
		  errno = 0;
		  crc32 = strtoll(crc32_str+8, NULL, 10);
	  }
	  if(force_upd_str = strstr(resp_tokens[i], "x-forceupd")) {
		  errno = 0;
		  force_upd = strtoll(force_upd_str+11, NULL, 10);
	  }
  }
  if(!crc32) {
	  _bl_boot();
	  goto at_end;
  }
  if (force_upd != 1) {
	  if(app1.crc32 == crc32 || app2.crc32 == crc32) {
		  _bl_boot();
		  goto at_end;
	  }
  }
  size_t file_size = at_get_file_size(&huart1, url_target, bin_file, sizeof(bin_file));
  // TODO: Make HEAD call to check for Accept-Ranges
  int page_count = (file_size + 2048 - 1) / 2048;
  int target_page = -1;
  if(app1.status != BL_VALID)
	  target_page = 40;
  else if(app2.status != BL_VALID)
	  target_page = 40 + 256;
  if (_bl_erase(page_count, target_page) == -1)
	  while(1);
  	  // TODO: Handle erase failure
  size_t file_loaded = 0;
  size_t next_file_loaded = 0;
  while (file_size > file_loaded) {
	  memset(bin_file, 0, sizeof(bin_file));
	  next_file_loaded = min(file_size,file_loaded + 1024);
	  size_t range[2] = {file_loaded, next_file_loaded-1};
	  at_get_file(&huart1, url_target, bin_file, sizeof(bin_file), range);
	  _bl_write(bin_file, next_file_loaded - file_loaded, 0x08000000 + (2048 * target_page) + file_loaded);
	  file_loaded = next_file_loaded;
  }

  if(app1.status != BL_VALID) {
	  app1.magic = MAGIC;
	  app1.address = 0x08000000 + (2048 * target_page);
	  app1.crc32 = crc32;
	  app1.length = file_size;
	  if(validate_app(&app1) == BL_ERROR || validate_crc32(&app1) == BL_ERROR)
		  app1.status = BL_INVALID;
	  else {
		  app1.status = BL_VALID;
		  app2.status = BL_INVALID;
	  }
  }
  else if(app2.status != BL_VALID) {
	  app2.magic = MAGIC;
	  app2.address = 0x08000000 + (2048 * target_page);
	  app2.crc32 = crc32;
	  app2.length = file_size;
	  if(validate_app(&app2) == BL_ERROR || validate_crc32(&app2) == BL_ERROR)
		  app2.status = BL_INVALID;
	  else {
		  app2.status = BL_VALID;
		  app1.status = BL_INVALID;
	  }
  }
  if(update_app_metadata(&app1, &app2) == BL_OK)
	  _bl_boot();
  at_end:
  	  __NOP();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	HAL_UART_Receive(&huart2, rx_cmd, 2, -1);
	if((rx_cmd[0] ^ rx_cmd[1]) != 255) {
		HAL_UART_Transmit(&huart2, NACK, 1, -1);
	}
	if(rx_cmd[0] == WRITE_CMD) {
		HAL_UART_Transmit(&huart2, ACK, 1, -1);
		if (bl_write() == 0) {
			HAL_UART_Transmit(&huart2, ACK, 1, -1);
			continue;
		}
	}
	else if(rx_cmd[0] == ERASE_CMD) {
		HAL_UART_Transmit(&huart2, ACK, 1, -1);
		if (bl_erase() == 0) {
			HAL_UART_Transmit(&huart2, ACK, 1, -1);
			continue;
		}
	}
	else if(rx_cmd[0] == GO_CMD) {
		HAL_UART_Transmit(&huart2, ACK, 1, -1);
		if (bl_go() == 0) {
			HAL_UART_Transmit(&huart2, ACK, 1, -1);
			continue;
		}
	}
	else if(rx_cmd[0] == UPDATE_CMD) {
		HAL_UART_Transmit(&huart2, ACK, 1, -1);
		if (bl_update() == 0) {
			HAL_UART_Transmit(&huart2, ACK, 1, -1);
			continue;
		}
	}
	HAL_UART_Transmit(&huart2, NACK, 1, -1);
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
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

#ifdef  USE_FULL_ASSERT
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
