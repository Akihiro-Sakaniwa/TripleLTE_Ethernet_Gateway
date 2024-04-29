/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <modem/modem_info.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "nrf91/nrf91_lte.h"
#include "hdr_uart.h"
#include "ip_util.h"


LOG_MODULE_REGISTER(hdr_uart, CONFIG_NRF91_LOG_LEVEL);
extern const struct device *gw_uart_dev;


void gw_write_frame_byte(uint8_t c)
{
    switch (c)
    {
    case STX:
    case ETX:
    case DLE:
        uart_poll_out(gw_uart_dev,DLE);
        break;
    }
    uart_poll_out(gw_uart_dev,c);
    //k_yield();
}

static volatile bool gw_uart_lock = false;
void get_gw_uart_lock(void)
{
    __COMPILER_BARRIER();
    while (gw_uart_lock)
    {
        __COMPILER_BARRIER();
        k_sleep(K_MSEC(10));
        __COMPILER_BARRIER();
    }
    __COMPILER_BARRIER();
    __COMPILER_BARRIER();
    gw_uart_lock = true;
    __COMPILER_BARRIER();
}
void put_gw_uart_lock(void)
{
    __COMPILER_BARRIER();
    gw_uart_lock = false;
    __COMPILER_BARRIER();
}

