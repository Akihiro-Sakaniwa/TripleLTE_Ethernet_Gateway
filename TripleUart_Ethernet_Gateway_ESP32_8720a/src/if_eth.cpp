/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#include "if_eth.h"
#include <WiFi.h>
#include "gw_eth.h"
#include "hdr_arp.h"
#include "hdr_portmap.h"

extern void tcpipInit();
extern void add_esp_interface_netif(esp_interface_t interface, esp_netif_t* esp_netif); /* from WiFiGeneric */


uint8_t my_mac_addr[6];
esp_eth_mac_t *mac;
esp_eth_phy_t *phy;
esp_netif_t *eth_netif;
esp_eth_handle_t if_eth_handle = NULL; // after the driver is installed, we will get the handle of the driver

#define ETH_ESP32_EMAC_DEFAULT_CONFIG()               \
    {                                                 \
        .smi_mdc_gpio_num = _pin_mdc,                 \
        .smi_mdio_gpio_num = _pin_mdio,               \
        .interface = EMAC_DATA_INTERFACE_RMII,        \
        .clock_config =                               \
        {                                             \
            .rmii =                                   \
            {                                         \
                .clock_mode = EMAC_CLK_DEFAULT,       \
                .clock_gpio = EMAC_CLK_IN_GPIO        \
            }                                         \
        },                                            \
        .dma_burst_len = ETH_DMA_BURST_LEN_32,        \
        .intr_priority = 0,                           \
    }

bool eth_connected = false;

void ETH_waitForConnect(void)
{
  while (!eth_connected)
    delay(100);
}
bool ETH_isConnected(void)
{
  return eth_connected;
}
static void ETH_event(WiFiEvent_t event)
{
  switch (event)
  {
#if ( ( defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2) ) && ( ARDUINO_ESP32_GIT_VER != 0x46d5afb1 ) )
    case ARDUINO_EVENT_ETH_START:
        log_i(F("\nETH Started"));
        break;

    case ARDUINO_EVENT_ETH_CONNECTED:
        log_i(F("ETH Connected"));
        break;

    case ARDUINO_EVENT_ETH_GOT_IP:
        if (!eth_connected)
        {
            esp_netif_get_mac(eth_netif, my_mac_addr);
            Serial.printf("\r\nip_address = %u.%u.%u.%u, mac_address = %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                my_if_ip[0], my_if_ip[1], my_if_ip[2], my_if_ip[3],
                my_mac_addr[0], my_mac_addr[1], my_mac_addr[2], my_mac_addr[3], my_mac_addr[4], my_mac_addr[5]);
            eth_connected = true;
        }
        break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
        log_i("ETH Disconnected");
        eth_connected = false;
        break;

    case ARDUINO_EVENT_ETH_STOP:
        log_i("\nETH Stopped");
        eth_connected = false;
        break;
#endif
    default:
      break;
  }
}
//bool ip_config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = (uint32_t)0, IPAddress dns2 = (uint32_t)0)
bool ip_config()
{
    esp_err_t err = ESP_OK;
    tcpip_adapter_ip_info_t info;

    info.ip.addr = *((uint32_t*)my_if_ip);
    info.gw.addr = *((uint32_t*)my_gw_ip);
    info.netmask.addr = *((uint32_t*)my_subnet);

    err = tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_ETH);
    if(err != ESP_OK && err != ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STOPPED){
        //log_e("DHCP could not be stopped! Error: %d", err);
        return false;
    }

    err = tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH, &info);
    if(err != ERR_OK){
        //log_e("STA IP could not be configured! Error: %d", err);
        return false;
    }
    // if(!info.ip.addr){
    //     err = tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_ETH);
    //     if(err != ESP_OK && err != ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STARTED){
    //         log_w("DHCP could not be started! Error: %d", err);
    //         return false;
    //     }
    // }
    //ip_addr_t d;
    //d.type = IPADDR_TYPE_V4;
    //if(static_cast<uint32_t>(dns1) != 0) {
    //    // Set DNS1-Server
    //    d.u_addr.ip4.addr = static_cast<uint32_t>(dns1);
    //    dns_setserver(0, &d);
    //}
    //if(static_cast<uint32_t>(dns2) != 0) {
    //    // Set DNS2-Server
    //    d.u_addr.ip4.addr = static_cast<uint32_t>(dns2);
    //    dns_setserver(1, &d);
    //}

    return true;
}

esp_err_t if_eth_start(void)
{
    esp_err_t err;
    init_pmap();
    memset(&arp_cache,0,sizeof(arp_cache));


    //while (!Serial);
    WiFi.onEvent(ETH_event);

    tcpipInit();
    tcpip_adapter_set_default_eth_handlers();
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    eth_netif = esp_netif_new(&cfg);

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.clock_config.rmii.clock_mode = (eth_clock_mode) ? EMAC_CLK_OUT : EMAC_CLK_EXT_IN;
    mac_config.clock_config.rmii.clock_gpio = (1 == eth_clock_mode) ? EMAC_APPL_CLK_OUT_GPIO : (2 == eth_clock_mode) ? EMAC_CLK_OUT_GPIO : (3 == eth_clock_mode) ? EMAC_CLK_OUT_180_GPIO : EMAC_CLK_IN_GPIO;
    mac_config.smi_mdc_gpio_num = _pin_mdc;
    mac_config.smi_mdio_gpio_num = _pin_mdio;
    mac_config.sw_reset_timeout_ms = 1000;
    mac = esp_eth_mac_new_esp32(&mac_config);

    static eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = _phy_addr; //phy_addr;
    phy_config.reset_gpio_num = _pin_power;
    phy = esp_eth_phy_new_lan87xx(&phy_config);
    static esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy); // apply default driver configuration
    err = esp_eth_driver_install(&eth_config, &if_eth_handle); // install driver
    if(err != ESP_OK || if_eth_handle == NULL)
    {
        //log_e("esp_eth_driver_install failed %d",err);
        //esp_deep_sleep_start();
        return err;
    }
    /* attach Ethernet driver to TCP/IP stack */
    err = esp_netif_attach(eth_netif, esp_eth_new_netif_glue(if_eth_handle));
    if(err != ESP_OK){
        //log_e("esp_netif_attach failed %d",err);
        //esp_deep_sleep_start();
        return err;
    }

    ip_config();
    /* attach to WiFiGeneric to receive events */
    add_esp_interface_netif(ESP_IF_ETH, eth_netif);

    err = esp_eth_start(if_eth_handle);
    if(err != ESP_OK){
        //log_e("esp_eth_start failed %d",err);
        //esp_deep_sleep_start();
        return err;
    }
    delay(50);
    err = esp_netif_get_mac(eth_netif, my_mac_addr);
    if(err != ESP_OK){
        //log_e("esp_netif_get_mac failed %d",err);
        //esp_deep_sleep_start();
        return err;
    }
    ETH_waitForConnect();

    err = esp_eth_update_input_path(if_eth_handle,eth_rx_handler,NULL);
    if(err != ESP_OK){
        //log_e("esp_eth_update_input_path failed %d",err);
        //esp_deep_sleep_start();
        return err;
    }
    return ESP_OK;
}
