/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#pragma once

#ifndef __GW_UART_H__
#define __GW_UART_H__

#include <Arduino.h>
#include "gw_eth.h"
esp_err_t gw_uart_start(void);
//void operate_gw_uart_rx(HardwareSerial * uart_dev);

#define GW_UART1     Serial1
#define GW_UART1_TX_PIN (33)
#define GW_UART1_RX_PIN (32)
#define GW_UART2     Serial2
#define GW_UART2_TX_PIN (15)
#define GW_UART2_RX_PIN (14)
#define GW_UART3     Serial3
#define GW_UART3_TX_PIN (4)
#define GW_UART3_RX_PIN (36)

#define GW_UART1_STATUS_PIN (34)
#define GW_UART2_STATUS_PIN (16)
#define GW_UART3_STATUS_PIN (2)

//#define GW_UART_SPEED  (1000000)
//#define GW_UART_SPEED  (460800)
#define GW_UART_SPEED  (230400)
//#define GW_UART_SPEED  (115200)

#define STX 0x02
#define ETX 0x03
#define DLE 0x10

#define MAX_RX_DATA_L1_LENGTH  (MAX_ETH_PACKET_LENGTH + 1)  
#define MAX_TX_DATA_L1_LENGTH  (MAX_ETH_PACKET_LENGTH + 1)  


#endif // __GW_UART_H__