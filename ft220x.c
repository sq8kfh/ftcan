/*
 * ft220x.c
 *
 * Created by SQ8KFH on 2014-08-14.
 *
 * Copyright (C) 2014-2020 Kamil Palkowski. All rights reserved.
 */

#include "ft220x.h"
#include <avr/interrupt.h>

#define FT_SS_EN PORT_FT &= ~(1<<SS_FT)
#define FT_SS_DIS PORT_FT |= (1<<SS_FT)

#define FT_CLK_0 PORT_FT &= ~(1<<CLK_FT)
#define FT_CLK_1 PORT_FT |= (1<<CLK_FT)

const uint8_t nibble_swap[16] = {0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE, 0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF};

static uint8_t spi_write(uint8_t value) {
    FT_CLK_1;
    PORT_FT = (PORT_FT & ~(0x0f << MIOSI0_FT)) | (nibble_swap[(value >> 4) & 0x0f] << MIOSI0_FT);
    asm ("nop");
    FT_CLK_0;
    asm ("nop");

    FT_CLK_1;
    PORT_FT = (PORT_FT & ~(0x0f << MIOSI0_FT)) | (nibble_swap[(value) & 0x0f] << MIOSI0_FT);
    asm ("nop");
    FT_CLK_0;
    uint8_t ack = (PIN_FT & (1<<MISO_FT)) >> MISO_FT;
    return ack;
}

static uint8_t spi_read(uint8_t *value) {
    FT_CLK_1;
    asm ("nop");
    FT_CLK_0;
    *value = nibble_swap[(PIN_FT >> MIOSI0_FT) & 0x0f] << 4u;
    //uint8_t ack = (PIN_FT & (1<<MISO_FT)) >> MISO_FT;
    //ack <<= 1u;

    FT_CLK_1;
    asm ("nop");
    FT_CLK_0;
    *value |= nibble_swap[(PIN_FT >> MIOSI0_FT) & 0x0f];
    uint8_t ack = (PIN_FT & (1<<MISO_FT)) >> MISO_FT;

    return ack;
}

static inline void set_miosi_as_input(void) {
    PORT_FT |= (1<<MIOSI0_FT) | (1<<MIOSI1_FT) | (1<<MIOSI2_FT) | (1<<MIOSI3_FT); //pull up
    DDR_FT &= ~((1<<MIOSI0_FT) | (1<<MIOSI1_FT) | (1<<MIOSI2_FT) | (1<<MIOSI3_FT)); // MIOSIO as input
    asm ("nop");
}

static inline void set_miosi_as_output(void) {
    DDR_FT |= (1<<MIOSI0_FT) | (1<<MIOSI1_FT) | (1<<MIOSI2_FT) | (1<<MIOSI3_FT);	//MIOSIO as output
}

static inline void bus_turnaround(void) {
    FT_CLK_1;
    set_miosi_as_input();
    FT_CLK_0;
}

void FT220X_init(void) {
    DDR_FT = (1<<SS_FT) | (1<<CLK_FT) | (1<<MIOSI0_FT) | (1<<MIOSI1_FT) | (1<<MIOSI2_FT) | (1<<MIOSI3_FT);
    PORT_FT = (1<<MISO_FT) | (1<<MIOSI0_FT) | (1<<MIOSI1_FT) | (1<<MIOSI2_FT) | (1<<MIOSI3_FT);
    set_miosi_as_input();
}

void FT220X_flush(void) {
    cli();
    FT_SS_EN;
    set_miosi_as_output();

    spi_write(FLUSH_REQ);
    bus_turnaround();

    FT_SS_DIS;
    sei();
}

uint8_t FT220X_read_line(char *buf, uint8_t len) {
    cli();
    FT_SS_EN;
    set_miosi_as_output();

    spi_write(READ_REQ);
    bus_turnaround();

    uint8_t rxf = (PIN_FT & (1<<MISO_FT)) >> MISO_FT;
    uint8_t i = 0;
    if (rxf == 0) { // check for data in fifo
        for (i = 0; i < len; i++) {
            uint8_t ack = spi_read((uint8_t*)&buf[i]);
            if (ack == 0) // break if no more data in fifo
            break;
            if (buf[i] == '\r') {
                ++i;
                break;
            }
        }
    }
    FT_SS_DIS;
    sei();
    return i;
}

void FT220X_write(uint8_t data) {
    cli();
    FT_SS_EN;
    set_miosi_as_output();

    spi_write(WRITE_REQ);

    //bus turnaround without changing direction
    FT_CLK_1;
    asm ("nop");
    FT_CLK_0;
    //uint8_t txe = (PIN_FT & (1<<MISO_FT)) >> MISO_FT;

    spi_write(data);

    set_miosi_as_input();
    FT_SS_DIS;
    sei();
}

uint8_t FT220X_write_s(char *data) {
    uint8_t err = 0;
    cli();
    FT_SS_EN;

    set_miosi_as_output();
    spi_write(WRITE_REQ);

    //bus turnaround without changing direction
    FT_CLK_1;
    asm ("nop");
    FT_CLK_0;
    uint8_t txe = (PIN_FT & (1<<MISO_FT)) >> MISO_FT;
    if (!txe) {
        for (; *data != 0; ++data) {
            if (!spi_write((uint8_t) (*data))) {
                err = 1;
                break;
            }
        }
    }
    else {
        err = 1;
    }
    set_miosi_as_input();
    FT_SS_DIS;
    sei();
    return err;
}
