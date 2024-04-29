/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#include "gw_uart.h"
#include "gw_eth.h" // read TX RING BUFFER
#include "hdr_portmap.h"
#include "hdr_arp.h"

HardwareSerial Serial3{0};

static bool is_timeout(uint32_t timeout_ms,uint32_t start_ms,uint32_t current_ms)
{
    if (start_ms <= current_ms)
    {
        return ((current_ms - start_ms) >= timeout_ms);
    }
    else
    {
        return ((0xFFFFFFFFUL - start_ms + current_ms + 1) >= timeout_ms);
    }
}

extern uint8_t eth_rxb[eth_rxb_cnt][mtu+14];
extern volatile int eth_rxb_ri;
extern volatile int eth_rxb_wi;
bool is_eth_rxb_re(void)
{
    return  (eth_rxb_ri != eth_rxb_wi);
}

static void write_byte(HardwareSerial* uart_dev, uint8_t c)
{
    switch (c)
    {
    case STX:
    case ETX:
    case DLE:
        uart_dev->write(DLE);
        break;
    }
    uart_dev->write(c);
}
static void uart_send_L0(HardwareSerial* uart_dev,const uint8_t * data, size_t size)
{
    uart_dev->write(STX);
    for (int i=0;i<(size);i++)
    {
        write_byte(uart_dev,data[i]);
    }
    uart_dev->write(ETX);
    uart_dev->flush();
}
static void SerialDiscard(HardwareSerial* uart_dev)
{
    int c   = 0x00;
    while (uart_dev->available()> 0)
    {
        c = uart_dev->read();
    }
}
static volatile uint8_t gw_status_tx1 = 0; // 0:none, 1:reay,2:not_ready
static volatile uint8_t gw_status_tx2 = 0; // 0:none, 1:reay,2:not_ready
static volatile uint8_t gw_status_tx3 = 0; // 0:none, 1:reay,2:not_ready
bool is_ready_gw_uart1(void)
{
//Serial.printf("UART1 = %d\r\n",digitalRead(GW_UART1_STATUS_PIN));
    return 0 == digitalRead(GW_UART1_STATUS_PIN);
//     uint8_t msg = 0xA0;
//     gw_status_tx1 = 0;
//     uart_send_L0(&GW_UART1,&msg,1);
//     uint32_t ts = millis();
//     while(gw_status_tx1 == 0 && !is_timeout(200,ts,millis()))
//     {
//         delay(1);
//     }
//     if (gw_status_tx1 != 1)
//     {
// //Serial.printf("UART1 nack = %u\r\n",gw_status_tx1);
//         return false;
//     }
//     return true;
}
bool is_ready_gw_uart2(void)
{
//Serial.printf("UART2 = %d\r\n",digitalRead(GW_UART2_STATUS_PIN));
    return 0 == digitalRead(GW_UART2_STATUS_PIN);
//    uint8_t msg = 0xA0;
//    gw_status_tx2 = 0;
//    uart_send_L0(&GW_UART2,&msg,1);
//    uint32_t ts = millis();
//    while(gw_status_tx2 == 0 && !is_timeout(200,ts,millis()))
//    {
//        delay(1);
//    }
//    if (gw_status_tx2 != 1)
//    {
////Serial.printf("UART2 nack = %u\r\n",gw_status_tx2);
//        return false;
//    }
//    return true;
}
bool is_ready_gw_uart3(void)
{
//Serial.printf("UART3 = %d\r\n",digitalRead(GW_UART3_STATUS_PIN));
    return 0 == digitalRead(GW_UART3_STATUS_PIN);
//    uint8_t msg = 0xA0;
//    gw_status_tx3 = 0;
//    uart_send_L0(&GW_UART3,&msg,1);
//    uint32_t ts = millis();
//    while(gw_status_tx3 == 0 && !is_timeout(200,ts,millis()))
//    {
//        delay(1);
//    }
//    if (gw_status_tx3 != 1)
//    {
////Serial.printf("UART3 nack = %u\r\n",gw_status_tx3);
//        return false;
//    }
//    return true;
}
bool is_ready_gw_uart(HardwareSerial* dev)
{
    if (dev == &GW_UART1) return is_ready_gw_uart1();
    else if (dev == &GW_UART2) return is_ready_gw_uart2();
    else if (dev == &GW_UART3) return is_ready_gw_uart3();
    else return false;

}
void gw_uart1_tx(uint8_t *ip_packet, uint32_t length)
{
    uart_send_L0(&GW_UART1,ip_packet,length);
    GW_UART1.flush();
    return;
}
void gw_uart2_tx(uint8_t *ip_packet, uint32_t length)
{
    uart_send_L0(&GW_UART2,ip_packet,length);
    GW_UART2.flush();
    return;
}
void gw_uart3_tx(uint8_t *ip_packet, uint32_t length)
{
    uart_send_L0(&GW_UART3,ip_packet,length);
    GW_UART3.flush();
    return;
}
static void task_gw_uart_tx(void* arg)
{
    uint8_t cntr = 0;
    while (true)
    {
        if (is_eth_rxb_re())
        {
            void (*tx_func)(uint8_t *, uint32_t ) = NULL;
            volatile int nxt = (eth_rxb_ri + 1) % eth_rxb_cnt;
            uint16_t ip_pkt_sz = ((uint16_t)(eth_rxb[nxt][14+2]) << 8) + (uint16_t)eth_rxb[nxt][14+3]; 
            uint16_t ip_hdr_sz = (uint16_t)(eth_rxb[nxt][14+0] & 0x0F) * 4;
            uint8_t protocol = eth_rxb[nxt][14+9]; 
            uint32_t src_ip = *(uint32_t*)&eth_rxb[nxt][14+12]; 
            uint32_t dst_ip = *(uint32_t*)&eth_rxb[nxt][14+16]; 
// for (int i = 0; i < ip_pkt_sz; i++)
// {
//     if (i % 16 == 0) Serial.printf("\r\n");
//     Serial.printf("%02x ",eth_rxb[nxt][14 + i]);
// 
// }
// Serial.printf("\r\n");
            if (protocol == 0x06) // TCP
            {
                uint16_t e_port = ((uint16_t)(eth_rxb[nxt][14+ip_hdr_sz+0]) << 8) + (uint16_t)eth_rxb[nxt][14+ip_hdr_sz+1]; 
                pmap_row_t* row = search_pmap_for_tcp(src_ip,dst_ip,e_port);
                if (row == NULL)
                {
                    //HardwareSerial* uart = &GW_UART1;
                    //tx_func = gw_uart1_tx;
                    //row = add_pmap(protocol,src_ip,e_port,dst_ip, lte_port_genarator(),uart);
                    //cntr = (row->uart == &GW_UART2)  ? 1 : 0;
                    HardwareSerial* uart = (cntr % 3 == 0)  ? &GW_UART3 : (cntr % 3 == 2) ? &GW_UART2 : &GW_UART1;
                    if (!is_ready_gw_uart(uart))
                    {
                        uart = (&GW_UART3 == uart)  ? &GW_UART1 : (&GW_UART2 == uart) ? &GW_UART3 : &GW_UART2;
                        if (!is_ready_gw_uart(uart))
                        {
                            uart = (&GW_UART3 == uart)  ? &GW_UART1 : (&GW_UART2 == uart) ? &GW_UART3 : &GW_UART2;
                            if (!is_ready_gw_uart(uart))
                            {
//Serial.printf("TCP PACKET IGNORED (ALL UART DISABLED)\r\n");
                                goto NEXT_BUFFER;
                            }
                        }
                        uart =  (&GW_UART2 == uart) ?  &GW_UART3 :(&GW_UART1 == uart)  ? &GW_UART2 : &GW_UART1; 
                    }
                    else
                    {
                        cntr++;
                    }
                    tx_func = (&GW_UART1 == uart)  ? gw_uart1_tx : (&GW_UART2 == uart) ? gw_uart2_tx : gw_uart3_tx;
                    row = add_pmap(protocol,src_ip,e_port,dst_ip, lte_port_genarator(),uart);
                    cntr = (row->uart == &GW_UART3)  ? 2 :(row->uart == &GW_UART2)  ? 1 : 0;
                }
                else
                {
                    HardwareSerial* uart = row->uart;
                    tx_func = (&GW_UART1 == uart)  ? gw_uart1_tx : (&GW_UART2 == uart) ? gw_uart2_tx : gw_uart3_tx;
                    update_pmap_by_client_info(protocol,src_ip,e_port);
                    cntr = (row->uart == &GW_UART3)  ? 2 :(row->uart == &GW_UART2)  ? 1 : 0;
                }
                if (row != NULL)
                {
//Serial.printf("[OUT] TCP PORT = %04X, %04X\r\n",row->eth_port,row->lte_port);
                    uint16_t l_port = row->lte_port;
                    eth_rxb[nxt][14+ip_hdr_sz+0] = (uint8_t)((l_port >> 8) & 0x00FFu);
                    eth_rxb[nxt][14+ip_hdr_sz+1] = (uint8_t)(l_port & 0x00FFu);
                }
            }
            else if (protocol == 0x11) // UDP
            {
                uint16_t e_port = ((uint16_t)(eth_rxb[nxt][14+ip_hdr_sz+0]) << 8) + (uint16_t)eth_rxb[nxt][14+ip_hdr_sz+1]; 
                pmap_row_t* row = search_pmap_by_client_info(protocol,src_ip,e_port);
                if (row == NULL)
                {
                    HardwareSerial* uart = (cntr % 3 == 0)  ? &GW_UART3 : (cntr % 3 == 2) ? &GW_UART2 : &GW_UART1;
                    if (!is_ready_gw_uart(uart))
                    {
                        uart = (&GW_UART3 == uart)  ? &GW_UART1 : (&GW_UART2 == uart) ? &GW_UART3 : &GW_UART2;
                        if (!is_ready_gw_uart(uart))
                        {
                            uart = (&GW_UART3 == uart)  ? &GW_UART1 : (&GW_UART2 == uart) ? &GW_UART3 : &GW_UART2;
                            if (!is_ready_gw_uart(uart))
                            {
//Serial.printf("UDP PACKET IGNORED (ALL UART DISABLED)\r\n");
                                goto NEXT_BUFFER;
                            }
                        }
                        uart =  (&GW_UART2 == uart) ?  &GW_UART3 :(&GW_UART1 == uart)  ? &GW_UART2 : &GW_UART1; 
                    }
                    else
                    {
                        cntr++;
                    }
                    tx_func = (&GW_UART1 == uart)  ? gw_uart1_tx : (&GW_UART2 == uart) ? gw_uart2_tx : gw_uart3_tx;
                    row = add_pmap(protocol,src_ip,e_port,dst_ip, lte_port_genarator(),uart);
                    cntr = (row->uart == &GW_UART3)  ? 2 :(row->uart == &GW_UART2)  ? 1 : 0;
                }
                else
                {
                    HardwareSerial* uart = (cntr % 3 == 0)  ? &GW_UART3 : (cntr % 3 == 2) ? &GW_UART2 : &GW_UART1;
                    if (!is_ready_gw_uart(uart))
                    {
                        uart = (&GW_UART3 == uart)  ? &GW_UART1 : (&GW_UART2 == uart) ? &GW_UART3 : &GW_UART2;
                        if (!is_ready_gw_uart(uart))
                        {
                            uart = (&GW_UART3 == uart)  ? &GW_UART1 : (&GW_UART2 == uart) ? &GW_UART3 : &GW_UART2;
                            if (!is_ready_gw_uart(uart))
                            {
//Serial.printf("UDP PACKET IGNORED (ALL UART DISABLED)\r\n");
                                goto NEXT_BUFFER;
                            }
                        }
                        uart =  (&GW_UART2 == uart) ?  &GW_UART3 :(&GW_UART1 == uart)  ? &GW_UART2 : &GW_UART1; 
                    }
                    else
                    {
                        cntr++;
                    }
                    tx_func = (&GW_UART1 == uart)  ? gw_uart1_tx : (&GW_UART2 == uart) ? gw_uart2_tx : gw_uart3_tx;
                    row->uart = uart; // CHANGE UART DEVICE
                    update_pmap_by_client_info(protocol,src_ip,e_port);
                    cntr = (row->uart == &GW_UART3)  ? 2 :(row->uart == &GW_UART2)  ? 1 : 0;
                }
                if (row != NULL)
                {
                    uint16_t l_port = row->lte_port;
//Serial.printf("[OUT] UDP PORT = %04X, %04X\r\n",row->eth_port,row->lte_port);
                    eth_rxb[nxt][14+ip_hdr_sz+0] = (uint8_t)((l_port >> 8) & 0x00FFu);
                    eth_rxb[nxt][14+ip_hdr_sz+1] = (uint8_t)(l_port & 0x00FFu);
                }
            }
            else if (protocol == 0x01) // ICMP
            {
                if (eth_rxb[nxt][14+ip_hdr_sz+0] == 0 || eth_rxb[nxt][14+ip_hdr_sz+0] == 8) // ECHO ,REPLY
                {
                    uint16_t e_port = (((uint16_t)eth_rxb[nxt][14+ip_hdr_sz+4]) << 8) + (uint16_t)eth_rxb[nxt][14+ip_hdr_sz+5];
                    pmap_row_t* row = search_pmap_by_client_info(protocol,src_ip,e_port);
                    if (row == NULL)
                    {
                        //HardwareSerial* uart = &GW_UART1;
                        //tx_func = gw_uart1_tx;
                        //row = add_pmap(protocol,src_ip,e_port,dst_ip,lte_port_genarator_icmp_echo(),uart); 
                        //cntr = (row->uart == &GW_UART2)  ? 1 : 0;
                        HardwareSerial* uart = (cntr % 3 == 0)  ? &GW_UART3 : (cntr % 3 == 2) ? &GW_UART2 : &GW_UART1;
                        if (!is_ready_gw_uart(uart))
                        {
                            uart = (&GW_UART3 == uart)  ? &GW_UART1 : (&GW_UART2 == uart) ? &GW_UART3 : &GW_UART2;
                            if (!is_ready_gw_uart(uart))
                            {
                                uart = (&GW_UART3 == uart)  ? &GW_UART1 : (&GW_UART2 == uart) ? &GW_UART3 : &GW_UART2;
                                if (!is_ready_gw_uart(uart))
                                {
//Serial.printf("ICMP PACKET IGNORED (ALL UART DISABLED)\r\n");
                                    goto NEXT_BUFFER;
                                }
                            }
                            uart =  (&GW_UART2 == uart) ?  &GW_UART3 :(&GW_UART1 == uart)  ? &GW_UART2 : &GW_UART1; 
                        }
                        else
                        {
                            cntr++;
                        }
                        tx_func = (&GW_UART1 == uart)  ? gw_uart1_tx : (&GW_UART2 == uart) ? gw_uart2_tx : gw_uart3_tx;
                        row = add_pmap(protocol,src_ip,e_port,dst_ip,lte_port_genarator_icmp_echo(),uart); 
                        cntr = (row->uart == &GW_UART3)  ? 2 :(row->uart == &GW_UART2)  ? 1 : 0;
                    }
                    else
                    {
                        HardwareSerial* uart = (cntr % 3 == 0)  ? &GW_UART3 : (cntr % 3 == 2) ? &GW_UART2 : &GW_UART1;
                        if (!is_ready_gw_uart(uart))
                        {
                            uart = (&GW_UART3 == uart)  ? &GW_UART1 : (&GW_UART2 == uart) ? &GW_UART3 : &GW_UART2;
                            if (!is_ready_gw_uart(uart))
                            {
                                uart = (&GW_UART3 == uart)  ? &GW_UART1 : (&GW_UART2 == uart) ? &GW_UART3 : &GW_UART2;
                                if (!is_ready_gw_uart(uart))
                                {
//Serial.printf("ICMP PACKET IGNORED (ALL UART DISABLED)\r\n");
                                    goto NEXT_BUFFER;
                                }
                            }
                            uart =  (&GW_UART2 == uart) ?  &GW_UART3 :(&GW_UART1 == uart)  ? &GW_UART2 : &GW_UART1; 
                        }
                        else
                        {
                            cntr++;
                        }
                        tx_func = (&GW_UART1 == uart)  ? gw_uart1_tx : (&GW_UART2 == uart) ? gw_uart2_tx : gw_uart3_tx;
                        row->uart = uart; // CHANGE UART DEVICE
                        update_pmap_by_client_info(protocol,src_ip,e_port);
                        cntr = (row->uart == &GW_UART3)  ? 2 :(row->uart == &GW_UART2)  ? 1 : 0;
                    }
                    if (row != NULL)
                    {
                        uint16_t l_port = row->lte_port;
//Serial.printf("[OUT] ICMP ID = %04X, %04X\r\n",row->eth_port,row->lte_port);

                        eth_rxb[nxt][14+ip_hdr_sz+4] = (uint8_t)((l_port >> 8) & 0x00FFu);
                        eth_rxb[nxt][14+ip_hdr_sz+5] = (uint8_t)(l_port & 0x00FFu);
                    }
                }
            }
            if (tx_func)
            {
//Serial.printf("SEND FF (%d)\r\n", (tx_func == gw_uart1_tx) ? 1 : 2);
//Serial.printf("[OUT] ---");
//for (int i=0;i<ip_pkt_sz;i++)
//{
//    if (i % 16 == 0) Serial.printf("\r\n");
//    Serial.printf("%02x " ,eth_rxb[nxt][14+i]);
//
//}
//Serial.printf("\r\n");
                //eth_rxb[nxt][14-1] = 0xFF;
//Serial.printf("send by uart[%s]\r\n", (tx_func == gw_uart1_tx) ? "1" : "2");
                //tx_func(&eth_rxb[nxt][14 -1],ip_pkt_sz+1);
                tx_func(&eth_rxb[nxt][14],ip_pkt_sz);
            }
NEXT_BUFFER:
            update_arp_cache_by_ip(src_ip, &eth_rxb[nxt][6]);
            eth_rxb_ri = nxt;
        }
        delay(1);
    }
}

