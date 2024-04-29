/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#include "if_eth.h"
#include "gw_eth.h"
#include "hdr_arp.h"
#include "hdr_portmap.h"
#include "hdr_icmp_echo_req.h"
#include "ip_util.h"

uint8_t eth_rxb[eth_rxb_cnt][mtu+14];
volatile int eth_rxb_ri      = eth_rxb_cnt - 1;
volatile int eth_rxb_wi      = eth_rxb_cnt - 1;
static inline bool is_eth_rxb_we(void)
{
    return  ((eth_rxb_wi + 1) % eth_rxb_cnt) != eth_rxb_ri;
}

esp_err_t eth_rx_handler(esp_eth_handle_t hdl, uint8_t *buffer, uint32_t length, void *priv)
{
    if (length >= min_frame_sz && mtu >= length)
    {
        uint16_t eth_frame_type = ((uint16_t)buffer[12] << 8) | ((uint16_t)buffer[13]);
        switch (eth_frame_type)
        {
        case 0x0806:    // ARP
//Serial.printf("\r\nRX(ARP) %d\r\n",length);
            operate_packet_arp(buffer,length);
            break;
        case 0x0800:    // IP
//Serial.printf("\r\nRX(IP) %d\r\n",length);
            uint8_t ttl = buffer[14 + 8];
            if ((ttl >= 64) && (0 == memcmp(&buffer[0],my_mac_addr,6)) && (0x04 == (buffer[14] >> 4)))
            {
                uint8_t ip_protocol = buffer[14 + 9];
                uint16_t ip_hdr_sz = (buffer[14] & 0x0F) * 4;
                if (0 == ip_checksum1(&buffer[14],ip_hdr_sz))
                {
                    if (ip_protocol == 0x01) // ICMP 
                    {
                        if (0 == memcmp(&buffer[14+16],my_if_ip,4))
                        {
                            if (buffer[14 + ip_hdr_sz + 0] == 0x08) // ECHO REQUEST
                            {
//for (int i=0;i<length;i++)
//{
//    if (i % 16 == 0) Serial.printf("\r\n");
//    Serial.printf("%02x " ,buffer[i]);
//
//}
//Serial.printf("\r\n");
                                operate_packet_icmp_echo_req(buffer,length);
                            }
                        }
                        else if (is_eth_rxb_we())  // ICMP FORWARDING
                        {
                            int nxt = (eth_rxb_wi + 1) % eth_rxb_cnt;
                            memcpy(eth_rxb[nxt],buffer,length);
//Serial.printf("\r\nRX ICMP %d\r\n",length);
                            eth_rxb_wi = nxt;
                        }
                        update_arp_cache_by_ip(*(uint32_t*)&buffer[14+12], &buffer[6]);
                    }
                    else if (ip_protocol == 0x06 && is_eth_rxb_we()) // TCP 
                    {
                        int nxt = (eth_rxb_wi + 1) % eth_rxb_cnt;
                        memcpy(eth_rxb[nxt],buffer,length);
//Serial.printf("\r\nRX TCP %d\r\n",length);
                        eth_rxb_wi = nxt;
                        update_arp_cache_by_ip(*(uint32_t*)&buffer[14+12], &buffer[6]);
                    }
                    else if (ip_protocol == 0x11 && is_eth_rxb_we()) // UDP 
                    {
                        int nxt = (eth_rxb_wi + 1) % eth_rxb_cnt;
                        memcpy(eth_rxb[nxt],buffer,length);
//Serial.printf("\r\nRX UDP %d\r\n",length);
                        eth_rxb_wi = nxt;
                        update_arp_cache_by_ip(*(uint32_t*)&buffer[14+12], &buffer[6]);
                    }
                }
            }
            break;
        }
    }
    free(buffer);
    return ESP_OK;
}

