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
/***************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H


/***************************************************************************/

// USB Device descriptor parameters:
#define VENDOR_ID       0x04b3
#define PRODUCT_ID      0x0013
#define DEVICE_VER      0x0001
#define MANUFACTURER    mtl
#define PRODUCT         MX13 SpaceSaver Keyboard
#define DESCRIPTION     t.m.k. keyboard firmware for MX13 SpaceSaver Keyboard


/***************************************************************************/

#define MATRIX_ROWS 5
#define MATRIX_COLS 19
#define DEBOUNCE    5

// Mechanical locking support.
// Use KC_LCAP, KC_LNUM or KC_LSCR instead in keymap.
#define LOCKING_SUPPORT_ENABLE

// Locking resynchronize hack:
#define LOCKING_RESYNC_ENABLE

// Key combination for command:
#define IS_COMMAND() ( \
    keyboard_report->mods == ( MOD_BIT( KC_LSHIFT ) | MOD_BIT( KC_RSHIFT ) ) \
)

// Configurable middle-button-scroll (defined in trackpoint.c).
// 0x80 is the default TrackPoint sensitivity.
#define PS2_MOUSE_SCROLL_DIVISOR_H ( ( tp_sensitivity * tp_scroll_divisor_h ) / 0x80 )
#define PS2_MOUSE_SCROLL_DIVISOR_V ( ( tp_sensitivity * tp_scroll_divisor_v ) / 0x80 )
extern int tp_sensitivity;
extern int tp_scroll_divisor_h; // [0,16256]
extern int tp_scroll_divisor_v; // [0,16256]


/***************************************************************************/
// Feature disable options.
//   These options are also useful to firmware size reduction.

/* disable debug print */
//#define NO_DEBUG

/* disable print */
//#define NO_PRINT

/* disable action features */
//#define NO_ACTION_LAYER
//#define NO_ACTION_TAPPING
//#define NO_ACTION_ONESHOT
//#define NO_ACTION_MACRO
//#define NO_ACTION_FUNCTION


/***************************************************************************/

#ifdef PS2_USE_USART

/* XCK for clock line and RXD for data line */
#define PS2_CLOCK_PORT  PORTD
#define PS2_CLOCK_PIN   PIND
#define PS2_CLOCK_DDR   DDRD
#define PS2_CLOCK_BIT   5
#define PS2_DATA_PORT   PORTD
#define PS2_DATA_PIN    PIND
#define PS2_DATA_DDR    DDRD
#define PS2_DATA_BIT    2

/*
#define PS2_USART_TX_VECT       USART1_TX_vect
#define PS2_USART_TX_DATA       UDR1
#define PS2_USART_TX_EMPTY      (!(UCSR1A & (1<<UDRE1)))
#define PS2_USART_TX_COMPLETE   (UCSR1A & (1<<TXC1))
#define PS2_USART_TX_INT_ON() do {  \
    UCSR1B |= ((1 << TXCIE1) |      \
              (1 << TXEN1));        \
} while (0)
//    UCSR1B &= ~(1 << UDRIE1)        \
*/

/* synchronous, odd parity, 1-bit stop, 8-bit data, sample at falling edge */
/* set DDR of CLOCK as input to be slave */
#define PS2_USART_INIT() do {   \
    PS2_CLOCK_DDR &= ~(1<<PS2_CLOCK_BIT);   \
    PS2_DATA_DDR &= ~(1<<PS2_DATA_BIT);     \
    UCSR1C = ((1 << UMSEL10) |  \
              (3 << UPM10)   |  \
              (0 << USBS1)   |  \
              (3 << UCSZ10)  |  \
              (0 << UCPOL1));   \
    UCSR1A = 0;                 \
    UBRR1H = 0;                 \
    UBRR1L = 0;                 \
} while (0)
#define PS2_USART_RX_INT_ON() do {  \
    UCSR1B = ((1 << RXCIE1) |       \
              (1 << RXEN1));        \
} while (0)
#define PS2_USART_RX_POLL_ON() do { \
    UCSR1B = (1 << RXEN1);          \
} while (0)
#define PS2_USART_OFF() do {    \
    UCSR1C = 0;                 \
    UCSR1B &= ~((1 << RXEN1) |  \
                (1 << TXEN1));  \
} while (0)
#define PS2_USART_RX_READY      (UCSR1A & (1<<RXC1))
#define PS2_USART_RX_DATA       UDR1
#define PS2_USART_ERROR         (UCSR1A & ((1<<FE1) | (1<<DOR1) | (1<<UPE1)))
#define PS2_USART_RX_VECT       USART1_RX_vect
#endif

/*
#ifdef PS2_USE_INT
// uses INT1 for clock line(ATMega32U4)
#define PS2_CLOCK_PORT  PORTD
#define PS2_CLOCK_PIN   PIND
#define PS2_CLOCK_DDR   DDRD
#define PS2_CLOCK_BIT   1
#define PS2_DATA_PORT   PORTD
#define PS2_DATA_PIN    PIND
#define PS2_DATA_DDR    DDRD
#define PS2_DATA_BIT    2

#define PS2_INT_INIT()  do {    \
    EICRA |= ((1<<ISC11) |      \
              (0<<ISC10));      \
} while (0)
#define PS2_INT_ON()  do {      \
    EIMSK |= (1<<INT1);         \
} while (0)
#define PS2_INT_OFF() do {      \
    EIMSK &= ~(1<<INT1);        \
} while (0)
#define PS2_INT_VECT    INT1_vect
#endif
*/

/*
#ifdef PS2_USE_BUSYWAIT
#define PS2_CLOCK_PORT  PORTA
#define PS2_CLOCK_PIN   PINA
#define PS2_CLOCK_DDR   DDRA
#define PS2_CLOCK_BIT   6
#define PS2_DATA_PORT   PORTA
#define PS2_DATA_PIN    PINA
#define PS2_DATA_DDR    DDRA
#define PS2_DATA_BIT    5
#endif
*/

/*
#ifdef PS2_MOUSE_ENABLE
#   define PS2_CLOCK_PORT  PORTA
#   define PS2_CLOCK_PIN   PINA
#   define PS2_CLOCK_DDR   DDRA
#   define PS2_CLOCK_BIT   6
#   define PS2_DATA_PORT   PORTA
#   define PS2_DATA_PIN    PINA
#   define PS2_DATA_DDR    DDRA
#   define PS2_DATA_BIT    5
#endif
*/


/***************************************************************************/

#endif

/* vi: set et sts=4 sw=4 ts=4: */
