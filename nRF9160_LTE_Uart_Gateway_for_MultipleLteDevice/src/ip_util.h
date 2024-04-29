/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __IP_UTIL_H
#define __IP_UTIL_H

#include <zephyr/kernel.h>

typedef struct __pseuso_ip_hdr_t {
    uint8_t    ip_src[4];
    uint8_t    ip_dst[4];
    uint8_t     dummy;
    uint8_t     ip_p;
    uint16_t    ip_len;
} pseudo_ip_hdr_t;

void calc_ics(uint8_t *buffer, int len, int hcs_pos);
uint16_t check_ics(const uint8_t *buffer, int len);
uint16_t ip_checksum2(uint8_t *data1, int len1, uint8_t *data2, int len2);
uint16_t ip_htons( uint16_t hostshort);
#endif //__IP_UTIL_H