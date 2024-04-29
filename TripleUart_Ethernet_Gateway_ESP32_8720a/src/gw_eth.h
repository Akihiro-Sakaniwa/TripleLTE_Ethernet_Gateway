/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#pragma once

#ifndef __GW_ETH_H__
#define __GW_ETH_H__

#include <Arduino.h>
#include <esp_eth.h>
esp_err_t eth_rx_handler(esp_eth_handle_t hdl, uint8_t *buffer, uint32_t length, void *priv);

void forward_downlink_packet(uint8_t *buffer, uint32_t length);

#define MAX_ETH_PACKET_LENGTH   (1280+14)
static const int min_frame_sz   = (14+21);
static const int mtu            = (1280);
static const int eth_rxb_cnt    = 16;

#endif // __GW_ETH_H__