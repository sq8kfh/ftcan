/*
 * ft220x.c
 *
 * Created: 2014-08-14 09:35:20
 * Author: SQ8KFH
 */ 

#include "ft220x.h"
#include <avr/interrupt.h>

#define FT_SS_EN PORT_FT &= ~(1<<SS_FT)
#define FT_SS_DIS PORT_FT |= (1<<SS_FT)

#define FT_CLK_0 PORT_FT &= ~(1<<CLK_FT)
#define FT_CLK_1 PORT_FT |= (1<<CLK_FT)

#define FT_MIOSI0_0 PORT_FT &= ~(1<<MIOSI0_FT)
#define FT_MIOSI0_1 PORT_FT |= (1<<MIOSI0_FT)

uint8_t
spi_write(uint8_t value)
{
	uint8_t bit_ctr;
	for (bit_ctr=0; bit_ctr<8; ++bit_ctr) {  // output 8-bit
		if (value & (uint8_t)0x80)
		FT_MIOSI0_1;
		else
		FT_MIOSI0_0;

		value = (value << 1);   // shift next bit into MSB..
		asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop");
		FT_CLK_1;          // Set SCK high..
		asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop");
		value |= (PIN_FT & (1<<MISO_FT)) >> MISO_FT;          // capture current MISO bit
		asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop");
		FT_CLK_0;                // ..then set SCK low again
	}
	return value;
}

uint8_t
spi_read(uint8_t *value)
{
	uint8_t bit_ctr;
	uint8_t err = 0; // error from MISO
	for (bit_ctr=0; bit_ctr<8; ++bit_ctr) {  // output 8-bit
		*value = (*value << 1);  // shift next bit into MSB..
		err = (err << 1);
		asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop");
		FT_CLK_1;          // Set SCK high..
		asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop");
		*value |= (PIN_FT & (1<<MIOSI0_FT)) >> MIOSI0_FT;      // capture current MIOSIO bit
		err |= (PIN_FT & (1<<MISO_FT)) >> MISO_FT;            // capture current error bit on MISO
		asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop"); asm (" nop");
		FT_CLK_0;                // ..then set SCK low again
	}
	return err;
}

void
FT220X_init(void)
{
	DDR_FT = (1<<SS_FT) | (1<<CLK_FT) | (1<<MIOSI0_FT) | (1<<MIOSI1_FT) | (1<<MIOSI2_FT) | (1<<MIOSI3_FT);
	PORT_FT = (1<<MISO_FT) | (1<<MIOSI0_FT) | (1<<MIOSI1_FT) | (1<<MIOSI2_FT) | (1<<MIOSI3_FT);
}

uint8_t
FT220X_read(uint8_t *buf, uint8_t len)
{
	uint8_t i = 0;
	uint8_t err = 0;
	DDR_FT |= (1<<MIOSI0_FT); // MIOSIO as output
	cli();
	FT_SS_EN;
	err = spi_write(READ_REQ);
	if ((err & 0xFF) == 0xFE) { // check for data in fifo
		DDR_FT &= ~(1<<MIOSI0_FT); // MIOSIO as input
		PORT_FT |= (1<<MIOSI0_FT); //pull up
		for (i = 0; i < len; i++) {
			err = spi_read(&buf[i]);
			if ((err & 0xFF) == 0xFF) // break if no more data in fifo
			break;
		}
	}
	FT_SS_DIS;
	sei();
	return i;
}

uint8_t
FT220X_read_line(char *buf, uint8_t len)
{
	uint8_t i = 0;
	uint8_t err = 0;
	DDR_FT |= (1<<MIOSI0_FT); // MIOSIO as output
	cli();
	FT_SS_EN;
	err = spi_write(READ_REQ);
	if ((err & 0xFF) == 0xFE) { // check for data in fifo
		DDR_FT &= ~(1<<MIOSI0_FT); // MIOSIO as input
		PORT_FT |= (1<<MIOSI0_FT); //pull up
		for (i = 0; i < len; i++) {
			err = spi_read((uint8_t*)&buf[i]);
			if ((err & 0xFF) == 0xFF) // break if no more data in fifo
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

void
FT220X_write(uint8_t data)
{
	DDR_FT |= (1<<MIOSI0_FT);	//MIOSIO as output
	//PORT_FT |= (1<<MIOSI0_FT);
	cli();
	FT_SS_EN;
	spi_write(WRITE_REQ);
	spi_write(data);
	FT_SS_DIS;
	sei();
}

void
FT220X_write_s(char *data)
{
	DDR_FT |= (1<<MIOSI0_FT);	// MIOSIO as output
	//PORT_FT |= (1<<MIOSI0_FT);
	cli();
	FT_SS_EN;
	spi_write(WRITE_REQ);
	for (;*data != 0; ++data)
	spi_write((uint8_t)(*data));
	FT_SS_DIS;
	sei();
}
