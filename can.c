/*
 * can.c
 *
 * Created: 2014-08-15 16:28:36
 * Author: SQ8KFH
 */ 

#include "can.h"

can_buf_t can_rx_buf[CAN_RX_BUF_SIZE];
volatile uint8_t can_rx_buf_top = 0;
volatile uint8_t can_rx_buf_bottom = 0;

ISR(CAN_INT_vect)
{
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

void
CAN_init(void)
{
	CANGCON = ( 1 << SWRES );   // CAN reset
	CANTCON = 0xff;             // CAN timing prescaler

	CANHPMOB = 0x00;			// preprograowanie 4 najmlodszych bitow dla CANPAGE = CANHPMOB;

	#if F_CPU == 4000000UL
		CANBT1 = 0x06;
		CANBT2 = 0x04;
		CANBT3 = 0x13;
	#elif F_CPU == 8000000UL
		CANBT1 = 0x0e;
		CANBT2 = 0x04;
		CANBT3 = 0x13;
	#elif F_CPU == 16000000UL
		CANBT1 = 0x1e;
		CANBT2 = 0x04;
		CANBT3 = 0x13;
	#else
		#error "Please specify F_CPU"
	#endif
	
	for ( uint8_t mob=0; mob<6; mob++ ) { 
		CANPAGE = ( mob << MOBNB0);        // Selects Message Object 0-5
		CANCDMOB = 0x00;             // Disable mob
		CANSTMOB = 0x00;           // Clear mob status register;
	}
	for ( uint8_t mob=1; mob<6; mob++ ) { 
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

void
CAN_tx(can_buf_t *buf)
{

   CANPAGE = 0 << MOBNB0;      // Select MOb0 for transmission
   
   while ( CANEN2 & ( 1 << ENMOB0 ) ); // Wait for MOb 0 to be free
   
   CANSTMOB = 0x00;       // Clear mob status register
   
   CANIDT4 = buf->canidt4;        // Set can id to 0   
   CANIDT3 = buf->canidt3;      // ""
   CANIDT2 = buf->canidt2;      // ""
   CANIDT1 = buf->canidt1;      // ""
   
   for ( int8_t i = 0; i < 8; ++i ){

        CANMSG = buf->data[i];  // set message data for all 8 bytes to 55 (alternating 1s and 0s
 
   } // for
    
   CANCDMOB = buf->cancdmob | ( 1 << CONMOB0 );    // Enable transmission         
}