static bool eth_tx_lock = false;
static uint16_t uart1_rxb_index = 0;
static uint16_t uart2_rxb_index = 0;
static uint16_t uart3_rxb_index = 0;
static bool uart1_rx_in_frame	= false;
static bool uart1_rx_in_escape	= false;
static bool uart2_rx_in_frame	= false;
static bool uart2_rx_in_escape	= false;
static bool uart3_rx_in_frame	= false;
static bool uart3_rx_in_escape	= false;
static uint8_t uart1_rxb[MAX_RX_DATA_L1_LENGTH];
static uint8_t uart2_rxb[MAX_RX_DATA_L1_LENGTH];
static uint8_t uart3_rxb[MAX_RX_DATA_L1_LENGTH];
static void operate_gw_uart_rx(HardwareSerial * uart_dev)
{
    int c	    = 0x00;
    size_t sz	= 0;
    bool res	= true;
    uint16_t *rxb_idx  = (uart_dev == &GW_UART1) ? &uart1_rxb_index    : ((uart_dev == &GW_UART2) ? &uart2_rxb_index    : &uart3_rxb_index   );
    bool *rx_in_frame  = (uart_dev == &GW_UART1) ? &uart1_rx_in_frame  : ((uart_dev == &GW_UART2) ? &uart2_rx_in_frame  : &uart3_rx_in_frame );
    bool *rx_in_escape = (uart_dev == &GW_UART1) ? &uart1_rx_in_escape : ((uart_dev == &GW_UART2) ? &uart2_rx_in_escape : &uart3_rx_in_escape);
    uint8_t *uart_rxb  = (uart_dev == &GW_UART1) ? uart1_rxb           : ((uart_dev == &GW_UART2) ? uart2_rxb           : uart3_rxb          ); 

    *rxb_idx = 0;
    while (true)
    {
        if (uart_dev->available() <= 0)
        {
            delay(1);
            continue;
        }


        c = uart_dev->read();
        if (c < 0)
        {
            delay(1);
            continue;
        }

//Serial.printf("recv=[%02X]\r\n", c);

        //---------------------------------------------------
        // L0 DATAFRAME OPERATION
        if (!*rx_in_frame)
        {
            switch (c)
            {
            case STX:
                *rxb_idx = 0;
                *rx_in_frame = true;
                continue;
            default:
                // L0 frame-error ---> ignore
                continue;
            }
        }
        else if (*rx_in_escape)
        {
            switch (c)
            {
            case STX:
            case ETX:
            case DLE:
                // L1 data
                *rx_in_escape = false;
                *rx_in_frame = true;
                break;
            default:
                // L0 frame error
                *rx_in_escape = false;
                *rx_in_frame = false;
                //Serial.printf("operate_gw_uart_rx frame_error 1\r\n");
                continue;
            }
        }
        else // if (!rx_in_escape)
        {
            switch (c)
            {
            case STX:
                // L0 frame error
                *rx_in_escape = false;
                *rx_in_frame = false;
                //Serial.printf("operate_gw_uart_rx frame_error 2\r\n");
                continue;
            case ETX:
                // L0 end of frame
                //Serial.print("[ETX]");
                *rx_in_frame = false;
                if (MAX_RX_DATA_L1_LENGTH <= *rxb_idx)
                {
                    *rx_in_escape = false;
                    *rx_in_frame = false;
                    //Serial.printf("operate_gw_uart_rx frame_error 3\r\n");
                    continue;
                }
                break;;
            case DLE:
                // L0 start escape
                //Serial.print("[esc]");
                *rx_in_escape = true;
                continue;
            default:
                // L1 data
                break;
            }
        }
        if (*rx_in_frame) {
            uart_rxb[*rxb_idx] = c;
            (*rxb_idx)++; 
            if (*rxb_idx >= MAX_RX_DATA_L1_LENGTH)
            {
                *rx_in_escape = false;
                *rx_in_frame = false;
                //Serial.printf("operate_gw_uart_rx frame_error 4\r\n");
                SerialDiscard(uart_dev);
                continue;
            }
        }
        else
        {
//for (int i = 0; i < *rxb_idx; i++)
//{
//    if (i % 16 == 0) Serial.printf("\r\n");
//    Serial.printf("%02x ",uart_rxb[i]);
//
//}
//Serial.printf("\r\n");
//            if (*rxb_idx == 1 && uart_rxb[0] == 0xA1)
//            {
//                if (uart_dev == &GW_UART1) gw_status_tx1 = 1;
//                else if (uart_dev == &GW_UART2) gw_status_tx2 = 1;
//                else if (uart_dev == &GW_UART3) gw_status_tx3 = 1;
////Serial.printf("0xA1\r\n");
//                *rx_in_escape = false;
//                *rx_in_frame = false;
//                *rxb_idx = 0;
//            }
//            else if (*rxb_idx == 1 && uart_rxb[0] == 0xA2)
//            {
//                if (uart_dev == &GW_UART1) gw_status_tx1 = 2;
//                else if (uart_dev == &GW_UART2) gw_status_tx2 = 2;
//                else if (uart_dev == &GW_UART3) gw_status_tx3 = 2;
////Serial.printf("0xA2\r\n");
//                *rx_in_escape = false;
//                *rx_in_frame = false;
//                *rxb_idx = 0;
//            }
//            else if ((uart_rxb[0] == 0xFF) && ((*rxb_idx - 1) > 20) &&  ( (uart_rxb[1+9]  ==  0x01) || (uart_rxb[1+9]  ==  0x06) || (uart_rxb[1+9]  ==  0x11)))
            if ( ((*rxb_idx) > 20) &&  ( (uart_rxb[9]  ==  0x01) || (uart_rxb[9]  ==  0x06) || (uart_rxb[9]  ==  0x11)))
            {
                uint16_t ip_pkt_sz = ((uint16_t)uart_rxb[2] << 8) + (uint16_t)uart_rxb[3];
//Serial.printf("sz = %u, %u\r\n",ip_pkt_sz,*rxb_idx);
                if (*rxb_idx == ip_pkt_sz)
                {
                    while (eth_tx_lock) {delay(10);};
                    eth_tx_lock = true;
                    forward_downlink_packet(uart_rxb,*rxb_idx);
                    eth_tx_lock = false;
                    *rx_in_escape = false;
                    *rx_in_frame = false;
                    *rxb_idx = 0;
                }
            }
            *rx_in_escape = false;
            *rx_in_frame = false;
            *rxb_idx = 0;
        }
    }   // while
}
static void task_gw_uart1_rx(void* arg)
{
    uint8_t cntr = 0;
    while (true)
    {
        operate_gw_uart_rx(&GW_UART1);
        delay(1);
    }
}
static void task_gw_uart2_rx(void* arg)
{
    uint8_t cntr = 0;
    while (true)
    {
        operate_gw_uart_rx(&GW_UART2);
        delay(1);
    }
}
static void task_gw_uart3_rx(void* arg)
{
    uint8_t cntr = 0;
    while (true)
    {
        operate_gw_uart_rx(&GW_UART3);
        delay(1);
    }
}
esp_err_t gw_uart_start(void)
{
    esp_err_t err = ESP_OK;
    pinMode(GW_UART1_STATUS_PIN,INPUT_PULLUP);
    pinMode(GW_UART2_STATUS_PIN,INPUT_PULLUP);
    pinMode(GW_UART3_STATUS_PIN,INPUT_PULLUP);
delay(500);
Serial.printf("UART1 %d\r\n",digitalRead(GW_UART1_STATUS_PIN));
Serial.printf("UART2 %d\r\n",digitalRead(GW_UART2_STATUS_PIN));
Serial.printf("UART3 %d\r\n",digitalRead(GW_UART3_STATUS_PIN));
Serial.flush();
delay(500);
    Serial.end();
    GW_UART1.begin(GW_UART_SPEED,SERIAL_8N1,GW_UART1_RX_PIN,GW_UART1_TX_PIN);
    GW_UART2.begin(GW_UART_SPEED,SERIAL_8N1,GW_UART2_RX_PIN,GW_UART2_TX_PIN);
    GW_UART3.begin(GW_UART_SPEED,SERIAL_8N1,GW_UART3_RX_PIN,GW_UART3_TX_PIN);
    xTaskCreatePinnedToCore(task_gw_uart_tx ,"task_gw_uart_tx" , 2048, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(task_gw_uart1_rx,"task_gw_uart1_rx", 2048, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(task_gw_uart2_rx,"task_gw_uart2_rx", 2048, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(task_gw_uart3_rx,"task_gw_uart3_rx", 2048, NULL, 2, NULL, 1);
    return err;
}