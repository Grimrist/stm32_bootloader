/*
 * nvs.h
 *
 *  Created on: Jun 22, 2025
 *      Author: moonflax
 */

#ifndef INC_NVS_H_
#define INC_NVS_H_

#include "bl.h"

bl_status_t validate_app(struct App_Data* app);
bl_status_t validate_crc32(struct App_Data* app);
bl_status_t update_app_metadata(struct App_Data* app1, struct App_Data* app2);

#endif /* INC_NVS_H_ */
