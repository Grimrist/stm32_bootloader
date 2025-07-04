/*
 * bl_ota_helper.h
 *
 *  Created on: Jul 2, 2025
 *      Author: moonflax
 */

#ifndef CLIENT_BL_OTA_HELPER_H_
#define CLIENT_BL_OTA_HELPER_H_

#include "bl.h"

void bl_ota_mark_valid();
void bl_ota_mark_invalid();
void bl_ota_mark_aborted();
bl_app_status_t bl_ota_check_status();

#endif /* CLIENT_BL_OTA_HELPER_H_ */
