/****************************************************************************
 *
 *  LED functions
 *
 ***************************************************************************/

#include <avr/io.h>
#include <stdbool.h>
#include "stdint.h"
#include "led.h"
#include "print.h"
#include "pwm-driver.h"
#include "trackpoint.h"
#include "led-local.h"


/***************************************************************************/

// Globals:
static bool led_trackpoint_on = true;
uint8_t led_trackpoint_value = 0xff;

#ifdef LED_CONTROLLER_ENABLE
static volatile bool led_update_pending = false;
static pwm_rgb_led_t leds[ LED_ARRAY_SIZE ];
#endif

/***************************************************************************/

// Only necessary to call if LED_CONTROLLER_ENABLED is defined.
void led_init() {

    // For the LEDs that are wired directly to the Teensy, put them into
    // Hi-Z mode (DDR:0, PORT:0), regardless of whether or not the LED
    // control logic is enabled.  This is because those lines coming into
    // the Teensy will be driven at +5V, and it may need to be assured that
    // the Teensy will not also try to drive those lines at +5V.  Grounding
    // them would light the LEDs at full brightness, which is also not
    // preferred.
    DDRB  &= ~0b01110000;
    PORTB &= ~0b01110000;
    DDRC  &= ~0b01110000;
    PORTC &= ~0b01110000;

    // Configure the TrackPoint lighting:
    led_set_trackpoint( led_trackpoint_on );

#ifdef LED_CONTROLLER_ENABLE

    // Initialize PWM on the Teensy:
    led_teensy_pwm_init();

    // Initialize the external LED controller:
    bool pwm_initialized = pwm_init();

    // Configure initial LED settings:
    for ( int i = 0; i < LED_ARRAY_SIZE; i++ ) {

        pwm_rgb_led_t * led = &leds[ i ];

        led->flags = PWM_LED_FLAGS_ENABLED;
        led->channel_r = i * 3 + 0;
        led->channel_g = i * 3 + 1;
        led->channel_b = i * 3 + 2;
        led->on_r = 0;
        led->on_b = 0;
        led->on_g = 0;
        led->off_r = 0;
        led->off_b = 0;
        led->off_g = 0;

        // PWM controller only supports 5 RGB LEDs.  The rest
        // are wired to the Teensy:
        if ( i >= LED_ARRAY_FIRST_TEENSY ) {
            led->flags |= PWM_LED_FLAGS_TEENSY;
            //led->on_r = LED_TEENSY_FULL;
            led->on_r = 0x01;
            led_set_teensy_led( led );
        } else {
            //led->on_r = PWM_LED_FULL;
            led->on_r = 0x199;
            led->off_r = 0x4cc;
            pwm_set_rgb_led( led );
        }
    }

    // Program the LED controller:
    if ( pwm_initialized ) {
        pwm_commit( true );
    }

#endif
}


/***************************************************************************/

// Process keyboard LED state flags update.
// This may be called from an ISR, so pwm_commit() will not be called here
// because that function will spin when the TWI driver is busy.  Instead,
// this function will set the led_update_pending flag and we will commit
// later.
void led_set( uint8_t usb_led ) {

#ifdef LED_CONTROLLER_ENABLE
    for ( int i = 0; i < LED_ARRAY_SIZE; i++ ) {

        bool new_on;

        switch ( i ) {
            case LED_DISPLAY:
                continue;
            case LED_CAPS_LOCK_0:
            case LED_CAPS_LOCK_1:
                new_on = usb_led & ( 1 << USB_LED_CAPS_LOCK );
                break;
            case LED_NUM_LOCK_0:
            case LED_NUM_LOCK_1:
                new_on = usb_led & ( 1 << USB_LED_NUM_LOCK );
                break;
            case LED_SCROLL_LOCK_0:
            case LED_SCROLL_LOCK_1:
                new_on = usb_led & ( 1 << USB_LED_SCROLL_LOCK );
                break;
        }

        led_set_one(
            &leds[ i ], new_on,
            leds[ i ].flags & PWM_LED_FLAGS_ENABLED
        );
    }
#else
    // May be better to wire the caps lock LEDs to the Teensy.
    // TrackPoint lights act as caps lock:
    led_set_trackpoint( usb_led & ( 1 << USB_LED_CAPS_LOCK ) );
#endif
}


