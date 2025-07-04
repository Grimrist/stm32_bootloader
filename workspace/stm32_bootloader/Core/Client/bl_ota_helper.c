/*
 * bl_ota_helper.c
 *
 *  Created on: Jul 2, 2025
 *      Author: moonflax
 */

#include "bl.h"
#include "bl_ota_helper.h"

void bl_ota_mark_valid() {
	struct App_Status_Data* app_status = (struct App_Status_Data*) RAM_PERSIST_ADDR;
	app_status->magic = MAGIC;
	app_status->status = BL_VALID;
}

void bl_ota_mark_invalid() {
	struct App_Status_Data* app_status = (struct App_Status_Data*) RAM_PERSIST_ADDR;
	app_status->magic = MAGIC;
	app_status->status = BL_INVALID;
}

void bl_ota_mark_aborted() {
	struct App_Status_Data* app_status = (struct App_Status_Data*) RAM_PERSIST_ADDR;
	app_status->magic = MAGIC;
	app_status->status = BL_ABORTED;
}

bl_app_status_t bl_ota_check_status() {
	struct App_Status_Data* app_status = (struct App_Status_Data*) RAM_PERSIST_ADDR;
	if(app_status->magic == MAGIC) {
		return app_status->status;
	}
}
