/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#ifndef __MAIN_H
#define __MAIN_H
#include <zephyr/kernel.h>
#include <stdint.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
#define NRF9160_MTU    (1280)

#ifdef CONFIG_BOARD_ACTINIUS_ICARUS_SOM_NS
static const uint8_t status_pin = 31;
static const uint8_t status_led_pin = 3;
static const uint8_t btn_pin = 23;
#endif //CONFIG_BOARD_ACTINIUS_ICARUS_SOM_NS
#ifdef CONFIG_BOARD_SPARKFUN_THING_PLUS_NRF9160_NS
static const uint8_t status_pin = 13;
static const uint8_t status_led_pin = 3;
static const uint8_t btn_pin = 12;
#endif //CONFIG_BOARD_SPARKFUN_THING_PLUS_NRF9160_NS
#ifdef CONFIG_BOARD_CIRCUITDOJO_FEATHER_NRF9160_NS
static const uint8_t status_pin = 13;
static const uint8_t status_led_pin = 3;
static const uint8_t btn_pin = 12;
#endif
#ifndef CONFIG_BOARD_ACTINIUS_ICARUS_SOM_NS
#ifndef CONFIG_BOARD_SPARKFUN_THING_PLUS_NRF9160_NS
#ifndef CONFIG_BOARD_CIRCUITDOJO_FEATHER_NRF9160_NS
static const uint8_t status_pin = 3;
static const uint8_t status_led_pin = 2; // SSCI IoT Board
static const uint8_t btn_pin = 6; // SSICC IoT Board
#endif
#endif
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct  L1_PAYLOAD
{
    uint16_t size;
    uint8_t  data[NRF9160_MTU+14];
} L1_PAYLOAD_t;
typedef struct  L2_PAYLOAD
{
    uint16_t size;
    uint8_t  data[NRF9160_MTU];
} L2_PAYLOAD_t;
////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //__MAIN_H