/**********************************************************************************/
// typedef struct __eth_tx_buffer_t
// {
//     bool        is_busy;
//     uint16_t    size;
//     uint8_t     eth_packet[MAX_ETH_PACKET_LENGTH];
// } eth_tx_buffer_t;
// static eth_tx_buffer_t eth_txb[4];
// void task_eth_tx(void* arg)
// {
//     uint8_t tid = *(uint8_t*)arg;
// 
//     Serial.printf("task_eth_tx[%d]: started\r\n",tid);
//     uint8_t* buf = eth_txb[tid].eth_packet;
//     uint32_t tgt_ip = *(uint32_t*)&buf[16];
//     static uint8_t mac[6];
//     esp_err_t err = resolve_arp(tgt_ip,mac,1000,true);
//     if (ESP_OK == err)
//     {
//         arp_cache_row_t* row = search_arp_cache_by_ip(tgt_ip);
//         if (row)
//         {
//             memcpy(&buf[0],row->mac,6);
//             memcpy(&buf[6],my_mac_addr,6);
//             buf[7] = 0x08;
//             buf[8] = 0x00;
//             esp_eth_transmit(if_eth_handle,buf,eth_txb[tid].size);
//         }
//         else
//         {
//             Serial.printf("task_eth_tx[%d]: resolve error %d\r\n",err);
//         }
//     }
//     else
//     {
//         Serial.printf("task_eth_tx[%d]: resolve error %d\r\n",err);
//     }
//     Serial.printf("task_eth_tx[%d]: terminated\r\n",tid);
//     eth_txb[tid].is_busy = false;
//     vTaskDelete(NULL);
// }
//static uint8_t get_free_task_id(void)
//{
//    uint8_t task_id = 0xFF; 
//    for (int i = 0; i < sizeof(eth_txb);i++)
//    {
//        if (eth_txb[i].is_busy) continue;
//        return i;
//    }
//    return 0xFF;
//}
void forward_downlink_packet(uint8_t *buffer, uint32_t length)
{
    static uint8_t eth_packet[MAX_ETH_PACKET_LENGTH];

    if (length >= (21) && mtu >= length)
    {
//Serial.printf("DOWNLINK %d\r\n",length);
        uint32_t src_ip = *(uint32_t*)&buffer[12];
        uint8_t ip_protocol = buffer[9];
        uint16_t ip_hdr_sz = (buffer[0] & 0x0F) * 4;
        uint16_t ip_pkt_len = ((uint16_t)buffer[2] << 8) + ((uint16_t)buffer[3]);
//for (int i=0;i<length;i++)
//{
//    if (i % 16 == 0) Serial.printf("\r\n");
//    Serial.printf("%02x " ,buffer[i]);
//    Serial.flush();
//
//}
//Serial.printf("\r\n"); Serial.flush();
        if ((ip_pkt_len == length) && (0 == ip_checksum1(&buffer[0],ip_hdr_sz)) )
        {
            bool is_udp = buffer[9] == 0x11;
            bool is_tcp = buffer[9] == 0x06;
            if (is_tcp || is_udp)
            {
                uint16_t port = (((uint16_t)buffer[ip_hdr_sz+2]) << 8) + (uint16_t)buffer[ip_hdr_sz+3];
                pmap_row_t* row = search_pmap_by_server_info(buffer[9],src_ip,port);
                if (row == NULL)
                {
//Serial.printf("IGNORE %d port = %u\r\n",length,port);
                    return;
                }
//Serial.printf("[IN]  %s PORT = %04X, %04X\r\n",is_udp ? "UDP":"TCP",row->eth_port,row->lte_port);

                memcpy(&buffer[16],(uint8_t *)(&row->client_ip),4);
                calc_checksum(&buffer[0],ip_hdr_sz,10);

                buffer[ip_hdr_sz+2] = (uint8_t)((row->eth_port) >> 8);
                buffer[ip_hdr_sz+3] = (uint8_t)((row->eth_port) >> 0);

                pseudo_ip_hdr_t ip_hdr;
                buffer[ip_hdr_sz + (is_udp ? 6 : 16)] = 0;
                buffer[ip_hdr_sz + (is_udp ? 7 : 17)] = 0;
                memset(&ip_hdr,0,sizeof(ip_hdr));
                memcpy(ip_hdr.ip_src,&buffer[12],4);
                memcpy(ip_hdr.ip_dst,&buffer[16],4);
                ip_hdr.ip_p   = buffer[9];
                ip_hdr.ip_len = ip_htons(ip_pkt_len - ip_hdr_sz);
                uint8_t * p_data = &buffer[ip_hdr_sz];
                uint16_t cs = ip_checksum2((uint8_t *)&ip_hdr, sizeof(pseudo_ip_hdr_t), p_data, ip_pkt_len - ip_hdr_sz);
                buffer[ip_hdr_sz + (is_udp ? 6 : 16)] = (uint8_t)(cs & 0xFF);
                buffer[ip_hdr_sz + (is_udp ? 7 : 17)] = (uint8_t)((cs >> 8) & 0xFF);
            }
            else
            {
                uint16_t port = (((uint16_t)buffer[ip_hdr_sz+4]) << 8) + (uint16_t)buffer[ip_hdr_sz+5];
                pmap_row_t *row = search_pmap_by_server_info(buffer[9],src_ip,port);
                if (row == NULL)
                {
//Serial.printf("IGNORE %d port = %u\r\n",length,port);
                    return;
                }
//Serial.printf("[IN]  ICMP ID = %04X, %04X\r\n",row->eth_port,row->lte_port);

                buffer[ip_hdr_sz+4] = (uint8_t)(row->eth_port >> 8);
                buffer[ip_hdr_sz+5] = (uint8_t)(row->eth_port & 0x00FFu);
                memcpy(&buffer[16],(uint8_t*)&row->client_ip,4);
                calc_checksum(&buffer[0],ip_hdr_sz,10);
                calc_checksum(&buffer[ip_hdr_sz],ip_pkt_len - ip_hdr_sz, 2);
            }

            uint8_t* buf = eth_packet;
            memcpy(&buf[14],&buffer[0],ip_pkt_len);
            uint32_t tgt_ip = *((uint32_t*)&buf[14+16]);
            static uint8_t mac[6];
            esp_err_t err = resolve_arp(tgt_ip,mac,100,true);
            if (ESP_OK == err)
            {
                memcpy(&buf[0],mac,6);
                memcpy(&buf[6],my_mac_addr,6);
                buf[12] = 0x08;
                buf[13] = 0x00;
// Serial.printf("[IN] --------\r\n");
// for (int i=0;i<ip_pkt_len;i++)
// {
//     if (i % 16 == 0) Serial.printf("\r\n");
//     Serial.printf("%02x " ,buf[14+i]);
//     Serial.flush();
// 
// }
// Serial.printf("\r\n"); Serial.flush();
                err = esp_eth_transmit(if_eth_handle,buf,14 + ip_pkt_len);
                if (err != ESP_OK)
                {
                    //Serial.printf("eth send error %d\r\n",err);
                }
            }
            else
            {
                //Serial.printf("resolve error %d\r\n",err);
            }
        }
    }
}
/**********************************************************************************/