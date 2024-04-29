/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#include "hdr_arp.h"
#include "if_eth.h"
#include "ip_util.h"
typedef struct __pkt_icmp_echo_t
{
    uint8_t     eth_dst_mac[6];
    uint8_t     eth_src_mac[6];
    uint8_t     frame_type[2];

    uint8_t     vz[1];
    uint8_t     st[1];
    uint8_t     tz[2];
    uint8_t     id[2];
    uint8_t     fragment[2];
    uint8_t     ttl[1];
    uint8_t     protocol[1];
    uint8_t     hdr_checkcum[2];
    uint8_t     src_ip[4];
    uint8_t     dst_ip[4];


    uint8_t     icmp_type[1];
    uint8_t     icmp_code[1];
    uint8_t     icmp_checksum[2];
    uint8_t     icmp_data[1500 - 4];
} pkt_icmp_echo_t;

static pkt_icmp_echo_t pkt_icmp_echo;

void operate_packet_icmp_echo_req(uint8_t *buffer, uint32_t length)
{
    uint16_t ip_hdr_sz = (buffer[14] & 0x0F) * 4;
    if ( ip_hdr_sz != 20 ) return;
    if ( 0 != ip_checksum1(&buffer[14+ip_hdr_sz],length - (14 + ip_hdr_sz)) ) return;

    memcpy(&pkt_icmp_echo,buffer,length);

    memcpy(pkt_icmp_echo.eth_dst_mac,&buffer[6],6);
    memcpy(pkt_icmp_echo.eth_src_mac,my_mac_addr,6);
    memcpy(pkt_icmp_echo.src_ip,my_if_ip,4);
    memcpy(pkt_icmp_echo.dst_ip,&buffer[14+12],4);
    calc_checksum(&pkt_icmp_echo.vz[0],20,10);

    pkt_icmp_echo.icmp_type[0] = 0x00; // ICMP ECHOREPLY
    calc_checksum(&pkt_icmp_echo.icmp_type[0],length - 20 - 14,2);
    esp_err_t err = esp_eth_transmit(if_eth_handle,&pkt_icmp_echo,length);
}