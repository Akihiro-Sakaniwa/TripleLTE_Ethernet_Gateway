/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "nrf91_util.h"

struct at_param_list at_param_list;

void us_delay(uint32_t us){
    uint32_t cycles_spent;
    uint32_t nanseconds_spent = 0;
    uint32_t start_time;
    __BARRIER__;
    start_time = k_cycle_get_32();
    __BARRIER__;
    while (nanseconds_spent <= (us*1000)) {
        __BARRIER__;
		if (k_cycle_get_32() >= start_time)	
		{
			cycles_spent = k_cycle_get_32() - start_time;
		}
		else
		{
			cycles_spent = (0xFFFFFFFFUL - start_time) + k_cycle_get_32();
		}
        __BARRIER__;
       nanseconds_spent = SYS_CLOCK_HW_CYCLES_TO_NS_AVG(cycles_spent,1);
       __BARRIER__;
    }
    return;
}
/**
 * @brief Compare string ignoring case
 */
bool nrf91_util_casecmp(const char *str1, const char *str2)
{
	int str2_len = strlen(str2);

	if (strlen(str1) != str2_len) {
		return false;
	}

	for (int i = 0; i < str2_len; i++) {
		if (toupper((int)*(str1 + i)) != toupper((int)*(str2 + i))) {
			return false;
		}
	}

	return true;
}


/**
 * @brief Compare name of AT command ignoring case
 */
bool nrf91_util_cmd_casecmp(const char *cmd, const char *nrf91_cmd)
{
	int i;
	int nrf91_cmd_len = strlen(nrf91_cmd);

	if (strlen(cmd) < nrf91_cmd_len) {
		return false;
	}

	for (i = 0; i < nrf91_cmd_len; i++) {
		if (toupper((int)*(cmd + i)) != toupper((int)*(nrf91_cmd + i))) {
			return false;
		}
	}
#if defined(CONFIG_NRF91_CR_LF_TERMINATION)
	if (strlen(cmd) > (nrf91_cmd_len + 2))
#else
	if (strlen(cmd) > (nrf91_cmd_len + 1))
#endif
	{
		char ch = *(cmd + i);
		/* With parameter, SET TEST, "="; READ, "?" */
		return ((ch == '=') || (ch == '?'));
	}

	return true;
}

/**
 * @brief Detect hexdecimal string data type
 */
bool nrf91_util_hexstr_check(const uint8_t *data, uint16_t data_len)
{
	for (int i = 0; i < data_len; i++) {
		char ch = *(data + i);

		if ((ch < '0' || ch > '9') &&
		    (ch < 'A' || ch > 'F') &&
		    (ch < 'a' || ch > 'f')) {
			return false;
		}
	}

	return true;
}

/**
 * @brief Encode hex array to hexdecimal string (ASCII text)
 */
int nrf91_util_htoa(const uint8_t *hex, uint16_t hex_len, char *ascii, uint16_t ascii_len)
{
	if (hex == NULL || ascii == NULL) {
		return -EINVAL;
	}
	if (ascii_len < (hex_len * 2)) {
		return -EINVAL;
	}

	for (int i = 0; i < hex_len; i++) {
		sprintf(ascii + (i * 2), "%02X", *(hex + i));
	}

	return (hex_len * 2);
}

/**
 * @brief Decode hexdecimal string (ASCII text) to hex array
 */
int nrf91_util_atoh(const char *ascii, uint16_t ascii_len, uint8_t *hex, uint16_t hex_len)
{
	char hex_str[3];

	if (hex == NULL || ascii == NULL) {
		return -EINVAL;
	}
	if ((ascii_len % 2) > 0) {
		return -EINVAL;
	}
	if (ascii_len > (hex_len * 2)) {
		return -EINVAL;
	}
	if (!nrf91_util_hexstr_check(ascii, ascii_len)) {
		return -EINVAL;
	}

	hex_str[2] = '\0';
	for (int i = 0; (i * 2) < ascii_len; i++) {
		strncpy(&hex_str[0], ascii + (i * 2), 2);
		*(hex + i) = (uint8_t)strtoul(hex_str, NULL, 16);
	}

	return (ascii_len / 2);
}

/**
 * @brief Get string value from AT command with length check
 */
int util_string_get(const struct at_param_list *list, size_t index, char *value, size_t *len)
{
	int ret;
	size_t size = *len;

	/* at_params_string_get calls "memcpy" instead of "strcpy" */
	ret = at_params_string_get(list, index, value, len);
	if (ret) {
		return ret;
	}
	if (*len < size) {
		*(value + *len) = '\0';
		return 0;
	}

	return -ENOMEM;
}

int util_str_to_int(const char *str_buf, int base, int *output)
{
	int temp;
	char *end_ptr = NULL;

	errno = 0;
	temp = strtol(str_buf, &end_ptr, base);

	if (end_ptr == str_buf || *end_ptr != '\0' ||
	    ((temp == LONG_MAX || temp == LONG_MIN) && errno == ERANGE)) {
		return -ENODATA;
	}

	*output = temp;

	return 0;
}
/**
 * @brief Resolve remote host by hostname or IP address
 */
#define PORT_MAX_SIZE    5 /* 0xFFFF = 65535 */
#define PDN_ID_MAX_SIZE  2 /* 0..10 */

int util_resolve_host(int cid, const char *host, uint16_t port, int family, struct sockaddr *sa)
{
	int err;
	char service[PORT_MAX_SIZE + PDN_ID_MAX_SIZE + 2];
	struct addrinfo *ai = NULL;
	struct addrinfo hints = {
		.ai_flags  = AI_NUMERICSERV | AI_PDNSERV,
		.ai_family = family
	};

	if (sa == NULL) {
		return DNS_EAI_AGAIN;
	}

	/* "service" shall be formatted as follows: "port:pdn_id" */
	snprintf(service, sizeof(service), "%hu:%d", port, cid);
	err = getaddrinfo(host, service, &hints, &ai);
	if (err) {
		return err;
	}

	*sa = *(ai->ai_addr);
	freeaddrinfo(ai);

	if (sa->sa_family != AF_INET && sa->sa_family != AF_INET6) {
		return DNS_EAI_ADDRFAMILY;
	}

	return 0;
}
