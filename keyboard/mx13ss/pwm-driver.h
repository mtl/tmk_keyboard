/****************************************************************************
 *
 *  Adafruit PWM servo driver support
 *  http://www.adafruit.com/products/815
 *
 ***************************************************************************/

#ifndef PWM_DRIVER_H
#define PWM_DRIVER_H

#include <stdbool.h>


/****************************************************************************
 * Typedefs
 ***************************************************************************/

typedef struct pwm_rgb_led {
    uint16_t on_r;
    uint16_t on_g;
    uint16_t on_b;
    uint16_t off_r;
    uint16_t off_g;
    uint16_t off_b;
    uint8_t channel_r;
    uint8_t channel_g;
    uint8_t channel_b;
    uint8_t flags;
} pwm_rgb_led_t;


/****************************************************************************
 * Constants and macros
 ***************************************************************************/

// pwm_rgb_led_t flags:
// LED currently lit:
#define PWM_LED_FLAGS_ON       0x01
// LED master enable:
#define PWM_LED_FLAGS_ENABLED  0x02
// LED is driven directly by the Teensy, not by the PWM controller:
#define PWM_LED_FLAGS_TEENSY   0x04

// Max message: 1-byte starting register address, plus 16 channels, 2
// registers per channel (lo/hi), and 2 bytes per register:
#define PWM_MAX_MESSAGE ( 1 + 16 * 2 * 2 )

#define PWM_LED_FULL 0x1000

#define PWM_MODE1            0x00
#define PWM_MODE2            0x01
#define PWM_SUBADR1          0x02
#define PWM_SUBADR2          0x03
#define PWM_SUBADR3          0x04
#define PWM_LED0_ON_L        0x06
#define PWM_LED0_ON_H        0x07
#define PWM_LED0_OFF_L       0x08
#define PWM_LED0_OFF_H       0x09
#define PWM_ALLLED_ON_L      0xFA
#define PWM_ALLLED_ON_H      0xFB
#define PWM_ALLLED_OFF_L     0xFC
#define PWM_ALLLED_OFF_H     0xFD
#define PWM_PRE_SCALE        0xFE

// MODE1 bits:
#define PWM_RESTART          7
#define PWM_EXTCLK           6 
#define PWM_AI               5
#define PWM_SLEEP            4
#define PWM_SUB1             3
#define PWM_SUB2             2
#define PWM_SUB3             1
#define PWM_ALLCALL          0

// MODE2 bits:
#define PWM_INVRT            4
#define PWM_OCH              3
#define PWM_OUTDRV           2
#define PWM_OUTNE            0

// 25 MHz internal oscillator?
#define PWM_OSC_CLOCK 25000000

#define PWM_MIN_PRESCALE     3


/****************************************************************************
 * Externs
 ***************************************************************************/


/****************************************************************************
 * Prototypes
 ***************************************************************************/

bool pwm_commit( bool );
uint8_t pwm_get_prescale_value( float );
bool pwm_init( void );
bool pwm_read_register( uint8_t, uint8_t * );
bool pwm_reset( void );
void pwm_rgb_led_off( pwm_rgb_led_t * );
void pwm_rgb_led_on( pwm_rgb_led_t * );
void pwm_set_channel( uint8_t, uint16_t, uint16_t );
bool pwm_set_prescaler( uint8_t );
void pwm_set_rgb_led( pwm_rgb_led_t * );
void pwm_toggle_rgb_led( pwm_rgb_led_t * );
bool pwm_write_register( uint8_t, uint8_t, bool );


/***************************************************************************/

#endif

/* vi: set et sts=4 sw=4 ts=4: */
