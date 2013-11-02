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
static pwm_rgb_led_t rgb_led;
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
        rgb_led.enabled = true;
        rgb_led.channel_r = 0;
        rgb_led.channel_g = 1;
        rgb_led.channel_b = 2;
        rgb_led.on_r = PWM_LED_FULL;
        rgb_led.on_b = PWM_LED_FULL;
        rgb_led.on_g = PWM_LED_FULL;
        rgb_led.off_r = 0;
        rgb_led.off_b = 0;
        rgb_led.off_g = 0;

        pwm_set_rgb_led( &rgb_led );
        pwm_commit( false );
#endif

    } else {

#ifdef LED_CONTROLLER_ENABLE
        //pwm_toggle_rgb_led( &rgb_led );
        //pwm_commit( false );
#endif

        // Hi-Z
        DDRB &= ~(1<<7);
        PORTB &= ~(1<<7);
    }
}

/* vi: set et sts=4 sw=4 ts=4: */
