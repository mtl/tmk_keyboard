/*
Copyright 2010,2011,2012,2013 Jun WAKO <wakojun@gmail.com>

This software is licensed with a Modified BSD License.
All of this is supposed to be Free Software, Open Source, DFSG-free,
GPL-compatible, and OK to use in both free and proprietary applications.
Additions and corrections to this file are welcome.


Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the
  distribution.

* Neither the name of the copyright holders nor the names of
  contributors may be used to endorse or promote products derived
  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

/*
Primitive PS/2 Library for AVR
==============================
Host side is only supported now.
Synchronous USART is used to receive data by hardware process
rather than interrupt. During V-USB interrupt runs, CLOCK interrupt
cannot interpose. In the result it is prone to lost CLOCK edge.


I/O control
-----------
High state is asserted by internal pull-up.
If you have a signaling problem, you may need to have
external pull-up resisters on CLOCK and DATA line.


PS/2 References
---------------
http://www.computer-engineering.org/ps2protocol/
http://www.mcamafia.de/pdf/ibm_hitrc07.pdf
*/
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "ps2.h"
#include "debug.h"


#define WAIT(stat, us, err) do { \
    if (!wait_##stat(us)) { \
        ps2_error = err; \
        goto ERROR; \
    } \
} while (0)


uint8_t ps2_error = PS2_ERR_NONE;


static inline void clock_lo(void);
static inline void clock_hi(void);
static inline bool clock_in(void);
static inline void data_lo(void);
static inline void data_hi(void);
static inline bool data_in(void);
static inline uint16_t wait_clock_lo(uint16_t us);
static inline uint16_t wait_clock_hi(uint16_t us);
static inline uint16_t wait_data_lo(uint16_t us);
static inline uint16_t wait_data_hi(uint16_t us);
static inline void idle(void);
static inline void inhibit(void);
static inline uint8_t pbuf_dequeue(void);
static inline void pbuf_enqueue(uint8_t data);
static inline bool pbuf_has_data(void);
static inline void pbuf_clear(void);


void ps2_host_init(void)
{
    idle();
    PS2_USART_INIT();
    PS2_USART_RX_INT_ON();
    //PS2_USART_TX_INT_ON();
}

//---------------------------------------------------------------------------

/*
ISR(PS2_USART_TX_VECT) {
	print( "PS2_USART_TX_VECT\n" );
}

bool ps2_host_send_ready() {
    return PS2_USART_TX_EMPTY;
}


void ps2_host_send_start(uint8_t data) {
    
	print( "Waiting for send ready.\n" );
	while ( ! ps2_host_send_ready() );
	print( "Initiating send.\n" );

    inhibit();
    _delay_us(100);

	// Request to send:
    data_lo();
    _delay_us(52);
    clock_hi();
//    WAIT(clock_lo, 15000, 1);
//    _delay_us(15);

	// Initiate transmission:
    PS2_USART_DATA = data;
}


uint8_t ps2_host_send2(uint8_t data) {

	ps2_host_send_start( data );
	print( "Waiting for receipt.\n" );
	while ( pbuf_empty() ) {
		;
	}
	uint8_t res = ps2_host_recv();

	print( "Sent " );
	phex( data );
	print( ", received " );
	phex( res );
	print( "\n" );

	return res;
}
*/
//---------------------------------------------------------------------------

uint8_t ps2_host_send(uint8_t data)
{
    uint8_t res = 0;
    bool parity = true;
    ps2_error = PS2_ERR_NONE;

    PS2_USART_OFF();

    /* terminate a transmission if we have */
    inhibit();
    _delay_us(100);

    /* start bit [1] */
    data_lo();
    clock_hi();
    WAIT(clock_lo, 15000, 1);
    /* data [2-9] */
    for (uint8_t i = 0; i < 8; i++) {
        _delay_us(15);
        if (data&(1<<i)) {
            parity = !parity;
            data_hi();
        } else {
            data_lo();
        }
        WAIT(clock_hi, 50, 2);
        WAIT(clock_lo, 50, 3);
    }
    /* parity [10] */
    _delay_us(15);
    if (parity) { data_hi(); } else { data_lo(); }
    WAIT(clock_hi, 50, 4);
    WAIT(clock_lo, 50, 5);
    /* stop bit [11] */
    _delay_us(15);
    data_hi();
    /* ack [12] */
    WAIT(data_lo, 50, 6);
    WAIT(clock_lo, 50, 7);

    /* wait for idle state */
    WAIT(clock_hi, 50, 8);
    WAIT(data_hi, 50, 9);

    PS2_USART_INIT();
    PS2_USART_RX_INT_ON();
    res = ps2_host_recv_response();
ERROR:
    idle();
    PS2_USART_INIT();
    PS2_USART_RX_INT_ON();
    return res;
}

