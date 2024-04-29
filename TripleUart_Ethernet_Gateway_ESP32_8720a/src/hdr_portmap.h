/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#ifndef __HDR_PORTMAP_H__
#define __HDR_PORTMAP_H__

#include <Arduino.h>
#include <HardwareSerial.h>

#define PMAP_BASE_PORT  (32768)
#define PMAP_TIMEOUT_MS (20*1000)
#define MAX_PMAP_ROWS   (1800)

typedef struct __pmap_row_t
{
    uint8_t  ip_protocol;
    uint32_t client_ip;
    uint16_t eth_port;
    uint32_t server_ip;
    uint16_t lte_port;
    HardwareSerial* uart;
    uint32_t ts;
} pmap_row_t;

typedef struct __pmap_tbl_t
{
    pmap_row_t  row[MAX_PMAP_ROWS];
} pmap_tbl_t;

extern pmap_tbl_t pmap;

uint8_t pmap_status(void);
void gc_pmap(uint32_t timeout_ms);
pmap_row_t* search_pmap_empty(void);
pmap_row_t* search_pmap_for_tcp(uint32_t client_ip,uint32_t server_ip,uint16_t eth_port);
pmap_row_t* search_pmap_by_server_info(uint8_t ip_protocol, uint32_t server_ip,uint16_t lte_port);
pmap_row_t* search_pmap_by_client_info(uint8_t ip_protocol, uint32_t client_ip,uint16_t eth_port);
pmap_row_t* update_pmap_by_server_info(uint8_t ip_protocol, uint32_t server_ip,uint16_t lte_port);
pmap_row_t* update_pmap_by_client_info(uint8_t ip_protocol, uint32_t client_ip,uint16_t eth_port);
void remove_pmap_by_server_info(uint8_t ip_prorocol, uint32_t server_ip,uint16_t lte_port);
void remove_pmap_by_client_info(uint8_t ip_prorocol, uint32_t client_ip,uint16_t eth_port);
pmap_row_t* add_pmap(uint8_t ip_protocol, uint32_t client_ip,uint16_t eth_port,uint32_t server_ip,uint16_t lte_port,HardwareSerial* uart);

uint16_t lte_port_genarator_icmp_echo(void);
uint16_t lte_port_genarator(void);

void init_pmap(void);

#endif  // __HDR_PORTMAP_H__