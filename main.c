/*
 * main.c
 *
 * Created by SQ8KFH on 2014-08-09.
 *
 * Copyright (C) 2014-2020 Kamil Palkowski. All rights reserved.
 */

#include "config.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "ft220x.h"
#include "can.h"

char itoh(uint8_t i) {
    char h[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    return h[i & 0x0F];
}

uint8_t htoi(char c) {
    if ('0' <= c && c <= '9')
        return ((uint8_t)(c - '0') & 0x0F);
    switch (c) {
        case 'a':
        case 'A':
            return 0x0A;
        case 'b':
        case 'B':
            return 0x0B;
        case 'c':
        case 'C':
            return 0x0C;
        case 'd':
        case 'D':
            return 0x0D;
        case 'e':
        case 'E':
            return 0x0E;
        case 'f':
        case 'F':
            return 0x0F;
    }
    return 0x00;
}

uint8_t slcan_command_t(char *data)	{ //TX Frame Format SFF tiiildddddddddddddddd
    can_buf_t frame;
    uint8_t id2	= htoi(data[1]);
    uint8_t id1	= htoi(data[2]);
    uint8_t id0	= htoi(data[3]);

    frame.canidt1 = id0;
    frame.canidt1 <<= 4;
    frame.canidt1 |= id1;
    frame.canidt1 <<= 1;
    frame.canidt1 |= (id2 >> 3);
    frame.canidt2 = id2;
    frame.canidt2 <<= 5;
    frame.canidt3 = 0;
    frame.canidt4 = 0;

    uint8_t dlc = htoi(data[4]);
    if (dlc > 8)
        return 0;
    frame.cancdmob = dlc;

    for (int i = 5, i2 = 0; data[i] != '\r' && i2 < 8; i+=2, ++i2) {
        frame.data[i2] = htoi(data[i]);
        frame.data[i2] <<= 4;
        frame.data[i2] |= htoi(data[i+1]);
    }

    CAN_tx(&frame);
    return 1;
}

uint8_t slcan_command_T(char *data)	{ //TX Frame Format EFF Tiiiiiiiildddddddd...
    can_buf_t frame;
    uint8_t id7	= htoi(data[1]);
    uint8_t id6	= htoi(data[2]);
    uint8_t id5	= htoi(data[3]);
    uint8_t id4	= htoi(data[4]);
    uint8_t id3	= htoi(data[5]);
    uint8_t id2	= htoi(data[6]);
    uint8_t id1	= htoi(data[7]);
    uint8_t id0	= htoi(data[8]);

    frame.canidt4 = id1;
    frame.canidt4 <<= 4;
    frame.canidt4 |= id0;
    frame.canidt4 <<= 3;

    frame.canidt3 = id3;
    frame.canidt3 <<= 4;
    frame.canidt3 |= id2;
    frame.canidt3 <<= 3;
    frame.canidt3 |= (id1 >> 1);

    frame.canidt2 = id5;
    frame.canidt2 <<= 4;
    frame.canidt2 |= id4;
    frame.canidt2 <<= 3;
    frame.canidt2 |= (id3 >> 1);

    frame.canidt1 = id7;
    frame.canidt1 <<= 4;
    frame.canidt1 |= id6;
    frame.canidt1 <<= 3;
    frame.canidt1 |= (id5 >> 1);

    uint8_t dlc = htoi(data[9]);
    if (dlc > 8)
        return 0;
    frame.cancdmob = dlc | (1 << IDE);

    for (int i = 10, i2 = 0; data[i] != '\r' && i2 < 8; i+=2, ++i2) {
        frame.data[i2] = htoi(data[i]);
        frame.data[i2] <<= 4;
        frame.data[i2] |= htoi(data[i+1]);
    }

    CAN_tx(&frame);
    return 1;
}

uint8_t slcan_command_r(char *data)	{ //TX Frame Format RTR/SFF riiil
    can_buf_t frame;
    uint8_t id2	= htoi(data[1]);
    uint8_t id1	= htoi(data[2]);
    uint8_t id0	= htoi(data[3]);

    frame.canidt1 = id0;
    frame.canidt1 <<= 4;
    frame.canidt1 |= id1;
    frame.canidt1 <<= 1;
    frame.canidt1 |= (id2 >> 3);
    frame.canidt2 = (id2 << 5);
    frame.canidt3 = 0;
    frame.canidt4 = (1 << RTRTAG);

    uint8_t dlc = htoi(data[4]);
    if (dlc > 8)
        return 0;
    frame.cancdmob = dlc;

    CAN_tx(&frame);
    return 1;
}

uint8_t slcan_command_R(char *data)	{ //TX Frame Format RTR/EFF Riiiiiiiil
    can_buf_t frame;
    uint8_t id7	= htoi(data[1]);
    uint8_t id6	= htoi(data[2]);
    uint8_t id5	= htoi(data[3]);
    uint8_t id4	= htoi(data[4]);
    uint8_t id3	= htoi(data[5]);
    uint8_t id2	= htoi(data[6]);
    uint8_t id1	= htoi(data[7]);
    uint8_t id0	= htoi(data[8]);

    frame.canidt4 = id1;
    frame.canidt4 <<= 4;
    frame.canidt4 |= id0;
    frame.canidt4 <<= 3;
    frame.canidt4 |= (1 << RTRTAG);

    frame.canidt3 = id3;
    frame.canidt3 <<= 4;
    frame.canidt3 |= id2;
    frame.canidt3 <<= 3;
    frame.canidt3 |= (id1 >> 1);

    frame.canidt2 = id5;
    frame.canidt2 <<= 4;
    frame.canidt2 |= id4;
    frame.canidt2 <<= 3;
    frame.canidt2 |= (id3 >> 1);

    frame.canidt1 = id7;
    frame.canidt1 <<= 4;
    frame.canidt1 |= id6;
    frame.canidt1 <<= 3;
    frame.canidt1 |= (id5 >> 1);

    uint8_t dlc = htoi(data[9]);
    if (dlc > 8)
        return 0;
    frame.cancdmob = dlc | (1 << IDE);

    CAN_tx(&frame);
    return 1;
}

void slcan_interpreter(char *command) {
    if (command[0] == 'T') {	//TX Frame Format EFF Tiiiiiiiildddddddd...
        if (slcan_command_T(command))
            FT220X_write('\r');
        else
            goto error;
    }
    else if (command[0] == 't') {	//TX Frame Format SFF tiiildddddddddd...
        if (slcan_command_t(command))
            FT220X_write('\r');
        else
            goto error;
    }
    else if (command[0] == 'R') {	//TX Frame Format RTR/EFF Riiiiiiiil
        if (slcan_command_R(command))
            FT220X_write('\r');
        else
            goto error;
    }
    else if (command[0] == 'r') {	//TX Frame Format RTR/SFF riiil
        if (slcan_command_r(command))
            FT220X_write('\r');
        else
            goto error;
    }
    else if (command[0] == 'O') {	//Open Channel
        if (command[1] != '\r')
            goto error;
        FT220X_write('\r');
    }
    else if (command[0] == 'C') {	//Close Channel
        if (command[1] != '\r')
            goto error;
        FT220X_write('\r');
    }
    else if (command[0] == 'F') {	//Read Status Flags
        if (command[1] != '\r')
            goto error;
        char tmp[5];
        sprintf(tmp, "F%02hhx\r", CANGSTA);
        FT220X_write_s(tmp);
    }
//	else if (command[0] == 'M') {	//Acceptance Mask Mxxxxxxxx
//	}
//	else if (command[0] == 'm') {	//Acceptance Value mxxxxxxxx
//	}
    else if (command[0] == 'V') {	//HW/SW Version
        if (command[1] != '\r')
            goto error;
        char tmp[9];
        sprintf(tmp, "V01%02hhu\r", FTCAN_VERSION_MAJOR);
        FT220X_write_s(tmp);
    }
    else if (command[0] == 'v') {	//Major/Minor Version
        if (command[1] != '\r')
            goto error;
        char tmp[9];
        sprintf(tmp, "v%02hhu%02hhu\r", FTCAN_VERSION_MAJOR, FTCAN_VERSION_MINOR);
        FT220X_write_s(tmp);
    }
    else {
        goto error;
    }
    return;

    error:
        FT220X_write('\a');
        return;
}

int main(void) {
    DDRC = (1<<PC0);
    PORTC &= ~(1<<PC0);
    FT220X_init();
    CAN_init();
    sei();

    FT220X_flush();

    uint8_t read_idx = 0;
    uint8_t led = 0;
    while (1) {
//		if (led == 0) {
//			PORTC ^= (1<<PC0); //led toggle
//		}
//		++led;

        char s[51];
        PORTC |= (1<<PC0);
        read_idx += FT220X_read_line(&s[read_idx],50-read_idx);
        PORTC &= ~(1<<PC0);
        if (s[read_idx-1] == '\r') {
            s[read_idx] = '\0';
            read_idx=0;
            slcan_interpreter(s);
        }
        else if (read_idx > 50) {
            read_idx=0;
            FT220X_write('\b');
        }

        if (can_rx_buf_top != can_rx_buf_bottom) {
            char buf[30];
            uint8_t i, dlc;
            if (can_rx_buf[can_rx_buf_bottom].cancdmob & (1 << IDE)) {	//29-bits
                if (can_rx_buf[can_rx_buf_bottom].canidt4 & (1 << RTRTAG)) //remote frame
                    buf[0] = 'R';
                else
                    buf[0] = 'T';
                buf[1] = itoh(can_rx_buf[can_rx_buf_bottom].canidt1 >> 7);
                buf[2] = itoh(can_rx_buf[can_rx_buf_bottom].canidt1 >> 3);
                buf[3] = itoh(can_rx_buf[can_rx_buf_bottom].canidt1 << 1 | can_rx_buf[can_rx_buf_bottom].canidt2 >> 7);
                buf[4] = itoh(can_rx_buf[can_rx_buf_bottom].canidt2 >> 3);
                buf[5] = itoh(can_rx_buf[can_rx_buf_bottom].canidt2 << 1 | can_rx_buf[can_rx_buf_bottom].canidt3 >> 7);
                buf[6] = itoh(can_rx_buf[can_rx_buf_bottom].canidt3 >> 3);
                buf[7] = itoh(can_rx_buf[can_rx_buf_bottom].canidt3 << 1 | can_rx_buf[can_rx_buf_bottom].canidt4 >> 7);
                buf[8] = itoh(can_rx_buf[can_rx_buf_bottom].canidt4 >> 3);
                dlc = can_rx_buf[can_rx_buf_bottom].cancdmob & 0x0F;
                buf[9] = itoh(dlc);
                i = 10;
            }
            else {	//11-bits
                if (can_rx_buf[can_rx_buf_bottom].canidt4 & (1 << RTRTAG)) //remote frame
                    buf[0] = 'r';
                else
                    buf[0] = 't';
                buf[1] = itoh(can_rx_buf[can_rx_buf_bottom].canidt1 >> 5);
                buf[2] = itoh(can_rx_buf[can_rx_buf_bottom].canidt1 >> 1);
                buf[3] = itoh(can_rx_buf[can_rx_buf_bottom].canidt1 << 3 | can_rx_buf[can_rx_buf_bottom].canidt2 >> 5);
                dlc = can_rx_buf[can_rx_buf_bottom].cancdmob & 0x0F;
                buf[4] = itoh(dlc);
                i = 5;
            }
            if (!(can_rx_buf[can_rx_buf_bottom].canidt4 & (1 << RTRTAG))) {
                for (uint8_t op = 0; op < dlc; ++op) {
                    buf[i++] = itoh(can_rx_buf[can_rx_buf_bottom].data[op] >> 4);
                    buf[i++] = itoh(can_rx_buf[can_rx_buf_bottom].data[op]);
                }
            }
            buf[i++] = '\r';
            buf[i++] = '\0';
            FT220X_write_s(buf);
            can_rx_buf_bottom = (uint8_t)((can_rx_buf_bottom + 1) & CAN_RX_BUF_INDEX_MASK);
        }
    }
}
