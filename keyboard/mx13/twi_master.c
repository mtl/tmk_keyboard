/* =============================================================================

    Copyright (c) 2006 Pieter Conradie [www.piconomic.co.za]
    All rights reserved.
    
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

    Title:          Interrupt-driven TWI (I2C) Master
    Author(s):      Pieter Conradie
    Creation Date:  2007-03-31
    Revision Info:  $Id: twi_master.c 117 2010-06-24 20:21:28Z pieterconradie $

============================================================================= */

/* _____STANDARD INCLUDES____________________________________________________ */
#include <compat/twi.h>
#include <avr/interrupt.h>

#include <util/delay.h>
#include "print.h"

/* _____PROJECT INCLUDES_____________________________________________________ */
#include "twi_master.h"

/* _____LOCAL DEFINITIONS____________________________________________________ */
/// Size TWI clock prescaler according to desired TWI clock frequency
#define TWI_PRESCALER    ((0<<TWPS1)|(0<<TWPS0))

/// Map TWI clock prescaler to prescaler value
#if   (TWI_PRESCALER == ((0<<TWPS1)|(0<<TWPS0)))
#define TWI_PRESCALER_VALUE 1
#elif (TWI_PRESCALER == ((0<<TWPS1)|(1<<TWPS0)))
#define TWI_PRESCALER_VALUE 4
#elif (TWI_PRESCALER == ((1<<TWPS1)|(0<<TWPS0)))
#define TWI_PRESCALER_VALUE 16
#elif (TWI_PRESCALER == ((1<<TWPS1)|(1<<TWPS0)))
#define TWI_PRESCALER_VALUE 64
#else
#error "Undefined TWI_PRESCALER value!"
#endif

/// Calculate TWI Baud rate value according to selected frequency and prescaler
#define TWI_BR_VALUE ((DIV_ROUND(F_CPU,TWI_FREQUENCY_HZ)-16ul)/(2ul*TWI_PRESCALER_VALUE))

/// Make sure that TWI Baud rate value is an 8-bit value
#ifndef __DOXYGEN__
#if (TWI_BR_VALUE > 255)
#error "Make TWI_PRESCALER_VALUE larger!"
#endif
#endif

/// TWI State machine value when finished
#define TWI_STATUS_DONE 0xff

/* _____LOCAL VARIABLES______________________________________________________ */
static uint8_t twi_adr;
static uint8_t *twi_data;
static uint8_t twi_data_counter;
static uint8_t twi_status;
static bool initialized = false;

