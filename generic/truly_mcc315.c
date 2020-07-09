/*
 * truly_mcc315.c
 *
 *  Created on: 24.06.2020
 *      Author: pantec
 */


#include "truly_mcc315.h"

#ifdef GENERIC_CODE
	#include "timer.h"
		// must define
		//		MS_TO_GAP(ms)
		//		setup_gaptimer(gap)
		//		gaptimer_expired()
		//		uDelay150()
		//		uDelay2000()
		//		uDelay4500()

	#include "spi.h"
		// must define
		//		void spi_write(uint8_t)
		//		uint8_t nextSpiByte()

	#include "peripherie.h"
		// must define SPI_LCD_SS_CHAR, SPI_LCD_SS_NUM
#else
	#include "project.h"

	#include "timer_hc908.h"
	#include "spi_hc908.h"
#endif


#include 	"crouzet_lcd_keybits.h"

#include 	"keyMatrix.h"


#define		_displaycontrol		mcc315_data.displaycontrol
#define		_displaymode		mcc315_data.displaymode
// #define		_displayfunction	mcc315_data.displayfunction

#define		_numlines 			4
#define		CHARS_PER_ROW		12

const static uint8_t  _row_offsets[_numlines] =
		{0x00, CHARS_PER_ROW, 0x40, 0x40 + CHARS_PER_ROW};


#if !defined(SPI_LCD_SS_CHAR) || !defined(SPI_LCD_SS_NUM)
	#error "you must define 	SPI_LCD_SS_CHAR and SPI_LCD_SS_NUM somewhere in your config"
#endif

#define		SPI_LCD_SS_DDR		DEF_DDR(SPI_LCD_SS_CHAR)
#define		SPI_LCD_SS_PORT		DEF_PORT(SPI_LCD_SS_CHAR)
#define		SPI_LCD_SS_MASK		_BV(SPI_LCD_SS_NUM)
#define		SPI_LCD_SS			DEF_PTX(SPI_LCD_SS_CHAR, SPI_LCD_SS_NUM)



/************ low level data pushing commands **********/

// The display needs LSB order but the SPI unit of 68hc908 is MSB
#include "bit_reverse.h"


/**
 * write to and read from display via SPI
 */
uint8_t mcc315lcd_transport(hd44780_write_mode_e mode)
{
	SPI_LCD_SS = 0;	// pull slave select wire low, shared with one row of key-matrix
	uDelay150();

	uint8_t br = bitreversed(mcc315_data.transferBuf[0]);
	mcc315_data.transferBuf[0] = br;
	uint8_t sent = 0;

	spi_write(mode);

	spi_write(br & 0xF0);
	spi_write((br << 4) & 0xF0);

//	mcc315_data.writeMode = mode;

#if 0
	if (mode == lcd_Command) {
		return 0;
	}

	if (mode == lcd_Write) {
		return 0;
	}

	sent = 3;

	setup_gaptimer(MS_TO_GAP(10));  // 10 ms should be enough patience

	while (sent > 0) {
		if (spi_bytesAvailable) {
			sent--;
			mcc315_data.transferBuf[sent] = nextSpiByte();
			if (sent == 0) {
				// received in reversed order to save a variable
				return 3;
//				value[0] // last byte
//				value[1] // high nibble
//				value[2] // response to read command
			}
		}
		if (gaptimer_expired()) {
			return 0xFF;
		}
	}
#endif
	return 0;
}


uint8_t readRAM()
{
	uint8_t r = mcc315lcd_transport(lcd_ReadRAM);
	if (r == 3) {
//		r = (mcc315_data.transferBuf[1] & 0xF0);
//		r |= (mcc315_data.transferBuf[0] >> 4);
		//					value[0] // last byte
		//					value[1] // high nibble
		//					value[2] // response to read command
		return bitreversed(mcc315_data.transferBuf[0]);
	}
	// timeout
	return 0xFF;
}

