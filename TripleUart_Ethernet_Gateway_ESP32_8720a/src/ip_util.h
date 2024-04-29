/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#pragma once

#ifndef __IP_UTIL_H__
#define __IP_UTIL_H__

#include <Arduino.h>

typedef struct __pseuso_ip_hdr_t {
    uint8_t    ip_src[4];
    uint8_t    ip_dst[4];
    uint8_t     dummy;
    uint8_t     ip_p;
    uint16_t    ip_len;
} pseudo_ip_hdr_t;

/*****************************************************************************/
static inline uint16_t ip_checksum1(const uint8_t *buffer, uint16_t len)
{
	const uint32_t *ptr32 = (const uint32_t *)buffer;
	uint32_t hcs = 0;
	const uint16_t *ptr16;

	for (int i = len / 4; i > 0; i--) {
		uint32_t s = *ptr32++;

		hcs += s;
		if (hcs < s) {
			hcs++;
		}
	}

	ptr16 = (const uint16_t *)ptr32;

	if (len & 2) {
		uint16_t s = *ptr16++;

		hcs += s;
		if (hcs < s) {
			hcs++;
		}
	}

	if (len & 1) {
		const uint8_t *ptr8 = (const uint8_t *)ptr16;
		uint8_t s = *ptr8;

		hcs += s;
		if (hcs < s) {
			hcs++;
		}
	}

	while (hcs > 0xFFFF) {
		hcs = (hcs & 0xFFFF) + (hcs >> 16);
	}

	return ~hcs; /* One's complement */
}
/*****************************************************************************/
static inline void calc_checksum(uint8_t *buffer, int len, int hcs_pos)
{
	uint16_t *ptr_hcs = (uint16_t *)(buffer + hcs_pos);

	*ptr_hcs = 0;
	uint16_t hcs = 0;

	hcs = ip_checksum1(buffer, len);

	*ptr_hcs = hcs;
}

/*****************************************************************************/
static inline uint16_t ip_checksum2(uint8_t *data1, int len1, uint8_t *data2, int len2)
{
    register uint32_t sum;
    register uint16_t *ptr;
    register int c;

    sum = 0;
    ptr=(uint16_t *)data1;

    for(c = len1; c>1; c -= 2) {
        sum += (*ptr);
        if(sum&0x80000000) {
            sum=(sum&0xFFFF) + (sum>>16);
        }
        ptr++;
    }

    if(c == 1) {
        uint16_t val;
        val = ((*ptr)<<8) + (*data2);
        sum += val;
        if(sum&0x80000000) {
            sum = (sum&0xFFFF) + (sum>>16);
        }
        ptr = (uint16_t *)(data2 + 1);
        len2--;
    }
    else {
        ptr = (uint16_t *)data2;
    }

    for(c = len2; c>1; c -= 2) {
        sum += (*ptr);
        if(sum&0x80000000) {
            sum = (sum&0xFFFF) + (sum>>16);
        }
        ptr++;
    }

    if(c == 1) {
        uint16_t    val;
        val = 0;
        memcpy(&val, ptr, sizeof(uint8_t));
        sum += val;
    }

    while(sum>>16) {
        sum = (sum&0xFFFF) + (sum>>16);
    }

    return(~sum);
}
/*****************************************************************************/
static inline uint16_t ip_htons( uint16_t hostshort)
{
#if 1
  //#ifdef LITTLE_ENDIAN
    uint16_t netshort=0;
    netshort = (hostshort & 0xFF) << 8;
    netshort |= ((hostshort >> 8)& 0xFF);
    return netshort;
#else
    return hostshort;
#endif
}

/*****************************************************************************/
static inline bool is_timeout(uint32_t timeout_ms,uint32_t start_ms,uint32_t current_ms)
{
    if (start_ms <= current_ms)
    {
        return ((current_ms - start_ms) >= timeout_ms);
    }
    else
    {
        return ((0xFFFFFFFFUL - start_ms + current_ms + 1) >= timeout_ms);
    }
}
/*****************************************************************************/
#endif //__IP_UTIL_H__