/* _____PRIVATE FUNCTIONS____________________________________________________ */
/// TWI state machine interrupt handler
ISR(TWI_vect)
{
    switch(TW_STATUS)
    {
    case TW_START:

        // START has been transmitted
        // Fall through...

    case TW_REP_START:

        // REPEATED START has been transmitted

        // Load data register with TWI slave address
        TWDR = twi_adr;
        // TWI Interrupt enabled and clear flag to send next byte
        TWCR = (1<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(1<<TWIE);
        break;

    case TW_MT_SLA_ACK:

        // SLA+W has been tramsmitted and ACK received
        // Fall through...

    case TW_MT_DATA_ACK:

        // Data byte has been tramsmitted and ACK received
        if(twi_data_counter != 0)
        {
            // Decrement counter
            twi_data_counter--;
            // Load data register with next byte
            TWDR = *twi_data++;
            // TWI Interrupt enabled and clear flag to send next byte
            TWCR = (1<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(1<<TWIE);
        }
        else
        {
#if TWI_REPEATED_START_SUPPORT
            // Disable TWI Interrupt
            TWCR = (0<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(0<<TWIE);
#else
            // Initiate STOP condition after last byte; TWI Interrupt disabled
            TWCR = (1<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(1<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(0<<TWIE);
#endif
            // Transfer finished
            twi_status = TWI_STATUS_DONE;
        }
        break;

    case TW_MR_DATA_ACK:

        // Data byte has been received and ACK tramsmitted
        // Buffer received byte
        *twi_data++ = TWDR;
        // Decrement counter
        twi_data_counter--;
        // Fall through...

    case TW_MR_SLA_ACK:

        // SLA+R has been transmitted and ACK received
        // See if last expected byte will be received ...
        if(twi_data_counter > 1)
        {
            // Send ACK after reception
            TWCR = (1<<TWINT)|(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(1<<TWIE);
        }
        else
        {
            // Send NACK after next reception
            TWCR = (1<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(1<<TWIE);
        }
        break;

    case TW_MR_DATA_NACK:

        // Data byte has been received and NACK tramsmitted
        // Buffer received byte
        *twi_data++ = TWDR;
        // Decrement counter
        twi_data_counter--;

#if TWI_REPEATED_START_SUPPORT
        // Disable TWI Interrupt
        TWCR = (0<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(0<<TWIE);
#else
        // Initiate STOP condition after last byte; TWI Interrupt disabled
        TWCR = (1<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(1<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(0<<TWIE);
#endif
        // Transfer finished
        twi_status = TWI_STATUS_DONE;
        break;

    case TW_MT_ARB_LOST:

        // Arbitration lost...
        // Initiate a (REPEATED) START condition; Interrupt enabled and flag cleared
        TWCR = (1<<TWINT)|(0<<TWEA)|(1<<TWSTA)|(0<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(1<<TWIE);
        break;

    default:

        // Error condition; save status
        twi_status = TWSR;
        // Reset TWI Interface; disable interrupt
        TWCR = (0<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(0<<TWIE);
    }
}

/* _____FUNCTIONS_____________________________________________________ */
void twi_init(void)
{
	if ( initialized ) return;

    DDRD  &= ~0b00000011; // Configure SDA and SDL as inputs
    PORTD |=  0b00000011; // Enable internal pull-ups on SDA and SCL
    //PORTD &= ~0b00000011; // Enable Hi-Z on SDA and SCL
    _delay_us(1);

    // Initialise variable
    twi_data_counter = 0;
	//twi_status = TWI_STATUS_DONE; // mtl

    // Initialize TWI clock
    TWSR = TWI_PRESCALER;
    TWBR = TWI_BR_VALUE;

    // Load data register with default content; release SDA
    //TWDR = 0xff;

    // Enable TWI peripheral with interrupt disabled
    TWCR = (0<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(0<<TWIE);

	initialized = true;
}

void twi_start_tx(uint8_t adr, uint8_t *data, uint8_t bytes_to_send)
{
    // Wait for previous transaction to finish
    while(twi_busy())
    {
        ;
    }

    // Copy address; clear R/~W bit in SLA+R/W address field
    //twi_adr = adr & ~TW_READ;
    twi_adr = adr << 1;

    // Save pointer to data and number of bytes to send
    twi_data        = data;
    twi_data_counter = bytes_to_send;

    // Initiate a START condition; Interrupt enabled and flag cleared
    TWCR = (1<<TWINT)|(0<<TWEA)|(1<<TWSTA)|(0<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(1<<TWIE);
}

void twi_start_rx(uint8_t adr, uint8_t *data, uint8_t bytes_to_receive)
{
    // Wait for previous transaction to finish
    while(twi_busy())
    {
        ;
    }

    // Copy address; set R/~W bit in SLA+R/W address field
    //twi_adr = adr | TW_READ;
    twi_adr = ( adr << 1 ) | TW_READ;

    // Save pointer to data and number of bytes to receive
    twi_data         = data;
    twi_data_counter = bytes_to_receive;

    // Initiate a START condition; Interrupt enabled and flag cleared
    TWCR = (1<<TWINT)|(0<<TWEA)|(1<<TWSTA)|(0<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(1<<TWIE);
}

bool twi_busy(void)
{
    // IF TWI Interrupt is enabled then the peripheral is busy
    if(BIT_IS_HI(TWCR,TWIE))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool twi_success(void)
{
    if(twi_status == TWI_STATUS_DONE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint8_t twi_get_status(void)
{
    return twi_status;
}

#if TWI_REPEATED_START_SUPPORT
void twi_stop(void)
{
    // Wait for transaction to finish
    while(twi_busy())
    {
        ;
    }

    // Make sure transaction was succesful
    if(twi_status != TWI_STATUS_DONE)
    {
        return;
    }

    // Initiate a STOP condition
    TWCR = (1<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(1<<TWSTO)|(0<<TWWC)|(1<<TWEN)|(0<<TWIE);

    // Wait until STOP has finished
    LOOP_UNTIL_BIT_IS_LO(TWCR, TWSTO);
}
#endif

/**
 *  @}
 */

/* _____LOG__________________________________________________________________ */
/*

 2007-03-31 : Pieter Conradie
 - First release
   
*/
