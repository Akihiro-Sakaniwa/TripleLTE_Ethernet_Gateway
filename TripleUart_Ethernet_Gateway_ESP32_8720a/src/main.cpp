#include <Arduino.h>
#include "if_eth.h"
#include "gw_uart.h"

void setup()
{
    Serial.begin(115200);
    if_eth_start();
    gw_uart_start();
}
void loop()
{
    delay(100);
}