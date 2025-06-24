/*
 * partitions.h
 *
 *  Created on: Jun 21, 2025
 *      Author: moonflax
 */

#ifndef INC_PARTITION_H_
#define INC_PARTITION_H_

typedef enum {
	BL_PARTITION_TYPE_BOOTLOADER,
	BL_PARTITION_TYPE_APP,
	BL_PARTITION_TYPE_DATA
} bl_partition_type_t;

typedef enum {
	BL_PARTITION_SUBTYPE_FACTORY,
	BL_PARTITION_SUBTYPE_OTA,
} bl_partition_subtype_t;


struct bl_partition {
	uint16_t magic;
	uint8_t type;
	uint32_t addr;
	uint8_t label[16];
};

#endif /* INC_PARTITION_H_ */
