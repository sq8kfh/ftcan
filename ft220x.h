/*
 * ft220x.h
 *
 * Created by SQ8KFH on 2014-08-14.
 *
 * Copyright (C) 2014-2020 Kamil Palkowski. All rights reserved.
 */

#ifndef FT220X_H_
#define FT220X_H_

#include "config.h"
#include <avr/io.h>

#define DDR_FT DDRB
#define PORT_FT PORTB
#define PIN_FT PINB

#define MISO_FT PB0
#define MIOSI0_FT PB1
#define MIOSI1_FT PB2
#define MIOSI2_FT PB3
#define MIOSI3_FT PB4
#define SS_FT PB6
#define CLK_FT PB7

#define WRITE_REQ 0x00
#define READ_REQ 0x02
#define FLUSH_REQ 0x10

void FT220X_init(void);
void FT220X_flush(void);
uint8_t FT220X_read_line(char *buf, uint8_t len);
void FT220X_write(uint8_t data);
uint8_t FT220X_write_s(char *data);

#endif /* FT220X_H_ */
