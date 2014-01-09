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
#include "timer.h"


/***************************************************************************/

// Globals:
static bool led_trackpoint_on = true;
uint8_t led_trackpoint_value = 0x11;

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

    // Initialize PWM on the Teensy:
    led_teensy_pwm_init();

    // Configure the TrackPoint lighting:
    led_set_trackpoint( led_trackpoint_on );

#ifdef LED_CONTROLLER_ENABLE

    // Initialize the external LED controller:
    bool pwm_initialized = pwm_init();

#endif

    // Configure initial LED settings:
    for ( int i = 0; i < LED_ARRAY_SIZE; i++ ) {

        pwm_rgb_led_t * led = &leds[ i ];

        led->flags = PWM_LED_FLAGS_ENABLED;
        for ( int ch = 0; ch < 3; ch++ ) {
            led->channels[ ch ] = i * 3 + ch;
        }
        for ( int v = 0; v < 6; v++ ) {
            led->values[ v ] = 0;
        }

        // PWM controller only supports 5 RGB LEDs.  The rest
        // are wired to the Teensy:
        if ( i >= LED_ARRAY_FIRST_TEENSY ) {
            led->flags |= PWM_LED_FLAGS_TEENSY;
            // Must set teensy flag before calling this:
            pwm_rgb_led_set_percent( led, PWM_RED, 10 );
            led_set_teensy_led( led );
        } else {
            pwm_rgb_led_set_percent( led, PWM_RED, 10 );
#ifdef LED_CONTROLLER_ENABLE
            pwm_set_rgb_led( led );
#endif
        }
    }

#ifdef LED_CONTROLLER_ENABLE
    // Program the LED controller:
    if ( pwm_initialized ) {
        pwm_commit( true );
        led_update_pending = false;
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

void led_set_layer_indicator( uint32_t state ) {

    pwm_rgb_led_t * led = &leds[ LED_DISPLAY ];

    if ( state == 1 ) {
        led->flags &= ~PWM_LED_FLAGS_ENABLED;
    } else {

        led->flags |= PWM_LED_FLAGS_ENABLED | PWM_LED_FLAGS_ON;
        for ( int v = 0; v < 6; v++ ) {
            led->values[ v ] = 0;
        }

        if ( state & 2 ) {
            pwm_rgb_led_set_percent( led, PWM_RED, 10 );
        }
        if ( state & 4 ) {
            pwm_rgb_led_set_percent( led, PWM_BLUE, 10 );
        }

    }

    pwm_set_rgb_led( led );
    led_update_pending = true;
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
        for ( int ch = 0; ch < 3; ch++ ) {
            led_set_teensy_channel(
                led->channels[ ch ],
                led->values[ ch * 2 ]
            );
        }

    } else {

        // Should disable PWM instead?
        for ( int ch = 0; ch < 3; ch++ ) {
            led_set_teensy_channel( led->channels[ ch ], 0 );
        }
    }
}


/***************************************************************************/

// Turn the TrackPoint lights on or off.
void led_set_trackpoint( bool on ) {

    // Without PWM, the TrackPoint LED can be swiched on/off as follows:
    //
    // On (output high):
    //DDRB |= (1<<7);
    //PORTB |= (1<<7);
    //
    // Off (hi-Z):
    //DDRB &= ~(1<<7);
    //PORTB &= ~(1<<7);

    if ( on ) {
        led_set_teensy_channel( LED_TEENSY_CH_B7, led_trackpoint_value );
    } else {
        led_set_teensy_channel( LED_TEENSY_CH_B7, 0 );
    }

    led_trackpoint_on = on;
}


/***************************************************************************/

void led_teensy_pwm_init() {

    // Enable output drivers on PWM pins:
    DDRB |= 0b11110000;
    DDRC |= 0b01110000;

    // Can't use OC0A because timer 0 is being used by common/timer.c in CTC
    // (non-PWM) mode.  Pin B7 can be driven by either OC0A or OC1C, so we use
    // OC1C.

    // This seems to be required to enable PWM on B7.  Has something to do
    // with the OC0A/OC1C output modulator that is supposed to be disabled
    // when OC0A is not in use.
    PORTB &= ~0b10000000;

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
        (1<<COM1C1) |
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
    //   Set OC2A on compare match, clear at top.
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

    //led_fade();

    if ( led_update_pending ) {
        led_update_pending = false;
        pwm_commit( false );
    }
}


/***************************************************************************/

bool led_fade_direction = true;
int led_fade_color = 0;
bool led_fade_percent = true;
uint32_t led_fade_time = 0;

int led_fade_interval = 0;
int led_fade_step = 0;
int16_t led_fade_value = 0;
int led_fade_max = 0;

void led_fade() {

    pwm_rgb_led_t * led = &leds[ LED_SCROLL_LOCK_0 ];

    if ( led_fade_time == 0 ) {
        led_fade_time = timer_read32();

        led->flags |= PWM_LED_FLAGS_ON;
        led->flags |= PWM_LED_FLAGS_ENABLED;

        if ( led_fade_percent ) {
            led_fade_interval = 100;
            led_fade_step = 1;
            led_fade_value = 0;
            led_fade_max = 100;
        } else {
            led_fade_interval = 100;
            led_fade_step = 41;
            led_fade_value = 0;
            led_fade_max = 4095;
        }
    }

    else {

        if ( timer_elapsed32( led_fade_time ) >= led_fade_interval ) {

            if ( led_fade_direction ) {
                if ( led_fade_value == led_fade_max ) {
                    led_fade_direction = false;
                    led_fade_value = led_fade_max - led_fade_step;
                }
                else {
                    led_fade_value += led_fade_step;
                    if ( led_fade_value > led_fade_max ) {
                        led_fade_value = led_fade_max;
                    }
                }

            } else {
                if ( led_fade_value == 0 ) {
                    led_fade_direction = true;
                    led_fade_value = led_fade_step;
                    led_fade_color += 2;
                    if ( led_fade_color >= 6 ) led_fade_color = 0;
                }
                else {
                    led_fade_value -= led_fade_step;
                    if ( led_fade_value < 0 ) {
                        led_fade_value = 0;
                    }
                }
            }

            if ( led_fade_percent ) {
                pwm_rgb_led_set_percent( led, led_fade_color, led_fade_value );
            } else {
                led->values[ led_fade_color + 0 ] = 0;
                led->values[ led_fade_color + 1 ] = led_fade_value;
            }

            pwm_set_rgb_led( led );
            led_update_pending = true;
            led_fade_time = timer_read32();
        }
    }

}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
