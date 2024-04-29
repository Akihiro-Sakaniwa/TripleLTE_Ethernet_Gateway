# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nRF9160_LTE_Uart_Gateway_for_MultipleLteDevice)

target_sources(app PRIVATE src/main.c)

target_sources(app PRIVATE src/nrf91/nrf91_lte.c)
target_sources(app PRIVATE src/nrf91/nrf91_at_commands.c)
target_sources(app PRIVATE src/nrf91/nrf91_util.c)

target_sources(app PRIVATE src/gateway.c)
target_sources(app PRIVATE src/arp_cache.c)
target_sources(app PRIVATE src/pmap.c)
target_sources(app PRIVATE src/hdr_uart.c)
target_sources(app PRIVATE src/ip_util.c)
