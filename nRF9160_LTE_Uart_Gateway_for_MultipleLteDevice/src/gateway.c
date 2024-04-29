/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <modem/modem_info.h>
#include <zephyr/posix/poll.h>

#include <zephyr/net/socket.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/sys/socket.h>

#include "nrf91/nrf91_lte.h"
#include "main.h"
#include "hdr_uart.h"
#include "ip_util.h"

LOG_MODULE_REGISTER(gateway, CONFIG_NRF91_LOG_LEVEL);

K_MSGQ_DEFINE(uart_cmd_msgq, 1, 8, 1);
K_MSGQ_DEFINE(uart_pkt_msgq, NRF9160_MTU, 8, 128);
K_MSGQ_DEFINE(lte_msgq     , NRF9160_MTU, 8, 128);

extern const struct device *gw_uart_dev;

#define RX_THREAD_STACK_SIZE    KB(4)
#define TX_THREAD_STACK_SIZE    KB(4)
#define THREAD_PRIORITY     K_LOWEST_APPLICATION_THREAD_PRIO
//#define THREAD_PRIORITY     CONFIG_NUM_PREEMPT_PRIORITIES
//#define THREAD_PRIORITY     K_HIGHEST_APPLICATION_THREAD_PRIO

static struct k_thread lte_rx_thread;
static K_THREAD_STACK_DEFINE(lte_rx_thread_stack, RX_THREAD_STACK_SIZE);
static k_tid_t lte_rx_thread_id;

static struct k_thread lte_tx_thread;
static K_THREAD_STACK_DEFINE(lte_tx_thread_stack, TX_THREAD_STACK_SIZE);
static k_tid_t lte_tx_thread_id;


extern uint8_t my_addr[4];
//static volatile bool raw_socket_lock = false;
static volatile int raw_socket = -1;
//static volatile bool lock_lte_txb = false;

static volatile bool on_req_gw_thread_terminate = false;
static volatile bool on_lte_rx_thread_terminate = false;
static volatile bool on_lte_tx_thread_terminate = false;

////////////////////////////////////////////////////////////////////////////////
static uint8_t uart_txb[NRF9160_MTU];
static uint8_t lte_txb[NRF9160_MTU];
static void gw_lte_tx_thread(void *p1, void *p2, void *p3)
{
    on_lte_tx_thread_terminate = false;
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    while (!on_req_gw_thread_terminate)
    {
        while (k_msgq_get(&uart_pkt_msgq, &uart_txb, K_NO_WAIT) == 0)
        {
            uint16_t pkt_sz = ((uint16_t)uart_txb[2] << 8) + (uint16_t)uart_txb[3];
            uart_poll_out(gw_uart_dev,STX);
            for (int i=0;i<(pkt_sz);i++)
            {
                gw_write_frame_byte(uart_txb[i]);
            }
            uart_poll_out(gw_uart_dev,ETX);
//printk("++ ETH SEND ++\r\n");
//for (int i = 0; i < MIN(1280,pkt_sz) ; i++)
//{
//    if (i % 16 == 0) printk("\r\n");
//    printk("%02x ",uart_txb[i]);
//
//}
//printk("\r\n++++++++++\r\n");
        }
        while (k_msgq_get(&lte_msgq, &lte_txb, K_NO_WAIT) == 0)
        {
            if (is_cgev_act() && !is_cgev_detach() && raw_socket != -1)
            {
                uint16_t pkt_sz = ((uint16_t)lte_txb[2] << 8) + (uint16_t)lte_txb[3];
//printk("XX LTE SEND XX\r\n");
//for (int i = 0; i < MIN(1280,pkt_sz) ; i++)
//{
//    if (i % 16 == 0) printk("\r\n");
//    printk("%02x ",lte_txb[i]);
//
//}
//printk("\r\nXXXXXXX\r\n");
                ssize_t sz = send(raw_socket,lte_txb,pkt_sz,0);
//printk("lte send %d / %u\r\n",sz, pkt_sz);
                if (sz <= 0)
                {
                    LOG_WRN("send err (%d) (%d)",-errno,sz);
                }
            }
        }
        k_sleep(K_MSEC(100));
    }
    on_lte_tx_thread_terminate = true;
}

