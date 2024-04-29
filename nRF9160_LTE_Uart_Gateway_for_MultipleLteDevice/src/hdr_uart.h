/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#ifndef __HDR_UART_H
#define __HDR_UART_H

#include <zephyr/drivers/uart.h>

#define GW_MAX_ERROR_COUNT          0x03
#define GW_MAX_PACKET_SIZE          (2048)
#define GW_MAX_SEND_BUFF_COUNT      (90)

#define STX 0x02
#define ETX 0x03
#define DLE 0x10

void get_gw_uart_lock(void);
void put_gw_uart_lock(void);
//bool gw_uart_send_packet(uint8_t* packet, size_t size);
void gw_write_frame_byte(uint8_t c);

#endif //__HDR_UART_H