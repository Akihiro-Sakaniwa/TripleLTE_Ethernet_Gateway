/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#pragma once

#ifndef __IF_ETH_H__
#define __IF_ETH_H__

#include <Arduino.h>
#include <esp_eth.h>
#include <esp_eth_mac.h>
#include <esp_netif.h>
#include <ETH.h>

static const uint8_t bc_mac[]={0xff,0xff,0xff,0xff,0xff,0xff};
static const uint8_t my_if_ip[]={192,168,63,1};
static const uint8_t my_gw_ip[]={192,168,63,1};
static const uint8_t my_subnet[]={255,255,255,0};

extern uint8_t my_mac_addr[6];
extern esp_eth_mac_t *mac;
extern esp_eth_phy_t *phy;
extern esp_netif_t *eth_netif;
extern esp_eth_handle_t if_eth_handle;

const int8_t _pin_mdc = 23;
const int8_t _pin_mdio = 18;
//const int8_t _pin_power = 5; // LAN8720A
const int8_t _pin_power = 12; // ESP32-POE(-ISO)
const int8_t _pin_rmii_clock = 0; // LAN8720A,ESP32-POE(-ISO)
//const int8_t _phy_addr = 1; //LAN8720A,
const int8_t _phy_addr = 0; // ESP32-POE(-ISO)

/*
  //typedef enum { ETH_CLOCK_GPIO0_IN, ETH_CLOCK_GPIO0_OUT, ETH_CLOCK_GPIO16_OUT, ETH_CLOCK_GPIO17_OUT } eth_clock_mode_t;
  ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
  ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO0 - possibly an inverter is needed for LAN8720
  ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
  ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720
*/
//static eth_clock_mode_t eth_clock_mode = ETH_CLOCK_GPIO0_IN; // LAN8720A
static eth_clock_mode_t eth_clock_mode = ETH_CLOCK_GPIO17_OUT; // ESP32-POE(-ISO)

esp_err_t if_eth_start(void);
bool ETH_isConnected();
void ETH_waitForConnect();

#endif // __IF_ETH_H__