/***************************************************************************/

// Change an LED to reflect the given 'on' and 'enabled' settings, noting
// when further actions may be necessary to persist the changes or program
// the PWM controller.
void led_set_one( pwm_rgb_led_t * led, bool on, bool enabled ) {

    bool was_on = led->flags & PWM_LED_FLAGS_ON;
    bool was_enabled = led->flags & PWM_LED_FLAGS_ENABLED;

    // Whether a change occurred that we may want to persist:
//    bool state_changed = on == was_on && enabled == was_enabled;

    if ( on ) {
        led->flags |= PWM_LED_FLAGS_ON;
    } else {
        led->flags &= ~PWM_LED_FLAGS_ON;
    }

    // Update flags and note if there were any visual effects:
    bool effect_changed = false;
    if ( enabled ) {
        led->flags |= PWM_LED_FLAGS_ENABLED;

        if (
            ( on && ( ! was_enabled || ! was_on ) ) ||
            ( ! on && ( was_enabled && was_on ) )
        ) {
            effect_changed = true;
        }
    } else {
        led->flags &= ~PWM_LED_FLAGS_ENABLED;

        if ( was_enabled && was_on ) {
            effect_changed = true;
        }
    }

    if ( effect_changed ) {

        if ( led->flags & PWM_LED_FLAGS_TEENSY ) {

            led_set_teensy_led( led );

        } else {
            pwm_set_rgb_led( led );
            led_update_pending = true;
        }
    }
}


/***************************************************************************/

void led_set_teensy_channel( uint8_t channel, uint8_t value ) {

    switch ( channel ) {
        case LED_TEENSY_CH_B4:
            if ( value ) {
                OCR2A = value;
                DDRB |= 0b00010000;
            } else {
                DDRB &= ~0b00010000;
                PORTB &= ~0b00010000;
            }
            break;
        case LED_TEENSY_CH_B5:
            if ( value ) {
                OCR1AL = value;
                DDRB |= 0b00100000;
            } else {
                DDRB &= ~0b00100000;
                PORTB &= ~0b00100000;
            }
            break;
        case LED_TEENSY_CH_B6:
            if ( value ) {
                OCR1BL = value;
                DDRB |= 0b01000000;
            } else {
                DDRB &= ~0b01000000;
                PORTB &= ~0b01000000;
            }
            break;
        case LED_TEENSY_CH_B7:
            if ( value ) {
                OCR1CL = value;
                DDRB |= 0b10000000;
            } else {
                DDRB &= ~0b10000000;
                PORTB &= ~0b10000000;
            }
            break;
        case LED_TEENSY_CH_C4:
            if ( value ) {
                OCR3CL = value;
                DDRC |= 0b00010000;
            } else {
                DDRC &= ~0b00010000;
                PORTC &= ~0b00010000;
            }
            break;
        case LED_TEENSY_CH_C5:
            if ( value ) {
                OCR3BL = value;
                DDRC |= 0b00100000;
            } else {
                DDRC &= ~0b00100000;
                PORTC &= ~0b00100000;
            }
            break;
        case LED_TEENSY_CH_C6:
            if ( value ) {
                OCR3AL = value;
                DDRC |= 0b01000000;
            } else {
                DDRC &= ~0b01000000;
                PORTC &= ~0b01000000;
            }
            break;
    }
}


/***************************************************************************/

void led_set_teensy_led( pwm_rgb_led_t * led ) {

    if (
        led->flags & PWM_LED_FLAGS_ENABLED &&
        led->flags & PWM_LED_FLAGS_ON
    ) {
        led_set_teensy_channel( led->channel_r, led->on_r );
        led_set_teensy_channel( led->channel_g, led->on_g );
        led_set_teensy_channel( led->channel_b, led->on_b );

    } else {

        // Should disable PWM instead?
        led_set_teensy_channel( led->channel_r, 0 );
        led_set_teensy_channel( led->channel_g, 0 );
        led_set_teensy_channel( led->channel_b, 0 );
    }
}