uint8_t readCGRAM(uint8_t addr)
{
	mcc315_data.transferBuf[0] = 0x40 | ( addr & 0x3F);
	return readRAM();
}

uint8_t readDRAM(uint8_t addr)
{
	mcc315_data.transferBuf[0] = 0x80 | ( addr & 0x7F);
	return readRAM();
}


/* TODO:
uint8_t readAddrBF()
{
	return transport(0, 1 , 0xFF);
}
*/







/*********** mid level commands, for sending data/cmds */

void mcc315lcd_command(uint8_t value)
{
	mcc315_data.transferBuf[0] = value;
	mcc315lcd_transport(lcd_Command);
}

// inline
void mcc315lcd_write(uint8_t value)
{
	mcc315_data.transferBuf[0] = value;
	mcc315lcd_transport(lcd_Write);
}



/********** high level commands, for the user! */

void mcc315lcd_clear()
{
	mcc315lcd_command(lcd_CLEARDISPLAY);  // clear display, set cursor position to zero
	uDelay2000();  // this command takes a long time!
}

void mcc315lcd_home()
{
	mcc315lcd_command(lcd_RETURNHOME);  // set cursor position to zero
	uDelay2000();  // this command takes a long time!
}

//inline
void mcc315lcd_setCursor(uint8_t col, uint8_t row)
{

	const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
	if ( row >= max_lines ) {
		row = max_lines - 1;    // we count rows starting w/0
	}
	if ( row >= _numlines ) {
		row = _numlines - 1;    // we count rows starting w/0
	}
	mcc315lcd_command(lcd_SETDDRAMADDR | (col + _row_offsets[row]));
}

// Turn the display on/off (quickly)
inline void mcc315lcd_noDisplay()
{
	_displaycontrol &= ~lcd_DISPLAYON;
	mcc315lcd_command(lcd_DISPLAYCONTROL | _displaycontrol);
}