static volatile bool gw_l0_rx_in_frame = false;
static volatile bool gw_l0_rx_in_escape = false;
static volatile uint16_t gw_packet_sz = 0;
static uint8_t gw_packet[GW_MAX_PACKET_SIZE];
void gw_uart_serial_cb(const struct device *dev, void *user_data)
{
	if (!uart_irq_update(gw_uart_dev))
    {
		return;
	}
    if (!uart_irq_rx_ready(gw_uart_dev))
    {
        return;
    }
    uint8_t c;
    while (1 == uart_fifo_read(gw_uart_dev,&c,1))
    {
        //if (0 != uart_poll_in(gw_uart_dev,&c))
        //{
        //    break;
        //}
        // L0 DATAFRAME OPERATION
        if (!gw_l0_rx_in_frame)
        {
//printk("-- L0 %02X\r\n",c);
            switch (c)
            {
            case 0x00:
                continue;
            case STX:
                gw_packet_sz = 0;
                gw_l0_rx_in_frame = true;
                continue;
            default:
                // L0 frame-error ---> ignore
                continue;
            }
        }
        if (gw_l0_rx_in_escape)
        {
            switch (c)
            {
            case STX:
            case ETX:
            case DLE:
//printk("-- L0 ESC %02X\r\n",c);
                gw_l0_rx_in_escape = false;
                gw_l0_rx_in_frame  = true;
                break;
            default:
printk("-- L0 ESC ERROR %02X\r\n",c);
                // L0 frame error  --> send-nack, igrone current-frame
                gw_l0_rx_in_escape = false;
                gw_l0_rx_in_frame = false;
                compiler_barrier();
                continue;
                break;
            }
        }
        else // if (!rx_in_escape)
        {
            switch (c)
            {
            case STX:
                // L0 frame error  --> send-nack, igrone current-frame
                gw_l0_rx_in_escape = false;
                gw_l0_rx_in_frame = false;
printk("-- L0 FRAME ERROR %02X\r\n",c);
                continue;
            case ETX:
                // L0 end of frame
                //Serial.print("[ETX]");
                gw_l0_rx_in_frame = false;
                break;;
            case DLE:
                // L0 start escape
                gw_l0_rx_in_escape = true;
                continue;
            default:
                // L1 data
                break;
            }
        }

        // L1 PACKET OPERATION
        if (!gw_l0_rx_in_frame && ETX == c)
        {
//printk("uart_pkt[0] = %02X \r\n",gw_packet[0]);
            compiler_barrier();
            gw_l0_rx_in_escape = false;
            gw_l0_rx_in_frame = false;
            compiler_barrier();
            if (*(uint32_t*)my_addr == 0 || !is_cgev_act() || is_cgev_detach())
            {
                continue;
            }
//printk("XXXXXXX\r\n");
//for (int i = 0; i < gw_packet_sz - 1; i++)
//{
//    if (i % 16 == 0) printk("\r\n");
//    printk("%02x ",gw_packet[1+i]);
//
//}
//printk("\r\nXXXXXXX\r\n");
            uint16_t ip_pkt_len = ((uint16_t)gw_packet[2] << 8) + ((uint16_t)gw_packet[3]);
            if (ip_pkt_len != gw_packet_sz)
            {
                continue;
            }
            uint16_t ip_hdr_len = (gw_packet[0] & 0x0F) * 4;


            memcpy(&gw_packet[12],my_addr,4);
            calc_ics(&gw_packet[0],ip_hdr_len,10);

            bool is_udp = gw_packet[9] == IPPROTO_UDP;
            bool is_tcp = gw_packet[9] == IPPROTO_TCP;
            if (is_tcp || is_udp)
            {
                gw_packet[ip_hdr_len + (is_udp ? 6 : 16)] = 0;
                gw_packet[ip_hdr_len + (is_udp ? 7 : 17)] = 0;
                pseudo_ip_hdr_t ip_hdr;
                memset(&ip_hdr,0,sizeof(ip_hdr));
                memcpy(ip_hdr.ip_src,&gw_packet[12],4);
                memcpy(ip_hdr.ip_dst,&gw_packet[16],4);
                ip_hdr.ip_p   = gw_packet[9];
                ip_hdr.ip_len = ip_htons(ip_pkt_len - ip_hdr_len);
                uint8_t * p_data = &gw_packet[ip_hdr_len];
                uint16_t cs = ip_checksum2((uint8_t *)&ip_hdr, sizeof(pseudo_ip_hdr_t), p_data, ip_pkt_len - ip_hdr_len);
                gw_packet[ip_hdr_len + (is_udp ? 7 : 17)] = (uint8_t)((cs >> 8) & 0xFF);
                gw_packet[ip_hdr_len + (is_udp ? 6 : 16)] = (uint8_t)(cs & 0xFF);
                //printk("CS = %04x\r\n",ip_checksum2((uint8_t *)&ip_hdr, sizeof(pseudo_ip_hdr_t), p_data, ip_pkt_len - ip_hdr_len));
            }
            else
            {
                calc_ics(&gw_packet[ip_hdr_len],ip_pkt_len - ip_hdr_len,2);
            }

            compiler_barrier();
            k_msgq_put(&lte_msgq, &gw_packet[0], K_NO_WAIT);
            compiler_barrier();
            gw_packet_sz = 0;
        }
        else
        {
            gw_packet[gw_packet_sz++] = c;
        }
    }// while
    compiler_barrier();
    compiler_barrier();
    compiler_barrier();
}
//static void gw_uart_rx_thread(void *p1, void *p2, void *p3)
//{
//    on_uart_rx_thread_terminate = false;
//    ARG_UNUSED(p1);
//    ARG_UNUSED(p2);
//    ARG_UNUSED(p3);
//    while (!on_req_gw_thread_terminate)
//    {
//        k_sleep(K_MSEC(1));
//    }
//}
extern void status_pin_on(void);
extern void status_pin_off(void);
static char rx_data[NRF9160_MTU];
static void gw_lte_rx_thread(void *p1, void *p2, void *p3)
{
    int ret = 0;
    static struct pollfd fds[1];

    uart_irq_callback_user_data_set(gw_uart_dev, gw_uart_serial_cb, NULL);
    uart_irq_rx_enable(gw_uart_dev);    

    on_lte_rx_thread_terminate = false;


    while(!on_req_gw_thread_terminate)
    {
        if (raw_socket >= 0) {
            (void)close(raw_socket);
            raw_socket = -1;
        }
        if (*(uint32_t*)my_addr == 0 || !is_cgev_act() || is_cgev_detach())
        {
            status_pin_off(); 
            k_sleep(K_MSEC(500));
            continue;
        }
        status_pin_on(); 
        /* Open socket */
        raw_socket = socket(AF_PACKET, SOCK_RAW, 0);
        if (raw_socket < 0) {
            LOG_ERR("socket(AF_PACKET, SOCK_RAW, 0) failed: %d", -errno);
            k_sleep(K_MSEC(500));
            continue;
        }

        while(!on_req_gw_thread_terminate)
        {
            if (*(uint32_t*)my_addr == 0 || !is_cgev_act() || is_cgev_detach())
            {
                k_sleep(K_MSEC(1000));
                break;
            }
            errno = 0;
            fds[0].fd = raw_socket;
            fds[0].events = POLLIN;
//printk(".");
            //ret = poll(&fds[0], 1, 500 * 1000*1000);//MSEC_PER_SEC * UDP_POLL_TIME);
            ret = poll(fds, 1, 100);//MSEC_PER_SEC * UDP_POLL_TIME);
            if (ret == 0) {  /* timeout */
                k_sleep(K_MSEC(1));
                continue;
            }
            else if (ret < 0) {  /* IO error */
                LOG_WRN("poll() error: (%d) (%d)", -errno, ret);
                break;
            }
            else if (ret > NRF9160_MTU)
            {
                LOG_INF("poll() : recv size %d > %u", ret, NRF9160_MTU);
                k_sleep(K_MSEC(1));
                continue;
            }
            /* Receive data */
//printk("[ R E C V ]\r\n");
            ret = recv(raw_socket, (void *)rx_data, sizeof(rx_data), 0);
            if (ret <= 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    LOG_ERR( "recv() failed with errno (%d) and return value (%d)", -errno, ret);
                }
                break;
            }
            compiler_barrier();
//printk("[ R E C V ] -> FORWARD\r\n");
            k_msgq_put(&uart_pkt_msgq, rx_data, K_NO_WAIT);
            compiler_barrier();
            //gw_uart_send_packet(rx_data,ret);

            k_sleep(K_MSEC(1));
        }
        uart_irq_rx_disable(gw_uart_dev);    
        k_sleep(K_MSEC(1));
    }
    if (raw_socket != -1) {
        (void)close(raw_socket);
        raw_socket = -1;
    }
    LOG_INF("GW thread terminated");
    on_lte_rx_thread_terminate = true;
}
int gw_start(void)
{
    on_req_gw_thread_terminate = false;    
    raw_socket = -1;
    lte_rx_thread_id = k_thread_create(&lte_rx_thread, lte_rx_thread_stack,
            K_THREAD_STACK_SIZEOF(lte_rx_thread_stack),
            gw_lte_rx_thread, NULL, NULL, NULL,
            THREAD_PRIORITY, K_USER, K_NO_WAIT);
    lte_tx_thread_id = k_thread_create(&lte_tx_thread, lte_tx_thread_stack,
            K_THREAD_STACK_SIZEOF(lte_tx_thread_stack),
            gw_lte_tx_thread, NULL, NULL, NULL,
            THREAD_PRIORITY, K_USER, K_NO_WAIT);
    return 0;
}