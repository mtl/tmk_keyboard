/****************************************************************************
 *
 *  LED functions
 *
 ***************************************************************************/

#ifndef MX13_LED_LOCAL_H
#define MX13_LED_LOCAL_H

#include <stdbool.h>
#include "pwm-driver.h"


/****************************************************************************
 * Typedefs
 ***************************************************************************/


/****************************************************************************
 * Constants and macros
 ***************************************************************************/

typedef enum {

    // LEDs wired to the PWM controller:
#ifdef LED_CONTROLLER_ENABLE
    LED_CAPS_LOCK_1,
    LED_NUM_LOCK_0,
    LED_SCROLL_LOCK_0,
    LED_CAPS_LOCK_0,
    LED_DISPLAY,
#endif

    // LEDs wired directly to the Teensy:
    LED_NUM_LOCK_1,
    LED_SCROLL_LOCK_1,

    // Wired directly to the Teensy; not in the array because it's not RGB:
    LED_TRACKPOINT
} led_indices_t;

#define LED_ARRAY_SIZE LED_TRACKPOINT
#define LED_ARRAY_FIRST_TEENSY LED_NUM_LOCK_1
#define LED_TEENSY_FULL 0xff

// Teensy PWM "channel numbers":
#define LED_TEENSY_CH_B4 (LED_NUM_LOCK_1 * 3 + 1)
#define LED_TEENSY_CH_B5 (LED_NUM_LOCK_1 * 3 + 0)
#define LED_TEENSY_CH_B6 (LED_NUM_LOCK_1 * 3 + 2)
#define LED_TEENSY_CH_C4 (LED_SCROLL_LOCK_1 * 3 + 0)
#define LED_TEENSY_CH_C5 (LED_SCROLL_LOCK_1 * 3 + 1)
#define LED_TEENSY_CH_C6 (LED_SCROLL_LOCK_1 * 3 + 2)
#define LED_TEENSY_CH_B7 (LED_TRACKPOINT * 3 + 0)


/****************************************************************************
 * Externs
 ***************************************************************************/

extern uint8_t led_trackpoint_value;


/****************************************************************************
 * Prototypes
 ***************************************************************************/

void led_init( void );
//void led_set( uint8_t );
void led_set_layer_indicator( uint32_t );
void led_set_one( pwm_rgb_led_t *, bool, bool );
void led_set_teensy_led( pwm_rgb_led_t * );
void led_set_teensy_channel( uint8_t, uint8_t );
void led_set_trackpoint( bool );
void led_teensy_pwm_init( void );
void led_update( void );
void led_fade( void );


/***************************************************************************/

#endif

/* vi: set et sts=4 sw=4 ts=4: */
