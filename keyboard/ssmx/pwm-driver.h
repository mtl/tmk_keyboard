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

//typedef int TP_STATUS;

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
	bool enabled;
} pwm_rgb_led_t;


/****************************************************************************
 * Constants and macros
 ***************************************************************************/

// Max message: 1-byte starting register address, plus 16 channels, 2
// registers per channel (lo/hi), and 2 bytes per register:
#define PWM_MAX_MESSAGE ( 1 + 16 * 2 * 2 )

#define PWM_LED_FULL 0x1000

#define PWM_PCA9685_SUBADR1 0x2
#define PWM_PCA9685_SUBADR2 0x3
#define PWM_PCA9685_SUBADR3 0x4

#define PWM_PCA9685_MODE1 0x0
#define PWM_PCA9685_PRESCALE 0xFE

#define PWM_LED0_ON_L 0x6
#define PWM_LED0_ON_H 0x7
#define PWM_LED0_OFF_L 0x8
#define PWM_LED0_OFF_H 0x9

#define PWM_ALLLED_ON_L 0xFA
#define PWM_ALLLED_ON_H 0xFB
#define PWM_ALLLED_OFF_L 0xFC
#define PWM_ALLLED_OFF_H 0xFD


/****************************************************************************
 * Externs
 ***************************************************************************/


/****************************************************************************
 * Prototypes
 ***************************************************************************/

bool pwm_commit( bool );
void pwm_disable_rgb_led( pwm_rgb_led_t * );
void pwm_enable_rgb_led( pwm_rgb_led_t * );
bool pwm_init( float );
bool pwm_read_register( uint8_t, uint8_t * );
bool pwm_reset( void );
void pwm_set_channel( uint8_t, uint16_t, uint16_t );
bool pwm_set_freq( float );
void pwm_set_rgb_led( pwm_rgb_led_t * );
void pwm_toggle_rgb_led( pwm_rgb_led_t * );
bool pwm_write_register( uint8_t, uint8_t, bool );


/***************************************************************************/

#endif
