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
#define PWM_DRIVER_ADDRESS 0x80
#endif


/***************************************************************************/

// Globals:
static uint8_t message[ PWM_MAX_MESSAGE ];
static bool initialized = false;

/***************************************************************************/

bool pwm_init( float freq ) {

    twi_init();
    initialized = true;
    if ( ! pwm_reset() ) {
        initialized = false;
        return false;
    }

    if ( ! pwm_set_freq( freq ) ) {
        print( "PWM: Failed to set prescaler frequency.\n" );
        initialized = false;
        return false;
    }

    for ( int i = 0; i < PWM_MAX_MESSAGE; i++ ) {
        message[ i ] = 0;
    }

    return true;
}


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

void pwm_disable_rgb_led( pwm_rgb_led_t * led ) {

    led->enabled = false;
    pwm_set_rgb_led( led );
}


/***************************************************************************/

void pwm_enable_rgb_led( pwm_rgb_led_t * led ) {
    led->enabled = true;
    pwm_set_rgb_led( led );
}


/***************************************************************************/

// Read an 8-bit register:
bool pwm_read_register( uint8_t reg, uint8_t * value ) {

    if ( ! initialized ) return false;

    print( "Reading register 1: " );
    phex( reg );
    print( "\n" );
    _delay_ms( 100 );

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

    print( "Reading register 2: " );
    phex( value );
    print( "\n" );
    _delay_ms( 100 );

    return true;
}


/***************************************************************************/

bool pwm_reset() {

    pwm_write_register( PWM_PCA9685_MODE1, 0x0, true );

    if ( ! twi_success() ) {
        print( "PWM reset failed: " );
        phex( twi_get_status() );
        print( "\n" );
        return false;
    }
    return true;
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

// Set PWM frequency to @param(freq) Hz.
bool pwm_set_freq( float freq ) {

    if ( ! initialized ) return false;

    float prescaleval = 25000000; // 25 MHz
    prescaleval /= 4096; // 12-bit
    prescaleval /= freq;
    prescaleval -= 1;
    uint8_t prescale = floor( prescaleval + 0.5 );
    print( "PWM: Final pre-scale: " );
    phex( prescale );
    print( "\n" );

    uint8_t mode;
    if ( ! pwm_read_register( PWM_PCA9685_MODE1, &mode ) ) {
        return false;
    }

    // Put the device to sleep:
    if (
        ! pwm_write_register(
            PWM_PCA9685_MODE1,
            ( mode & 0x7F ) | 0x10,
            true
        )
    ) {
        return false;
    }

    // Set prescaler:
    if ( ! pwm_write_register( PWM_PCA9685_PRESCALE, prescale, true ) ) {
        return false;
    }

    // Wake up:
    if ( ! pwm_write_register( PWM_PCA9685_MODE1, mode, true ) ) {
        return false;
    }

    // Enable auto-increment.  This should really go in pwm_reset(),
    // but here it saves having to re-read the MODE1 register.
    _delay_ms( 5 );
    if ( ! pwm_write_register( PWM_PCA9685_MODE1, mode | 0xa1, true ) ) {
        return false;
    }

    return true;
}


/***************************************************************************/

void pwm_set_rgb_led( pwm_rgb_led_t * led ) {

    if ( led->enabled ) {
        pwm_set_channel( led->channel_r, led->on_r, led->off_r );
        pwm_set_channel( led->channel_g, led->on_g, led->off_g );
        pwm_set_channel( led->channel_b, led->on_b, led->off_b );
    } else {
        pwm_set_channel( led->channel_r, 0, PWM_LED_FULL );
        pwm_set_channel( led->channel_g, 0, PWM_LED_FULL );
        pwm_set_channel( led->channel_b, 0, PWM_LED_FULL );
    }
}


/***************************************************************************/

void pwm_toggle_rgb_led( pwm_rgb_led_t * led ) {
    if ( led->enabled ) {
        pwm_disable_rgb_led( led );
    } else {
        pwm_enable_rgb_led( led );
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

    print( "Writing register 1: " );
    phex( reg );
    print( ", " );
    phex( value );
    print( "\n" );
    _delay_ms( 100 );

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
