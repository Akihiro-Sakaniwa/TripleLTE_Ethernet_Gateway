# TripleLTE_Ethernet_Gateway
LTE Ethernet Gateway using three nRF9160s and connecting to ESP32 and UART


# Overview
This source program is LTE-Ethernet-Gateway using three nRF9160s. 
On the Ethernet side, we use a board with 8720a connected to ESP32 via RMII, and three nRF9160s are connected via UART.
In order to know the LTE connection status of each nRF9160 on the ESP32 side, a total of three Status lines are connected. 
The Status line is LOW when the LTE connection status is valid, and HIGH when it is invalid.

I hope this is of some use to you.

FOTA related functions have been omitted because the code would be complicated.

Set the MTU of the LAN side device to 1280.

The GW IP address is written in if_eth.h(ESP32).

There are no DHCP related functions.

Since the port mapping function is implemented, multiple units can be connected on the LAN side.


# Target SDK

nRF9160 NCS v2.5.2

ESP32 PlatformIO Espressif32  6.4.0  Arduino

# Target board

スイッチサイエンス nRF9160搭載 LTE-M/NB-IoT/GNSS対応 IoT開発ボード

https://ssci.to/8999

Actinius Icarus SoM DK

https://www.actinius.com/icarus-som-dk

ESP32-POE-ISO or ESP32-POE

https://www.olimex.com/Products/IoT/ESP32/ESP32-POE-ISO/open-source-hardware

# Target SIM

SORACOM PLAN-D

https://soracom.jp/store/13380/


# Board Pin Assign

nRF9160 see  xxxxxxxx_overlay.dts and main.h files
ESP32 see  gw_uart.h file



# Disclaimer
This source program is an implementation example and does not provide any guarantees.
Please use at your own risk.

# License
LicenseRef-Nordic-5-Clause

MIT License
