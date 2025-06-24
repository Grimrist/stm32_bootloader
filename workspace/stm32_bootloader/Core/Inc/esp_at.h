/*
 * esp_at.h
 *
 *  Created on: May 12, 2025
 *      Author: moonflax
 */

#ifndef INC_ESP_AT_H_
#define INC_ESP_AT_H_

typedef enum {
	AT_OK,
	AT_ERROR,
	AT_OVERFLOW,
} AT_Status;

struct AP_Settings {
	char ssid[32];
	char pwd[63];
	char bssid[32];
};

struct AT_Command {
	char* cmd;
	char** args;
	size_t arg_count;
};

struct AT_Reception {
	char* resp_buf;
	size_t buf_size;
	char** token_arr;
};

AT_Status at_recv(UART_HandleTypeDef* huart, char* resp_buf, size_t buf_size, char** token_arr, int max_tok, uint32_t timeout);
HAL_StatusTypeDef at_send(UART_HandleTypeDef* huart, struct AT_Command at_cmd);
AT_Status at_exchange(UART_HandleTypeDef* huart, struct AT_Command at_cmd, char* resp_buf, size_t buf_size, char** token_arr, size_t max_tok);
AT_Status at_head(UART_HandleTypeDef* huart, char* url, char* buf, size_t buf_size, char** token_arr, size_t max_tok);
int at_connect_wifi(UART_HandleTypeDef* huart, struct AP_Settings settings);
AT_Status at_get_file(UART_HandleTypeDef* huart, char* url, char* buf, size_t buf_size, size_t* range);
size_t at_get_file_size(UART_HandleTypeDef* huart, char* url, char* buf, size_t buf_size);

#endif /* INC_ESP_AT_H_ */
