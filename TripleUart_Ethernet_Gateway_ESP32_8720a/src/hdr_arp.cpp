/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#include "hdr_arp.h"
#include "if_eth.h"
#include "ip_util.h"

arp_cache_tbl_t arp_cache;
static void __gc_arp_cache(uint32_t timeout_ms)
{
    for (int i = 0; i < MAX_ARP_CACHE_ROWS; i++)
    {
        if (arp_cache.rows[i].ip == 0) continue;
        if (is_timeout(timeout_ms,arp_cache.rows[i].ts,millis()))
        {
//Serial.printf("GC\r\n");
            arp_cache.rows[i].ip = 0;
        }
    }
}
arp_cache_row_t* search_arp_cache_empty(void)
{
    {
        __gc_arp_cache(ARP_CACHE_TIMEOUT_MS);
        for (int i = 0; i < MAX_ARP_CACHE_ROWS; i++)
        {
            if (arp_cache.rows[i].ip == 0) return &arp_cache.rows[i];
        }
    }
    return NULL;
}
arp_cache_row_t* search_arp_cache_by_ip(uint32_t tgt_ip)
{
    {
        __gc_arp_cache(ARP_CACHE_TIMEOUT_MS);
        for (int i = 0; i < MAX_ARP_CACHE_ROWS; i++)
        {
            uint32_t ip = arp_cache.rows[i].ip;
            if (ip == 0) continue;
            if (ip == tgt_ip) 
            {
//Serial.printf("HIT\r\n");
                return &arp_cache.rows[i];
            }
        }
    }
    return NULL;
}
arp_cache_row_t* update_arp_cache_by_ip(uint32_t tgt_ip, uint8_t* mac)
{
    {
        __gc_arp_cache(ARP_CACHE_TIMEOUT_MS);
        arp_cache_row_t* empty = NULL;
        for (int i = 0; i < MAX_ARP_CACHE_ROWS; i++)
        {
            uint32_t ip = arp_cache.rows[i].ip;
            if (ip == 0)
            {
                if (!empty) empty = &arp_cache.rows[i];
                continue;
            }
            if (ip == tgt_ip)
            {
//Serial.printf("update %02x:%02x:%02x:%02x:%02x:%02x\r\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
                memcpy(arp_cache.rows[i].mac,mac,6);
                arp_cache.rows[i].ts = millis();
                return &arp_cache.rows[i];
            }
        }
        if (empty)
        {
//Serial.printf("add %02x:%02x:%02x:%02x:%02x:%02x\r\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
            memcpy(empty->mac,mac,6);
            empty->ts = millis();
            empty->ip = tgt_ip;
            return empty;
        }
    }
    return NULL;
}
//static volatile bool lock_resolve_arp = false;
esp_err_t resolve_arp(uint32_t tgt_ip, uint8_t *mac, uint32_t timeout_ms, bool use_arp_cache = true)
{
    uint32_t ts = (uint32_t)millis();
    static pkt_arp_t pkt_arp_req;
    esp_err_t err = ESP_ERR_TIMEOUT;

    if (!ETH_isConnected())
    {
        return ESP_ERR_TIMEOUT;
    }
    if (use_arp_cache)
    {
        arp_cache_row_t* r = search_arp_cache_by_ip(tgt_ip);
        if (r)
        {
//Serial.printf("cache\r\n");
            memcpy(mac,r->mac,6);
            return ESP_OK;
        }
    }
    //while (lock_resolve_arp)
    //{
    //    if(is_timeout(timeout_ms,ts,millis()) || !ETH_isConnected())
    //    {
    //        return ESP_ERR_TIMEOUT;
    //    }
    //    delay(1);
    //};
    //lock_resolve_arp = true;
    {
        memset(&pkt_arp_req,0,sizeof(pkt_arp_req));
        memset(pkt_arp_req.eth_dst_mac,0xFF,6);
        memcpy(pkt_arp_req.eth_src_mac,my_mac_addr,6);
        pkt_arp_req.frame_type[0]   = 0x08;
        pkt_arp_req.frame_type[1]   = 0x06;
        pkt_arp_req.htype[0]        = 0x00;
        pkt_arp_req.htype[1]        = 0x01;
        pkt_arp_req.ptype[0]        = 0x08;
        pkt_arp_req.ptype[1]        = 0x00;
        pkt_arp_req.hsz[0]          = 0x06;
        pkt_arp_req.psz[0]          = 0x04;
        pkt_arp_req.operation[0]    = 0x00;
        pkt_arp_req.operation[1]    = 0x01;
        memcpy(pkt_arp_req.src_mac,my_mac_addr,6);
        memcpy(pkt_arp_req.src_ip,my_if_ip,4);
        memset(pkt_arp_req.dst_mac,0,6);
        memcpy(pkt_arp_req.dst_ip,(uint8_t*)&tgt_ip,4);
//for(int i = 0; i < sizeof(pkt_arp_req);i++)
//{
//    if (i % 16 == 0) Serial.printf("\r\n");
//    Serial.printf("%02x ",((uint8_t*)&pkt_arp_req)[i]);
//}
//Serial.printf("\r\n");
        err = esp_eth_transmit(if_eth_handle,&pkt_arp_req,sizeof(pkt_arp_req));
        if (err == ESP_OK)
        {
//Serial.printf("arp send\r\n");
            while (ETH_isConnected() && !is_timeout(timeout_ms,ts,millis()))
            {
                arp_cache_row_t* r = search_arp_cache_by_ip(tgt_ip);
                if (r)
                {
//Serial.printf("resolve %02x:%02x:%02x:%02x:%02x:%02x\r\n",r->mac[0],r->mac[1],r->mac[2],r->mac[3],r->mac[4],r->mac[5]);
                    memcpy(mac,r->mac,6);
                    return ESP_OK;
                }
                delayMicroseconds(1);
            }
        }
    }
    //lock_resolve_arp = false;
    return err;
 
}
void operate_packet_arp(uint8_t *buffer, uint32_t length)
{
//Serial.printf("a\r\n");
    static pkt_arp_t pkt_arp_ack;
    if (length < 42) return;
    if (0 != memcmp(&buffer[14+24],my_if_ip,4)) return;
    uint16_t htype = (((uint16_t)buffer[14+0]) << 8) + ((uint16_t)buffer[14+1]);
    if (htype != 0x0001) return;
    uint16_t ptype = (((uint16_t)buffer[14+2]) << 8) + ((uint16_t)buffer[14+3]);
    if (ptype != 0x0800) return;
    if (buffer[14+4] != 0x06) return; // HARD SIZE
    if (buffer[14+5] != 0x04) return; // PROTOCOL SIZE
    uint16_t operation = (((uint16_t)buffer[14+6]) << 8) + ((uint16_t)buffer[14+7]);

    if ( (operation == 0x0001) && (0 == memcmp(&buffer[0],bc_mac,6)) )
    {
    // get ARP REQUEST, send ARP RESPONCE
        // for (int i = 0; i < length; i++)
        // {
        //     if (i % 16 == 0) Serial.printf("\r\n");
        //     Serial.printf("%02x ",buffer[i]);
        // }
        memset(&pkt_arp_ack,0,sizeof(pkt_arp_ack));
        memcpy(pkt_arp_ack.eth_dst_mac,&buffer[6],6);
        memcpy(pkt_arp_ack.eth_src_mac,my_mac_addr,6);
        pkt_arp_ack.frame_type[0]   = 0x08;
        pkt_arp_ack.frame_type[1]   = 0x06;
        pkt_arp_ack.htype[0]        = 0x00;
        pkt_arp_ack.htype[1]        = 0x01;
        pkt_arp_ack.ptype[0]        = 0x08;
        pkt_arp_ack.ptype[1]        = 0x00;
        pkt_arp_ack.hsz[0]          = 0x06;
        pkt_arp_ack.psz[0]          = 0x04;
        pkt_arp_ack.operation[0]    = 0x00;
        pkt_arp_ack.operation[1]    = 0x02;
        memcpy(pkt_arp_ack.src_mac,my_mac_addr,6);
        memcpy(pkt_arp_ack.src_ip,my_if_ip,4);
        memcpy(pkt_arp_ack.dst_mac,&buffer[14+8],6);
        memcpy(pkt_arp_ack.dst_ip,&buffer[14+14],4);
        esp_err_t err = esp_eth_transmit(if_eth_handle,&pkt_arp_ack,sizeof(pkt_arp_ack));
        if (err == ESP_OK)
        {
            update_arp_cache_by_ip(*(uint32_t*)(&buffer[14+14]),&buffer[14+8]);       
        }
        // if (ESP_OK != err)
        // {
        //     Serial.printf("\r\nerror ETH TX (ARP ACK) %d\r\n",err);
        // }
        // Serial.printf("\r\nETH TX (ARP ACK) %d\r\n",err);
    }
    else if ( (operation == 0x0002) && (0 == memcmp(&buffer[0],my_mac_addr,6)) )
    {
    // get  ARP RESPONCE, update ARP CACHE
        pkt_arp_t* arp = (pkt_arp_t*)buffer;
//Serial.printf("resolve %02x:%02x:%02x:%02x:%02x:%02x\r\n",arp->src_mac[0],arp->src_mac[1],arp->src_mac[2],arp->src_mac[3],arp->src_mac[4],arp->src_mac[5]);
        update_arp_cache_by_ip(*(uint32_t*)(arp->src_ip),arp->src_mac);        
    }
}