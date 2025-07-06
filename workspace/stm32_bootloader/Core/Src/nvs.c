/*
 * nvs.c
 *
 *  Created on: Jun 22, 2025
 *      Author: moonflax
 */

#include "bl.h"
#include "crc.h"
#include "nvs.h"

bl_status_t validate_app(struct App_Data* app) {
	if (app->magic != MAGIC)
		return BL_ERROR;
	if (app->address < &_binary_end) // TODO: Add calculation for maximum length
		return BL_ERROR;
	return BL_OK;
}

bl_status_t validate_crc32(struct App_Data* app) {
	#ifndef SKIP_CHKSUM
	uint32_t crc32_test = ~HAL_CRC_Calculate(&hcrc, (uint32_t*) app->address, app->length);
	if (crc32_test != app->crc32)
		return BL_ERROR;
	#endif
	return BL_OK;
}

bl_status_t update_app_metadata(struct App_Data* app1, struct App_Data* app2) {
	if (_bl_erase(1, GET_FLASH_PAGE((uint32_t)&_nvs_start)) == BL_ERROR) {
		return BL_ERROR;
	}
	if (_bl_write(app1, sizeof(struct App_Data), &_nvs_start) != 0)
		return BL_ERROR;
	if (_bl_write(app2, sizeof(struct App_Data), &_nvs_start + sizeof(struct App_Data)) != 0)
		return BL_ERROR;
	return BL_OK;
}

void bl_ota_move_status_to_ram(struct App_Data* app) {
	struct App_Status_Data* app_status = RAM_PERSIST_ADDR;
	app_status->magic = MAGIC;
	app_status->status = app->status;
}