/***************************************************************************/

// Turn the TrackPoint lights on or off.
void led_set_trackpoint( bool on ) {

    if ( on ) {
#ifdef LED_CONTROLLER_ENABLE
        led_set_teensy_channel( LED_TEENSY_CH_B7, led_trackpoint_value );
#else
        // On: Output high
        DDRB |= (1<<7);
        PORTB |= (1<<7);
#endif
    } else {
#ifdef LED_CONTROLLER_ENABLE
        led_set_teensy_channel( LED_TEENSY_CH_B7, 0 );
#else
        // Off: Hi-Z
        DDRB &= ~(1<<7);
        PORTB &= ~(1<<7);
#endif
    }

    led_trackpoint_on = on;
}


/***************************************************************************/

void led_teensy_pwm_init() {

    // Enable output drivers on PWM pins:
    DDRB |= 0b11110000;
    DDRC |= 0b01110000;

    // Configure PWM on OC1A, OC1B, and OC1C:
    //   Waveform generation mode (WGM) 5 (fast PWM mode, 8-bit),
    //   Set OC1A, OC1B, and OC1C on compare match, clear at top.
    // B7 could alternatively be driven by OCR0A, but timer 0 is used by
    // the TMK firmware for stuff.
    TCCR1A = (
        (1<<COM1A1) |
        (1<<COM1A0) |
        (1<<COM1B1) |
        (1<<COM1B0) |
        (1<<COM1C1) |
        (1<<COM1C0) |
        (0<<WGM11)  |
        (1<<WGM10)
    );
 
    // Configure PWM on OC1A, OC1B, and OC1C:
    //   Waveform generation mode (WGM) 5 (fast PWM mode, 8-bit),
    //   Select clock F_CPU / 8.
    TCCR1B = (
        (0<<ICNC1) |
        (0<<ICES1) |
        (0<<WGM13) |
        (1<<WGM12) |
        (0<<CS12)  |
        (1<<CS11)  |
        (0<<CS10)
    );

    // Configure PWM on OC2A:
    //   Waveform generation mode (WGM) 3 (fast PWM mode),
    //   Set OC0A on compare match, clear at top.
    TCCR2A = (
        (1<<COM2A1) |
        (1<<COM2A0) |
        (0<<COM2B1) |
        (0<<COM2B0) |
        (1<<WGM21)  |
        (1<<WGM20)
    );

    // Configure PWM on OC2A:
    //   Waveform generation mode (WGM) 3 (fast PWM mode),
    //   Select clock F_CPU / 8.
    TCCR2B = (
        (0<<FOC2A) |
        (0<<FOC2B) |
        (0<<WGM22) |
        (0<<CS22)  |
        (1<<CS21)  |
        (0<<CS20)
    );

    // Configure PWM on OC3A, OC3B, and OC3C:
    //   Waveform generation mode (WGM) 5 (fast PWM mode, 8-bit),
    //   Set OC3A, OC3B, and OC3C on compare match, clear at top.
    TCCR3A = (
        (1<<COM3A1) |
        (1<<COM3A0) |
        (1<<COM3B1) |
        (1<<COM3B0) |
        (1<<COM3C1) |
        (1<<COM3C0) |
        (0<<WGM31)  |
        (1<<WGM30)
    );
 
    // Configure PWM on OC3A, OC3B, and OC3C:
    //   Waveform generation mode (WGM) 5 (fast PWM mode, 8-bit),
    //   Select clock F_CPU / 8.
    TCCR3B = (
        (0<<ICNC3) |
        (0<<ICES3) |
        (0<<WGM33) |
        (1<<WGM32) |
        (0<<CS32)  |
        (1<<CS31)  |
        (0<<CS30)
    );
}

/***************************************************************************/

// Invoke this in the main keyboard loop to commit pending changes.
void led_update() {

    if ( led_update_pending ) {
        led_update_pending = false;
        pwm_commit( false );
    }
}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
