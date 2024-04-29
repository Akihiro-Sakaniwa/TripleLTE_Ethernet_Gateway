/*
 * Copyright (c) 2024 Akihiro Sakaniwa
 *
 * SPDX-License-Identifier: MIT License
 */
#pragma once

#ifndef _HDR_ICMP_ECHO_REQ_H__
#define _HDR_ICMP_ECHO_REQ_H__

#include <Arduino.h>

void operate_packet_icmp_echo_req(uint8_t *buffer, uint32_t length);

#endif  // _HDR_ICMP_ECHO_REQ_H__