inline void mcc315lcd_display()
{
	_displaycontrol |= lcd_DISPLAYON;
	mcc315lcd_command(lcd_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
inline void mcc315lcd_noCursor()
{
	_displaycontrol &= ~lcd_CURSORON;
	mcc315lcd_command(lcd_DISPLAYCONTROL | _displaycontrol);
}

inline void mcc315lcd_cursor()
{
	_displaycontrol |= lcd_CURSORON;
	mcc315lcd_command(lcd_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
inline void mcc315lcd_noBlink()
{
	_displaycontrol &= ~lcd_BLINKON;
	mcc315lcd_command(lcd_DISPLAYCONTROL | _displaycontrol);
}

inline void mcc315lcd_blink()
{
	_displaycontrol |= lcd_BLINKON;
	mcc315lcd_command(lcd_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
inline void mcc315lcd_scrollDisplayLeft(void)
{
	mcc315lcd_command(lcd_CURSORSHIFT | lcd_DISPLAYMOVE | lcd_MOVELEFT);
}

inline void mcc315lcd_scrollDisplayRight(void)
{
	mcc315lcd_command(lcd_CURSORSHIFT | lcd_DISPLAYMOVE | lcd_MOVERIGHT);
}

// This is for text that flows Left to Right
inline void mcc315lcd_leftToRight(void)
{
	_displaymode |= lcd_ENTRYLEFT;
	mcc315lcd_command(lcd_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
inline void mcc315lcd_rightToLeft(void)
{
	_displaymode &= ~lcd_ENTRYLEFT;
	mcc315lcd_command(lcd_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
inline void mcc315lcd_autoscroll(void)
{
	_displaymode |= lcd_ENTRYSHIFTINCREMENT;
	mcc315lcd_command(lcd_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
inline void mcc315lcd_noAutoscroll(void)
{
	_displaymode &= ~lcd_ENTRYSHIFTINCREMENT;
	mcc315lcd_command(lcd_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
inline void mcc315lcd_createChar(uint8_t location, uint8_t charmap[])
{
	location &= 0x07; // we only have 8 locations 0-7
	mcc315lcd_command(lcd_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		mcc315lcd_write(charmap[i]);
		uDelay150();
	}
}

/**
 *

EA DOGS164-A
APPLICATION EXAMPLES

$3A	8 bit data length extension Bit RE=1; REV=0
	Extended function set
$09	4 line display	Entry mode set
$06 bottom view Bias setting
$1E  BS1=1 Function Set
$39 8 bit data length extension Bit RE=0; IS=1 Internal OSC
$1B BS0=1 -> Bias=1/6 Follower control
$6C Devider on and set value Power control
$56 Booster on and set contrast (DB1=C5, DB0=C4) Contrast Set
$7A Set contrast (DB3-DB0=C3-C0) Function Set
$38 8 bit data length extension Bit RE=0; IS=0 Display On
$0F Display on, cursor on, blink on
 *
 *
 *
 *
Call writecom(0x30); //wake up
Call writecom(0x39); //function set
Call writecom(0x14); //internal osc frequency
Call writecom(0x56); //power control
Call writecom(0x6D); //follower control
Call writecom(0x70); //contrast
Call writecom(0x0C); //display on
Call writecom(0x06); //entry mode
Call writecom(0x01); //clear
 * */


/**
 * write to symbol buffer from cached bits
 * @param idx   index to cached symbol bits
 */
void mcc315lcd_writeSymbol(uint8_t idx)
{
	uint8_t addr = idx;

	if (idx > 2) {	// cgram addresses 0,1,2,5,7,9,11 are used
		addr <<= 1;
		addr--;
	}

	mcc315lcd_command(lcd_SETDDRAMADDR);			// set DRAM address to zero
	mcc315lcd_command(lcd_SETCGRAMADDR | addr);
	mcc315lcd_write( mcc315_data.symbolCache[idx] );
}



void mcc315lcd_standard()
{
	mcc315lcd_command(lcd_FUNCTIONSET | lcd_8BITMODE | lcd_2LINE /*| lcd_Extra | lcd_5x10DOTS | lcd_Inverse*/); // 0x38
//	mcc315lcd_command(0x38); // 0x38
	uDelay4500();  // wait more than 4.1ms


	mcc315lcd_command(lcd_DISPLAYCONTROL | lcd_DISPLAYON | lcd_CURSORON | lcd_BLINKON);	// 0x0F
	uDelay4500();  // wait more than 4.1ms

    // clear it off
    mcc315lcd_home();
}

/**
 * initializes the display from power-on
 */
inline void mcc315lcd_init()
{
	spi_init();

#ifdef WITH_KEYBOARD
	keymatrix_init();
#endif

//    _displayfunction |= lcd_2LINE;

	mcc315lcd_command(lcd_FUNCTIONSET | lcd_8BITMODE);	// 0x30  wake-up
	uDelay4500();  // wait more than 4.1ms

	mcc315lcd_command(0x3F);						// special function mode
	uDelay4500();  // wait more than 4.1ms

    uint8_t i;
    for (i=0; i<7; i++) {	// clear all symbols
    	mcc315lcd_writeSymbol(i);
    }

    mcc315lcd_standard();

    // clear it off
    mcc315lcd_clear();

    return;	// implementation not yet finished


#if 0
	// WARNING: spi must be initialized before

    // we start in 8bit mode
    mcc315lcd_transport(0x30);
    uDelay4500(); // wait min 4.1ms

    mcc315lcd_transport(0x30);
    uDelay4500(); // wait min 4.1ms

    // third try
    mcc315lcd_transport(0x30);
    uDelay150(); // wait min 150�s

    // finally, set to 4-bit interface
    mcc315lcd_transport(0x20);


    // this is according to the hitachi HD44780 datasheet
    // page 45 figure 23

    // Send function set command sequence
    mcc315lcd_command(lcd_FUNCTIONSET | _displayfunction);
    uDelay4500();  // wait more than 4.1ms

    // second try
    mcc315lcd_command(lcd_FUNCTIONSET | _displayfunction);
    uDelay150();

    // third go
    mcc315lcd_command(lcd_FUNCTIONSET | _displayfunction);

/*
    mcc315lcd_command(0x39); //function set
    uDelay150();

    mcc315lcd_command(0x14); //internal osc frequency
    uDelay150();

    mcc315lcd_command(0x56); //power control
    uDelay150();

    mcc315lcd_command(0x6D); //follower control
    uDelay150();

    mcc315lcd_command(0x7F); //contrast
    uDelay150();

    mcc315lcd_command(0x0C); // display ON
    uDelay150();
*/

    // finally, set # lines, font size, etc.
    mcc315lcd_command(lcd_FUNCTIONSET | _displayfunction);

    // turn the display on with no cursor or blinking default
    _displaycontrol = lcd_DISPLAYON | lcd_CURSOROFF | lcd_BLINKOFF;
    mcc315lcd_display();

    // clear it off
    mcc315lcd_clear();

    // Initialize to default text direction (for romance languages)
    _displaymode = lcd_ENTRYLEFT | lcd_ENTRYSHIFTDECREMENT;
    // set the entry mode
    mcc315lcd_command(lcd_ENTRYMODESET | _displaymode);
#endif
}


void mcc315lcd_setSymbol(top_row_symbols_e sym, top_row_attr_e attr)
{
	uint8_t ci = sym & symCache;

	if (ci != 0) {
		ci >>= 5;
	}

	uint8_t flag = (sym & symMASK) | (attr & symBLINK);
	uint8_t delta = mcc315_data.symbolCache[ci] ^ flag;

	if (delta == 0) {
		// already done
		return;
	}
	// we have to change things

	if (attr & symON) {
		mcc315_data.symbolCache[ci] |= flag;
	} else {
		mcc315_data.symbolCache[ci] &= ~flag;
	}
	mcc315lcd_writeSymbol(ci);
}



void mcc315lcd_setRotor(uint8_t count, top_row_attr_e blink)
{
	mcc315lcd_command(0x3F);

	uint8_t mask = 0x01;
	uint8_t i = 0;

	mcc315_data.symbolCache[0] = (count & 0x1F) | blink;
	mcc315lcd_writeSymbol(0);

	if (count & 0x20) {
		mcc315lcd_setSymbol(symRotor5, symON | blink);
	} else {
		mcc315_data.symbolCache[1] = 0;
		mcc315lcd_writeSymbol(1);
	}

//	mcc315lcd_command(0x38);
	mcc315lcd_standard();
}

void mcc315lcd_setPropeller(uint8_t count, uint8_t left)
{
	uint8_t pi = count % 3;
	uint8_t blink = left & 0x80;

	if (left & 0x01) {
		pi = 2 - pi;
	}
	switch(pi) {
		case 0:
			mcc315_data.symbolCache[0] = symRotor0 | symRotor3 | blink;
			mcc315_data.symbolCache[1] = 0;
			break;

		case 1:
			mcc315_data.symbolCache[0] = symRotor1 | symRotor4 | blink;
			mcc315_data.symbolCache[1] = 0;
			break;

		case 2:
			mcc315_data.symbolCache[0] = symRotor2 | blink;
			mcc315_data.symbolCache[1] = symRotor5 | blink;
			break;
	}
	mcc315lcd_command(0x3F);

	mcc315lcd_writeSymbol(0);
	mcc315lcd_writeSymbol(1);

//	mcc315lcd_command(0x38);
	mcc315lcd_standard();
}

void mcc315lcd_writeString(uint8_t *addr)
{
	while(*addr != '\0') {
		mcc315lcd_write(*addr);
		addr++;
	}
}



