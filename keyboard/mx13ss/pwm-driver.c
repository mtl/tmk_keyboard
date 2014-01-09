/****************************************************************************
 *
 *  Adafruit PWM servo driver support
 *  http://www.adafruit.com/products/815
 *
 ***************************************************************************/

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "pwm-driver.h"
#include "twi_master.h"
#include "print.h"
#include "debug.h"

#ifndef PWM_DRIVER_ADDRESS
#define PWM_DRIVER_ADDRESS 0x40
#endif


/***************************************************************************/

// Globals:
static bool initialized = false;
static uint8_t message[ PWM_MAX_MESSAGE ];


/***************************************************************************/

bool pwm_commit( bool wait ) {

    if ( ! initialized ) return false;

    twi_start_tx( PWM_DRIVER_ADDRESS, message, PWM_MAX_MESSAGE );

    if ( wait ) {
        while ( twi_busy() );
        if ( ! twi_success() ) {
            print( "PWM: Commit failed: " );
            phex( twi_get_status() );
            print( "\n" );
            return false;
        }
    }

    return true;
}


/***************************************************************************/

// Get PWM prescaler setting for @param(update_rate) Hz.
uint8_t pwm_get_prescale_value( float update_rate ) {

    float pv = PWM_OSC_CLOCK;
    pv = PWM_OSC_CLOCK / ( 4096 * update_rate );
    uint8_t prescale_value = floor( pv + 0.5 ) - 1;

    return prescale_value < PWM_MIN_PRESCALE ? PWM_MIN_PRESCALE : prescale_value;
}


/***************************************************************************/

bool pwm_init() {

    twi_init();
    initialized = true;
    if ( ! pwm_reset() ) {
        initialized = false;
        return false;
    }

    // Set prescaler:
    if ( ! pwm_set_prescaler( PWM_MIN_PRESCALE ) ) {
        print( "PWM: Failed to set prescaler.\n" );
        initialized = false;
        return false;
    }

    // Initialize bulk register write message buffer:
    message[ 0 ] = PWM_LED0_ON_L;  // First register to write, then auto-increment
    for ( int i = 1; i < PWM_MAX_MESSAGE; i++ ) {
        message[ i ] = 0;
    }

    return true;
}


/***************************************************************************/

// Read an 8-bit register:
bool pwm_read_register( uint8_t reg, uint8_t * value ) {

    if ( ! initialized ) return false;

//    for ( int i = 0; i < 10; i++ ) {
//        print( "Reading register 1: " );
//        phex( reg );
//        print( "\n" );
//        _delay_ms( 100 );
//    }

    // Select the register to read:
    twi_start_tx( PWM_DRIVER_ADDRESS, &reg, 1 );
    while ( twi_busy() );
    if ( ! twi_success() ) {
        print( "PWM: Failed to select register for read: " );
        phex( twi_get_status() );
        print( "\n" );
        return false;
    }

    // Read the register:
    twi_start_rx( PWM_DRIVER_ADDRESS, value, 1 );
    while ( twi_busy() );
    if ( ! twi_success() ) {
        print( "PWM: Failed to read register: " );
        phex( twi_get_status() );
        print( "\n" );
        return false;
    }

//    for ( int i = 0; i < 10; i++ ) {
//        print( "Reading register 2: " );
//        phex( *value );
//        print( "\n" );
//        _delay_ms( 100 );
//    }

    return true;
}


/***************************************************************************/

bool pwm_reset() {

    // Set MODE1 to known values:
    pwm_write_register( PWM_MODE1, 0x0, true );

    if ( ! twi_success() ) {
        print( "PWM reset failed: " );
        phex( twi_get_status() );
        print( "\n" );
        return false;
    }

    uint8_t mode_2;
    if ( ! pwm_read_register( PWM_MODE2, &mode_2 ) ) {
        return false;
    }
    if (
        ! pwm_write_register(
            PWM_MODE2,
            mode_2 | (1<<PWM_INVRT) | (1<<PWM_OCH) | (0<<PWM_OUTDRV),
            true
        )
    ) {
        return false;
    }

    return true;
}


/***************************************************************************/

void pwm_rgb_led_off( pwm_rgb_led_t * led ) {

    led->flags &= ~PWM_LED_FLAGS_ON;
    pwm_set_rgb_led( led );
}


/***************************************************************************/

void pwm_rgb_led_on( pwm_rgb_led_t * led ) {

    led->flags |= PWM_LED_FLAGS_ON;
    pwm_set_rgb_led( led );
}


/***************************************************************************/