// Do polling data from keyboard to get response to last command.
uint8_t ps2_host_recv_response(void)
{
    while (!pbuf_has_data()) {
        _delay_ms(1);   // without this delay it seems to fall into deadlock
    }
    return pbuf_dequeue();
}

uint8_t ps2_host_recv(void)
{
    return pbuf_dequeue();
}

ISR(PS2_USART_RX_VECT)
{
    uint8_t error = PS2_USART_ERROR;
    uint8_t data = PS2_USART_RX_DATA;
    if (!error) {
        pbuf_enqueue(data);
    }
}

/* send LED state to keyboard */
void ps2_host_set_led(uint8_t led)
{
    // send 0xED then keyboard keeps waiting for next LED data
    // and keyboard does not send any scan codes during waiting.
    // If fail to send LED data keyboard looks like being freezed.
    uint8_t retry = 3;
    while (retry-- && ps2_host_send(PS2_SET_LED) != PS2_ACK)
        ;
    retry = 3;
    while (retry-- && ps2_host_send(led) != PS2_ACK)
        ;
}


/*--------------------------------------------------------------------
 * static functions
 *------------------------------------------------------------------*/
static inline void clock_lo()
{
    PS2_CLOCK_PORT &= ~(1<<PS2_CLOCK_BIT);
    PS2_CLOCK_DDR  |=  (1<<PS2_CLOCK_BIT);
}
static inline void clock_hi()
{
    /* input with pull up */
    PS2_CLOCK_DDR  &= ~(1<<PS2_CLOCK_BIT);
    PS2_CLOCK_PORT |=  (1<<PS2_CLOCK_BIT);
}
static inline bool clock_in()
{
    PS2_CLOCK_DDR  &= ~(1<<PS2_CLOCK_BIT);
    PS2_CLOCK_PORT |=  (1<<PS2_CLOCK_BIT);
    _delay_us(1);
    return PS2_CLOCK_PIN&(1<<PS2_CLOCK_BIT);
}
static inline void data_lo()
{
    PS2_DATA_PORT &= ~(1<<PS2_DATA_BIT);
    PS2_DATA_DDR  |=  (1<<PS2_DATA_BIT);
}
static inline void data_hi()
{
    /* input with pull up */
    PS2_DATA_DDR  &= ~(1<<PS2_DATA_BIT);
    PS2_DATA_PORT |=  (1<<PS2_DATA_BIT);
}
static inline bool data_in()
{
    PS2_DATA_DDR  &= ~(1<<PS2_DATA_BIT);
    PS2_DATA_PORT |=  (1<<PS2_DATA_BIT);
    _delay_us(1);
    return PS2_DATA_PIN&(1<<PS2_DATA_BIT);
}

static inline uint16_t wait_clock_lo(uint16_t us)
{
    while (clock_in()  && us) { asm(""); _delay_us(1); us--; }
    return us;
}
static inline uint16_t wait_clock_hi(uint16_t us)
{
    while (!clock_in() && us) { asm(""); _delay_us(1); us--; }
    return us;
}
static inline uint16_t wait_data_lo(uint16_t us)
{
    while (data_in() && us)  { asm(""); _delay_us(1); us--; }
    return us;
}
static inline uint16_t wait_data_hi(uint16_t us)
{
    while (!data_in() && us)  { asm(""); _delay_us(1); us--; }
    return us;
}

/* idle state that device can send */
static inline void idle(void)
{
    clock_hi();
    data_hi();
}

/* inhibit device to send */
static inline void inhibit(void)
{
    clock_lo();
    data_hi();
}


/*--------------------------------------------------------------------
 * Ring buffer to store scan codes from keyboard
 *------------------------------------------------------------------*/
#define PBUF_SIZE 32
static uint8_t pbuf[PBUF_SIZE];
static uint8_t pbuf_head = 0;
static uint8_t pbuf_tail = 0;
static inline void pbuf_enqueue(uint8_t data)
{
    uint8_t sreg = SREG;
    cli();
    uint8_t next = (pbuf_head + 1) % PBUF_SIZE;
    if (next != pbuf_tail) {
        pbuf[pbuf_head] = data;
        pbuf_head = next;
    } else {
        debug("pbuf: full\n");
    }
    SREG = sreg;
}

static inline uint8_t pbuf_dequeue(void)
{
    uint8_t val = 0;

    uint8_t sreg = SREG;
    cli();
    if (pbuf_head != pbuf_tail) {
        val = pbuf[pbuf_tail];
        pbuf_tail = (pbuf_tail + 1) % PBUF_SIZE;
    }
    SREG = sreg;

    return val;
}
static inline bool pbuf_has_data(void)
{
    uint8_t sreg = SREG;
    cli();
    bool has_data = (pbuf_head != pbuf_tail);
    SREG = sreg;
    return has_data;
}
static inline void pbuf_clear(void)
{
    uint8_t sreg = SREG;
    cli();
    pbuf_head = pbuf_tail = 0;
    SREG = sreg;
}
