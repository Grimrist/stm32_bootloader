/*
 * bl.c
 *
 *  Created on: May 26, 2025
 *      Author: moonflax
 *
 *  Functions to
 */

#include <stdint.h>
#include <stddef.h>
#include "usart.h"
#include "bl.h"

static uint32_t PageError;

int _bl_write(char* bin_file, size_t bin_len, uint32_t write_addr) {
	uint64_t* file_ptr = bin_file;
	uint32_t user_addr = write_addr;
	/* Unlock the Flash to enable the flash control register access */
	if(HAL_FLASH_Unlock() != HAL_OK) {
		return -1;
	}
	int remainder = bin_len % 8;
	for(int i = 0; i < remainder; i++) {
		bin_file[bin_len + i] = '\0';
	}
	for(int i = 0; i < ((bin_len + 7) / 8); i++) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, user_addr, *file_ptr) == HAL_ERROR) {
			uint32_t err = HAL_FLASH_GetError();
			HAL_FLASH_Lock();
			return -1;
		}
		user_addr += 8;
		file_ptr += 1;
	}
	HAL_FLASH_Lock();
	return 0;
}

int _bl_erase(uint32_t page_count, uint32_t initial_page) {
	/* Unlock the Flash to enable the flash control register access */
	if (HAL_FLASH_Unlock() != HAL_OK) {
		return -1;
	}

	FLASH_EraseInitTypeDef EraseInitStruct;

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

__attribute__ ((long_call, section(".RamFunc"))) void _bl_go(uint32_t go_addr) {
	uint8_t bank_swp = 0;
	uint32_t vtor_addr = go_addr;
	// TODO: Check for valid address
	if (go_addr >= 0x08080000) {
		bank_swp = 1;
		go_addr -= 0x00080000;
	}
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

void _bl_boot() {
  struct App_Data* app1 = &_nvs_start;
  struct App_Data* app2 = &_nvs_start + sizeof(struct App_Data);
  if (app2->status == BL_VALID) {
	  _bl_go(app2->address);
  }
  else if (app1->status == BL_VALID) {
	  _bl_go(app1->address);
  }
  else return;
}

