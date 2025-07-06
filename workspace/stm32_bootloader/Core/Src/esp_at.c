/*
 * esp_at.c
 *
 *  Created on: May 12, 2025
 *      Author: moonflax
 *
 *  Utility functions for sending AT commands and receiving AT responses
 */

#include <stddef.h>
#include "usart.h"
#include <string.h>
#include "esp_at.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "bl.h"

#define UART_SEND_TIMEOUT 2000
#define UART_RECV_TIMEOUT 8000
#define UART_WIFI_TIMEOUT 10000
#define HTTP_HEAD "1"
#define HTTP_GET "2"
#define HTTP_TCP "1"

char msg_buf[256] = {0};
char* msg_tokens[10] = {0};

/* Generic parsing functions for AT calls */
AT_Status at_recv(UART_HandleTypeDef* huart, char* resp_buf, size_t buf_size, char** token_arr, int max_tok, uint32_t timeout) {
	if (max_tok == 0) {
		max_tok = INT_MAX;
	}
	int i = 0;
	char* resp_buf_origin = resp_buf;
	HAL_StatusTypeDef status;
	while((status = HAL_UART_Receive(huart, (uint8_t*) resp_buf, 1, timeout)) == HAL_OK) {
		resp_buf++;
		i++;
		if(i > buf_size) {
			return AT_OVERFLOW;
		}
		if(i > 4) {
			if(strncmp("OK\r\n", resp_buf-4, 4) == 0) {
				break;
			}
		}
		if(i > 7) {
			if(strncmp("ERROR\r\n", resp_buf-7, 7) == 0) {
				return AT_ERROR;
			}
		}
	}
	i = 0;
	char* tok = strtok(resp_buf_origin, "\r\n");
	while(tok != NULL) {
		token_arr[i] = tok;
		i++;
		if (i >= max_tok)
			break;
		tok = strtok(NULL, "\r\n");
	}
	return AT_OK;
}

HAL_StatusTypeDef at_send(UART_HandleTypeDef* huart, struct AT_Command at_cmd) {
	memset(msg_buf, 0, sizeof(msg_buf));
	sprintf(msg_buf, "AT+%s=", at_cmd.cmd);
	for(size_t i = 0; i < at_cmd.arg_count; i++) {
		strcat(msg_buf, at_cmd.args[i]);
		if (i != at_cmd.arg_count-1)
			strcat(msg_buf, ",");
		else
			strcat(msg_buf, "\r\n");
	}
	return HAL_UART_Transmit(huart, (uint8_t*) msg_buf, strlen(msg_buf), UART_SEND_TIMEOUT);
}

AT_Status at_exchange(UART_HandleTypeDef* huart, struct AT_Command at_cmd, char* resp_buf, size_t buf_size, char** token_arr, size_t max_tok) {
	at_send(huart, at_cmd);
	return at_recv(huart, resp_buf, buf_size, token_arr, max_tok, UART_RECV_TIMEOUT);
}
/** Specific AT command calls **/

/*
 * Connect to wifi using AT+CWJAP
 * AT+CWJAP=[<"ssid">],[<"pwd">][,<"bssid">][,<pci_en>][,<reconn_interval>][,<listen_interval>][,<scan_mode>][,<jap_timeout>][,<pmf>]
 */

AT_Status at_head(UART_HandleTypeDef* huart, char* url, char* buf, size_t buf_size, char** token_arr, size_t max_tok) {
	char* url_encoded = malloc(strlen(url) + 3);
	sprintf(url_encoded, "\"%s\"", url);
	char* args[] = {HTTP_HEAD,"0",url_encoded,"","",HTTP_TCP};
	size_t arg_count = 6;
	struct AT_Command http_get = {
		.cmd = "HTTPCLIENT",
		.args = args,
		.arg_count = arg_count
	};
	if (at_exchange(huart, http_get, buf, buf_size, token_arr, max_tok) != AT_OK)
		return AT_ERROR;
	return AT_OK;
}

int at_connect_wifi(UART_HandleTypeDef* huart, struct AP_Settings settings) {
	char* wifi_settings[2];
	wifi_settings[0] = malloc(strlen(settings.ssid) + 3);
	wifi_settings[1] = malloc(strlen(settings.pwd) + 3);
	sprintf(wifi_settings[0], "\"%s\"", settings.ssid);
	sprintf(wifi_settings[1], "\"%s\"", settings.pwd);
	char* args[] = {wifi_settings[0], wifi_settings[1]};
	struct AT_Command ap_settings = {
		.cmd = "CWJAP",
		.args = args,
		.arg_count = 2
	};
	at_send(huart, ap_settings);
	memset(msg_buf, 0, sizeof(msg_buf));
	if (at_recv(huart, msg_buf, sizeof(msg_buf), msg_tokens, sizeof(msg_tokens), UART_WIFI_TIMEOUT) != AT_OK) {
		return -1;
	}
	return 0;
}

size_t at_get_file_size(UART_HandleTypeDef* huart, char* url, char* buf, size_t buf_size) {
	char* url_encoded = malloc(strlen(url) + 3);
	sprintf(url_encoded, "\"%s\"", url);
	char* args[] = {url_encoded,"2048","2048","10000"};
	struct AT_Command http_get = {
		.cmd = "HTTPGETSIZE",
		.args = args,
		.arg_count = 2
	};
	char* token_arr[4] = {0};
	if (at_exchange(huart, http_get, buf, buf_size, token_arr, sizeof(token_arr)) != AT_OK)
		return 0;

	strtok(token_arr[1], ":");
	char* file_size_str = strtok(NULL, ":");
	return atoi(file_size_str);
}

AT_Status at_get_file(UART_HandleTypeDef* huart, char* url, char* buf, size_t buf_size, size_t* range) {
//	size_t file_size = at_get_file_size(huart, url, buf, buf_size);
	// TODO: Figure out some size threshold because of extra metadata from AT responses
//	if (file_size >= buf_size - strlen(url) - 200) {
//		return AT_ERROR;
//	}

	// Begin fetching file
	char* url_encoded = malloc(strlen(url) + 3);
	sprintf(url_encoded, "\"%s\"", url);
	char headers[40];
	int arg_count = 6;
	if (range != NULL) {
		sprintf(headers, "\"Range: bytes=%d-%d\"", range[0], range[1]);
		arg_count = 7;
	}
	char* args[] = {HTTP_GET,"0",url_encoded,"","",HTTP_TCP,headers};
	struct AT_Command http_get = {
		.cmd = "HTTPCLIENT",
		.args = args,
		.arg_count = arg_count
	};
	char* token_arr[6] = {0};
	if (at_exchange(huart, http_get, buf, buf_size, token_arr, 1) != AT_OK)
		return AT_ERROR;
	char* temp = token_arr[0] + strlen(token_arr[0]) + 1;
	strtok(temp, ",");
	char* data_ptr = temp + strlen(temp) + 1;
	// We move the entire file to the start of bin_file to ensure double-word alignment
	memmove(buf, data_ptr, (range[1] - range[0] + 1));
	return AT_OK;
}


