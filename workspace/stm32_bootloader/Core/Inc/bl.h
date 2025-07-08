/*
 * bl.h
 *
 *  Created on: May 26, 2025
 *      Author: moonflax
 */

#ifndef INC_BL_H_
#define INC_BL_H_

#include <stdint.h>
#include <stdlib.h>

#define GET_FLASH_PAGE(a) ((a - 0x08000000) / 2048)

/* Disable CRC32 checksum */
//#define SKIP_CHKSUM

/* Disable app rollback */
#define DISABLE_ROLLBACK


#define UPDATE_URL "http://192.168.90.106:8000/latest"
#define WIFI_AP "hotspot-m5"
#define WIFI_PWD "guak4768"

#define MAGIC 0xB007704D
#define RAM_PERSIST_ADDR 0x10000000

extern const volatile void _nvs_start;
extern const volatile void _nvs_end;

extern const volatile void _binary_end;
extern const volatile void _flash_end;

extern const volatile void _estack;

/* Metadata structures for bootloader */
typedef enum {
	BL_INVALID = 0x00,
	BL_VALID = 0x01,
	BL_ABORTED = 0x02,
	BL_PENDING = 0x03
} bl_app_status_t;

struct App_Data {
	uint64_t magic; // Force 64bit alignment on struct
	uint32_t crc32;
	uint32_t address;
	uint32_t length;
	uint8_t status;
};

// For app validation via RAM
struct App_Status_Data {
	uint32_t magic;
	uint8_t status;
};

typedef enum {
	BL_OK = 0x0,
	BL_ERROR = 0x1
} bl_status_t;

int _bl_write(char* bin_file, size_t bin_len, uint32_t write_addr);
int _bl_erase(uint32_t page_count, uint32_t initial_page);
__attribute__ ((long_call, section(".RamFunc"))) void _bl_go(uint32_t go_addr);
void _bl_boot();

#endif /* INC_BL_H_ */
