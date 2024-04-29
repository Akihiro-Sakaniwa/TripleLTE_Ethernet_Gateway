/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/net_pkt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>

#include <modem/at_monitor.h>
#include <nrf_modem.h>
#include <nrf_errno.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_power.h>
#include <hal/nrf_regulators.h>
#include <modem/nrf_modem_lib.h>
#include <dfu/dfu_target.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/init.h>
#include <zephyr/sys/reboot.h>
#include "ncs_version.h"

#include <nrf_socket.h>
#include <zephyr/net/socket.h>

#include "nrf91/nrf91_lte.h"

#include <modem/at_monitor.h>
#include <modem/modem_info.h>

#include "nrf91/nrf91_lte.h"
#include "nrf91/nrf91_util.h"

#include "main.h"
LOG_MODULE_REGISTER(main, CONFIG_NRF91_LOG_LEVEL);

volatile bool on_req_eth_thread_terminate = false;
volatile bool on_eth_rx_thread_terminate = false;
volatile bool on_eth_tx_thread_terminate = false;
extern int gw_start(void);



const struct device *gpio_dev;
const struct device *gw_uart_dev;

extern struct nrf91_lte_ctx lte_ctx;
extern struct nrf91_http_ctx http_ctx;

extern void nrf91_lte_main(int (*cbf)(void));
extern bool nrf91_lte_thread_start(void);

volatile bool is_on_buttun_up = false;
volatile bool is_on_buttun_up_mask = false;

//////////////////////////////////////////////////////////////////////////////////////////////////////
void button_callback_handler(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
  printk("\r\n(%08x)!\r\n",pins);
  if ((BIT(btn_pin) & pins) > 0) 
  {
      if (!is_on_buttun_up_mask) is_on_buttun_up = true;
  }
}
static struct gpio_callback button_callback;
////////////////////////////////////////////////////////////////////////////////////////////////////
void error_handler(void)
{
    LOG_WRN("\r\n### POWER-OFF ###\r\n");
    while (1)
    {
        k_sleep(K_MSEC(100));
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void status_pin_on(void)
{
    gpio_pin_set(gpio_dev,status_pin,1);
}
void status_pin_off(void)
{
    gpio_pin_set(gpio_dev,status_pin,0);
}
void status_led_on(void)
{
    gpio_pin_set(gpio_dev,status_led_pin,1);
}
void status_led_off(void)
{
    gpio_pin_set(gpio_dev,status_led_pin,0);
}
int read_btn(void)
{
  return gpio_pin_get(gpio_dev, btn_pin);
}
extern uint8_t my_addr[4];
extern uint8_t uart_txb[2048];
extern volatile uint16_t uart_txb_size;
#include "hdr_uart.h"
static int cbf_main(void)
{
    k_sleep(K_MSEC(10*1000));
    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
extern void gw_uart_serial_cb(const struct device *dev, void *user_data);
int main(void)
{
    printk("%s\r\n", CONFIG_BOARD);

#ifdef CONFIG_SERIAL
    NRF_UARTE0_NS->TASKS_STOPRX = 1;
    //NRF_UARTE1_NS->TASKS_STOPRX = 1;
    //NRF_UARTE1_NS->TASKS_STOPRX = 1;
#endif

    gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0)); 
    #ifdef CONFIG_BOARD_SPARKFUN_THING_PLUS_NRF9160_NS
    gpio_pin_configure(gpio_dev, status_led_pin , GPIO_OUTPUT | GPIO_PUSH_PULL | GPIO_ACTIVE_LOW);
    gpio_pin_configure(gpio_dev, btn_pin , GPIO_INPUT  | GPIO_PULL_UP   | GPIO_ACTIVE_HIGH);
    //gpio_pin_configure(gpio_dev, buz_pin , GPIO_OUTPUT | GPIO_PUSH_PULL | GPIO_ACTIVE_HIGH);
    #else
    gpio_pin_configure(gpio_dev, status_led_pin , GPIO_OUTPUT | GPIO_PUSH_PULL | GPIO_ACTIVE_HIGH);
    gpio_pin_configure(gpio_dev, btn_pin , GPIO_INPUT  | GPIO_PULL_UP   | GPIO_ACTIVE_HIGH);
    //gpio_pin_configure(gpio_dev, buz_pin , GPIO_OUTPUT | GPIO_PUSH_PULL | GPIO_ACTIVE_HIGH);
    #endif
    gpio_pin_configure(gpio_dev, status_pin , GPIO_OUTPUT | GPIO_PUSH_PULL | GPIO_ACTIVE_LOW);
    status_pin_off();
    status_led_off();

	gw_uart_dev = DEVICE_DT_GET(DT_NODELABEL(gw_uart));
	if (!device_is_ready(gw_uart_dev)) {
		printk("GW-UART device not found!");
		return 0;
	}

    gpio_init_callback(&button_callback, button_callback_handler, BIT(btn_pin));
    gpio_add_callback(gpio_dev, &button_callback);
    gpio_pin_interrupt_configure(gpio_dev, btn_pin, GPIO_INT_EDGE_RISING);

    int ret = nrf_modem_lib_init();
    if (ret)
    {
        LOG_ERR("Modem library init failed, err: %d", ret);
        if (ret != -EAGAIN && ret != -EIO)
        {
            return ret;
        }
        else if (ret == -EIO)
        {
            LOG_ERR("Please program full modem firmware with the bootloader or external tools");
        }
    }
    int err = modem_info_init();
    if (err)
    {
        LOG_ERR("Failed to init modem_info_init: %d", err);
        return(0);
    }
    if (0 != gw_start()) return 0;
    nrf91_lte_main(cbf_main);

    return 0;
}
// EOF //