void pwm_rgb_led_set_percent( pwm_rgb_led_t * led, int color, int percent ) {

    if ( led->flags & PWM_LED_FLAGS_TEENSY ) {

        uint8_t value = 0;
        if ( percent >= 100 ) {
            value = 0xff;
        } else if ( percent ) {
            value = ( percent * 0xff ) / 100;
            if ( value > 0xff ) value = 0xff;
            if ( ! value ) value = 1;
        }
        led->values[ color ] = value;

    } else {
        uint16_t on_value = 0;
        uint32_t off_value = 0;

        if ( ! percent ) {
            off_value = PWM_LED_FULL;
        } else if ( percent >= 100 ) {
            on_value = PWM_LED_FULL;
        } else {
            off_value = 4095;
            off_value *= percent;
            off_value /= 100;
            off_value += 1;
            if ( off_value > 0xfff ) off_value = 0xfff;
        }
        led->values[ color + 0 ] = on_value;
        led->values[ color + 1 ] = (uint16_t) off_value;
    }
}


/***************************************************************************/

void pwm_set_channel( uint8_t channel, uint16_t on, uint16_t off ) {

    int offset = 1 + ( channel << 2 );
    message[ offset + 0 ] = (uint8_t) on;
    message[ offset + 1 ] = (uint8_t) ( on >> 8 );
    message[ offset + 2 ] = (uint8_t) off;
    message[ offset + 3 ] = (uint8_t) ( off >> 8 );
}


/***************************************************************************/

// Set PWM prescaler to the given value.
bool pwm_set_prescaler( uint8_t prescale_value ) {

    if ( ! initialized ) return false;

    uint8_t mode_1;
    if ( ! pwm_read_register( PWM_MODE1, &mode_1 ) ) {
        return false;
    }

    // Put the device to sleep:
    if (
        ! pwm_write_register(
            PWM_MODE1,
            ( mode_1 & ~PWM_RESTART ) | (1<<PWM_SLEEP),
            true
        )
    ) {
        return false;
    }

    // Set prescaler:
    if ( ! pwm_write_register( PWM_PRE_SCALE, prescale_value, true ) ) {
        return false;
    }

    // Wake up:
    if ( ! pwm_write_register( PWM_MODE1, mode_1, true ) ) {
        return false;
    }

    // Allow time for oscillator to stabilize after wake-up:
    _delay_us( 500 );

    // Restart PWM channels after coming out of sleep, and enable
    // auto-increment mode.  The latter should really go in pwm_reset(),
    // but here it saves having to re-read the MODE1 register.
    if (
        ! pwm_write_register(
            PWM_MODE1,
//            mode_1 | (0xa1),
            mode_1 | (1<<PWM_RESTART) | (1<<PWM_AI) | (1<<PWM_ALLCALL),
            true
        )
    ) {
        return false;
    }

    return true;
}


/***************************************************************************/

void pwm_set_rgb_led( pwm_rgb_led_t * led ) {

    if (
        led->flags & PWM_LED_FLAGS_ENABLED &&
        led->flags & PWM_LED_FLAGS_ON
    ) {
        for ( int ch = 0; ch < 3; ch++ ) {
            pwm_set_channel(
                led->channels[ ch ],
                led->values[ ch * 2 + 0 ], 
                led->values[ ch * 2 + 1 ] 
            );
        }
    } else {
        for ( int ch = 0; ch < 3; ch++ ) {
            pwm_set_channel( led->channels[ ch ], 0, PWM_LED_FULL );
        }
    }
}


/***************************************************************************/

void pwm_toggle_rgb_led( pwm_rgb_led_t * led ) {

    if ( led->flags & PWM_LED_FLAGS_ON ) {
        pwm_rgb_led_off( led );
    } else {
        pwm_rgb_led_on( led );
    }
}


/***************************************************************************/

// Write an 8-bit register:
bool pwm_write_register( uint8_t reg, uint8_t value, bool wait ) {

    if ( ! initialized ) return false;

    // Send message the PWM driver:
    uint8_t msg[ 2 ];
    msg[ 0 ] = reg;
    msg[ 1 ] = value;

//    for ( int i = 0; i < 10; i++ ) {
//        print( "Writing register 1: " );
//        phex( reg );
//        print( ", " );
//        phex( value );
//        print( "\n" );
//        _delay_ms( 100 );
//    }

    twi_start_tx( PWM_DRIVER_ADDRESS, msg, 2 );

    if ( wait ) {
        while ( twi_busy() ) {

            /*uint8_t status = twi_get_status();*/
            /*print( "Write status: " );*/
            /*phex( status );*/
            /*print( ", TWSR=" );*/
            /*phex( TWSR );*/
            /*print( ", TWCR=" );*/
            /*phex( TWCR );*/
            /*print( ", SREG=" );*/
            /*phex( SREG );*/
            /*print( ", D=" );*/
            /*phex( PIND & 3 );*/
            /*print( "\n" );*/
            /*_delay_ms( 100 );*/

            ;
        }

        if ( ! twi_success() ) {
            print( "PWM: Failed to write register: " );
            phex( twi_get_status() );
            print( "\n" );
            return false;
        }
    }

    return true;
}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
