/*
 * can.h
 *
 * Created by SQ8KFH on 2014-08-15.
 *
 * Copyright (C) 2014-2020 Kamil Palkowski. All rights reserved.
 */

#ifndef CAN_H_
#define CAN_H_

#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define CAN_RX_BUF_SIZE 16
#define CAN_RX_BUF_INDEX_MASK 0x0F

#define CAN_TX_BUF_SIZE 16u
#define CAN_TX_BUF_INDEX_MASK 0x0Fu

typedef struct {
    uint8_t canidt1;
    uint8_t canidt2;
    uint8_t canidt3;
    uint8_t canidt4;
    uint8_t cancdmob;
    uint8_t data[8];
} can_buf_t;

extern can_buf_t can_rx_buf[CAN_RX_BUF_SIZE];
extern volatile uint8_t can_rx_buf_top;
extern volatile uint8_t can_rx_buf_bottom;

void CAN_init(uint8_t bitrate);
void CAN_disable(void);
uint8_t CAN_tx(can_buf_t *buf);

#endif /* CAN_H_ */
