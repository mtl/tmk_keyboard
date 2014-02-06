/*
Copyright 2012 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/****************************************************************************
 *
 *  Scan keyboard matrix
 *
 ***************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include "debug.h"
#include "led-local.h"
#include "print.h"
#include "matrix.h"
#include "util.h"

#ifdef DISPLAY_ENABLE
#include "display.h"
#endif


/***************************************************************************/
// Static prototypes:

static void init_cols(void);
static matrix_row_t read_cols(void);
static void select_row(uint8_t row);
static void unselect_rows(void);


/***************************************************************************/
// Globals:

#ifndef DEBOUNCE
#   define DEBOUNCE    5
#endif
static uint8_t debouncing = DEBOUNCE;

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_debouncing[MATRIX_ROWS];


/***************************************************************************/

/* Column pin configuration:
 * col: 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18
 * pin: C3  C2  C1  C0  F6  F5  F4  F3  F2  F1  E1  E0  D7  E7  F0  A5  D4  D3  E6
 */
static void init_cols() {

    // Input with pull-up(DDR:0, PORT:1)
    DDRA  &= ~0b00100000;
    PORTA |=  0b00100000;
    DDRC  &= ~0b00001111;
    PORTC |=  0b00001111;
    DDRD  &= ~0b10011000;
    PORTD |=  0b10011000;
    DDRE  &= ~0b11000011;
    PORTE |=  0b11000011;
    DDRF  &= ~0b01111111;
    PORTF |=  0b01111111;
}


/***************************************************************************/

inline
matrix_row_t matrix_get_row( uint8_t row ) {
    return matrix[row];
}


/***************************************************************************/

// common/keyboard.c invokes this when the keyboard is initialized.
// This is our entry point.
void matrix_init() {

    // To use PORTF disable JTAG with writing JTD bit twice within four cycles.
    MCUCR |= (1<<JTD);
    MCUCR |= (1<<JTD);
    
    // initialize row and col
    unselect_rows();
    init_cols();

    // initialize matrix state: all keys off
    for ( uint8_t i=0; i < MATRIX_ROWS; i++ ) {
        matrix[ i ] = (matrix_row_t) 0;
        matrix_debouncing[ i ] = (matrix_row_t) 0;
    }

    // Initialize LED control logic:
    led_init();

    // Initialize the OLED display:
#ifdef DISPLAY_ENABLE
    display_init();
#endif

}


/***************************************************************************/

void matrix_print() {

    print( "\nr/c 0123456789ABCDEFGHI\n" );

    for ( uint8_t row = 0; row < MATRIX_ROWS; row++ ) {

        phex( row );
        print( ": " );
        xprintf( "%019lb", bitrev32( matrix_get_row( row ) ) >> 13 );
        print( "\n" );
    }
}


/***************************************************************************/

/*

	if ( typematic_task() ) {
		goto MATRIX_LOOP_END;
	}




// last repeatable key press row & column
// ever-been-repeated flag
// time of last press event

bool typematic_repeated = false;
bool typematic_sent_break = false;

static bool typematic_task() {
}

static void typematic_update() {
}




*/

/***************************************************************************/

uint8_t matrix_scan() {

    // Update LED states if necessary:
#ifdef LED_CONTROLLER_ENABLE
    led_update();
#endif

    for ( uint8_t i = 0; i < MATRIX_ROWS; i++ ) {

        select_row( i );
        _delay_us( 30 );  // without this wait read unstable value.

        matrix_row_t cols = read_cols();

        if ( matrix_debouncing[ i ] != cols ) {

            matrix_debouncing[ i ] = cols;

            if ( debouncing ) {

                debug("bounce!: ");
                debug_hex( debouncing );
                debug( "\n" );
            }
            debouncing = DEBOUNCE;
        }
        unselect_rows();
    }

    if ( debouncing ) {

        if ( --debouncing ) {

            _delay_ms( 1 );

        } else {

            for ( uint8_t i = 0; i < MATRIX_ROWS; i++ ) {
                matrix[ i ] = matrix_debouncing[ i ];
            }
        }
    }

    return 1;
}


/***************************************************************************/

static matrix_row_t read_cols() {

    matrix_row_t cols = 0;

    uint8_t pin_a = PINA;
    uint8_t pin_c = PINC;
    uint8_t pin_d = PIND;
    uint8_t pin_e = PINE;
    uint8_t pin_f = PINF;

    if (!( pin_c & (1<<3) )) cols |= ((matrix_row_t) 1<<0);
    if (!( pin_c & (1<<2) )) cols |= ((matrix_row_t) 1<<1);
    if (!( pin_c & (1<<1) )) cols |= ((matrix_row_t) 1<<2);
    if (!( pin_c & (1<<0) )) cols |= ((matrix_row_t) 1<<3);
    if (!( pin_f & (1<<6) )) cols |= ((matrix_row_t) 1<<4);
    if (!( pin_f & (1<<5) )) cols |= ((matrix_row_t) 1<<5);
    if (!( pin_f & (1<<4) )) cols |= ((matrix_row_t) 1<<6);
    if (!( pin_f & (1<<3) )) cols |= ((matrix_row_t) 1<<7);
    if (!( pin_f & (1<<2) )) cols |= ((matrix_row_t) 1<<8);
    if (!( pin_f & (1<<1) )) cols |= ((matrix_row_t) 1<<9);
    if (!( pin_e & (1<<1) )) cols |= ((matrix_row_t) 1<<10);
    if (!( pin_e & (1<<0) )) cols |= ((matrix_row_t) 1<<11);
    if (!( pin_d & (1<<7) )) cols |= ((matrix_row_t) 1<<12);
    if (!( pin_e & (1<<7) )) cols |= ((matrix_row_t) 1<<13);
    if (!( pin_f & (1<<0) )) cols |= ((matrix_row_t) 1<<14);
    if (!( pin_a & (1<<5) )) cols |= ((matrix_row_t) 1<<15);
    if (!( pin_d & (1<<4) )) cols |= ((matrix_row_t) 1<<16);
    if (!( pin_d & (1<<3) )) cols |= ((matrix_row_t) 1<<17);
    if (!( pin_e & (1<<6) )) cols |= ((matrix_row_t) 1<<18);

    return cols;
}


/***************************************************************************/

static void select_row( uint8_t row ) {

    // Output low(DDR:1, PORT:0) to select
    DDRA  |= (1<<row);
    PORTA &= ~(1<<row);
}


/***************************************************************************/

/* Row pin configuration:
 * row: 0   1   2   3   4
 * pin: A0  A1  A2  A3  A4
 */
static void unselect_rows() {

    // Hi-Z(DDR:0, PORT:0) to unselect
    DDRA  &= ~0b00011111;
    PORTA &= ~0b00011111;
}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
