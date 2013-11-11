/****************************************************************************
 *
 *  LED functions
 *
 ***************************************************************************/

#ifndef LED_LOCAL_H
#define LED_LOCAL_H

#include <stdbool.h>
#include "pwm-driver.h"


/****************************************************************************
 * Typedefs
 ***************************************************************************/


/****************************************************************************
 * Constants and macros
 ***************************************************************************/

// leds array indices:
// PWM controller LEDs:
#define LED_CAPS_LOCK_0     0
#define LED_CAPS_LOCK_1     1
#define LED_NUM_LOCK_0      2
#define LED_SCROLL_LOCK_0   3
#define LED_DISPLAY         4
// Teensy-based LEDs:
#define LED_NUM_LOCK_1      5
#define LED_SCROLL_LOCK_1   6
#define LED_ARRAY_SIZE      7
// Teensy-based, but not in the array because it's not RGB:
#define LED_TRACKPOINT      7

// Teensy PWM "channel numbers":
#define LED_TEENSY_CH_B4 (LED_NUM_LOCK_1 * 3 + 0)
#define LED_TEENSY_CH_B5 (LED_NUM_LOCK_1 * 3 + 1)
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
void led_set_one( pwm_rgb_led_t *, bool, bool );
void led_set_teensy_led( pwm_rgb_led_t * );
void led_set_teensy_channel( uint8_t, uint8_t );
void led_set_trackpoint( bool );
void led_teensy_pwm_init( void );
void led_update( void );


/***************************************************************************/

#endif

/* vi: set et sts=4 sw=4 ts=4: */
