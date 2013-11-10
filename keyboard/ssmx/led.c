/*
Copyright 2012 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <avr/io.h>
#include "stdint.h"
#include "led.h"
#include "pwm-driver.h"
#include "trackpoint.h"

#ifdef LED_CONTROLLER_ENABLE
static pwm_rgb_led_t rgb_led[ 3 ];
#endif

void led_set(uint8_t usb_led)
{
    if (usb_led & (1<<USB_LED_CAPS_LOCK)) {
        // output high
        DDRB |= (1<<7);
        PORTB |= (1<<7);

//#ifdef PS2_ENABLE
//        trackpoint_init();
//#endif

#ifdef LED_CONTROLLER_ENABLE
        rgb_led[ 0 ].enabled = true;
        rgb_led[ 0 ].channel_r = 0;
        rgb_led[ 0 ].channel_g = 1;
        rgb_led[ 0 ].channel_b = 2;
        rgb_led[ 0 ].on_r = PWM_LED_FULL;
        rgb_led[ 0 ].on_b = 0;
        rgb_led[ 0 ].on_g = 0;
        rgb_led[ 0 ].off_r = 0;
        rgb_led[ 0 ].off_b = 0;
        rgb_led[ 0 ].off_g = 0;

        pwm_set_rgb_led( &rgb_led[ 0 ] );
        pwm_commit( false );
#endif

    } else {

#ifdef LED_CONTROLLER_ENABLE
        // Problem: This is called before the PWM controller has
        // initialized.  Refactor the code to allow queued changes
        // to be effective upon PWM init.

        // Also, led_set() can be called from an ISR and commit
        // will block if there is a TWI operation in progress.  So
        // just set a volitile flag here and do the commit in the
        // next matrix_scan().

        //pwm_toggle_rgb_led( &rgb_led[ 0 ] );
        //pwm_commit( false );

#endif

        // Hi-Z
        DDRB &= ~(1<<7);
        PORTB &= ~(1<<7);
    }
}

/* vi: set et sts=4 sw=4 ts=4: */
