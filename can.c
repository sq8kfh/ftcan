/*
 * can.c
 *
 * Created by SQ8KFH on 2014-08-15.
 *
 * Copyright (C) 2014-2020 Kamil Palkowski. All rights reserved.
 */

#include "can.h"

can_buf_t can_rx_buf[CAN_RX_BUF_SIZE];
volatile uint8_t can_rx_buf_top = 0;
volatile uint8_t can_rx_buf_bottom = 0;

#if F_CPU == 8000000UL
static uint8_t can_bitrate_map[9][3] = {
        {0x0e, 0x04, 0x13}, //S0 10Kbit //TODO: calculate
        {0x0e, 0x04, 0x13}, //S1 20Kbit //TODO: calculate
        {0x0e, 0x04, 0x13}, //S2 50Kbit //TODO: calculate
        {0x12, 0x04, 0x13}, //S3 100Kbit
        {0x0e, 0x04, 0x13}, //S4 125Kbit
        {0x06, 0x04, 0x13}, //S5 250Kbit
        {0x02, 0x04, 0x13}, //S6 500Kbit
        {0x0e, 0x04, 0x13}, //S7 800Kbit //TODO: calculate
        {0x00, 0x04, 0x12}, //S8 1Mbit
};
#else
#error "Please specify F_CPU"
#endif

ISR(CAN_INT_vect) {
    uint8_t canhpmob = CANHPMOB;
    uint8_t cangit = CANGIT;
    if (canhpmob != 0xf0) {
        uint8_t savecanpage = CANPAGE;
        CANPAGE = canhpmob;
        if (CANSTMOB & (1 << RXOK)) {
            can_rx_buf[can_rx_buf_top].canidt1 = CANIDT1;
            can_rx_buf[can_rx_buf_top].canidt2 = CANIDT2;
            can_rx_buf[can_rx_buf_top].canidt3 = CANIDT3;
            can_rx_buf[can_rx_buf_top].canidt4 = CANIDT4;
            can_rx_buf[can_rx_buf_top].cancdmob = CANCDMOB & 0x1f;
            for (uint8_t i = 0; i < 8; ++i) {
                can_rx_buf[can_rx_buf_top].data[i] = CANMSG;
            }
            can_rx_buf_top = (uint8_t)((can_rx_buf_top + 1) & CAN_RX_BUF_INDEX_MASK);

            CANIDM1 = 0;
            CANIDM2 = 0;
            CANIDM3 = 0;
            CANIDM4 = 0;
            CANCDMOB = (1<<CONMOB1); //rx mob
        }
        else if (CANSTMOB & (1 << TXOK)) {
            CANCDMOB = 0; //disable mob
        }
        CANSTMOB = 0x00;  // Reset reason on selected channel
        CANPAGE = savecanpage;
    }
    //other interrupt
    CANGIT |= (cangit & 0x7f);
}

void CAN_init(uint8_t bitrate) {
    CANGCON = ( 1 << SWRES );   // CAN reset
    CANTCON = 0xff;             // CAN timing prescaler

    CANHPMOB = 0x00;			// preprograowanie 4 najmlodszych bitow dla CANPAGE = CANHPMOB;

    CANBT1 = can_bitrate_map[bitrate][0];
    CANBT2 = can_bitrate_map[bitrate][1];
    CANBT3 = can_bitrate_map[bitrate][2];

    for ( uint8_t mob=0; mob<6; mob++ ) {
        CANPAGE = ( mob << MOBNB0);   // Selects Message Object 0-5
        CANCDMOB = 0x00;              // Disable mob
        CANSTMOB = 0x00;              // Clear mob status register;
    }
    for ( uint8_t mob=2; mob<6; mob++ ) {
        CANPAGE = mob << MOBNB0;
        CANIDM1 = 0;
        CANIDM2 = 0;
        CANIDM3 = 0;
        CANIDM4 = 0;
        CANCDMOB = (1<<CONMOB1); /*| (1<<IDE) | (8<<DLC0); *///rx mob
    }
    CANIE2 = ( 1 << IEMOB0 ) | ( 1 << IEMOB1 ) | ( 1 << IEMOB2 ) | ( 1 << IEMOB3 ) | ( 1 << IEMOB4 ) | ( 1 << IEMOB5 );

    CANGIE =  (1<<ENIT) |(1<<ENBOFF) | (1<<ENRX) | (1<<ENTX) | (1<<ENERR) | (1<<ENBX) | (1<<ENERG);
    CANGCON = 1<<ENASTB;
}

void CAN_disable(void) {
    CANGCON = (1 << SWRES);   // CAN reset
}

uint8_t CAN_tx(can_buf_t *buf) {
    if (!(CANGSTA & (1 << ENFG))) return 0; //bus disable
    while (1) {
        if (!(CANEN2 & ( 1 << ENMOB0 ))) {
            CANPAGE = 0 << MOBNB0;
            break;
        }
        if (!(CANEN2 & ( 1 << ENMOB1 ))) {
            CANPAGE = 1 << MOBNB0;
            break;
        }
    }

    CANSTMOB = 0x00;       // Clear mob status register

    CANIDT4 = buf->canidt4;
    CANIDT3 = buf->canidt3;
    CANIDT2 = buf->canidt2;
    CANIDT1 = buf->canidt1;

    for ( int8_t i = 0; i < 8; ++i ) {
         CANMSG = buf->data[i];
    }

    CANCDMOB = buf->cancdmob | ( 1 << CONMOB0 );    // Enable transmission

    return 1;
}
