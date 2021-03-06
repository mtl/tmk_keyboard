/****************************************************************************
 *
 *  SSD1351 support
 *  128 x 128 18-bit color OLED
 *
 ***************************************************************************/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h> 
#include "display.h"
#include "led-local.h"
#include "pwm-driver.h"
#include "ui.h"


/***************************************************************************/

//  text    data    bss     dec    hex
//  15968     16      289   16273   3f91   (tmk    )
//  14304     64     3152   17520   4470   (    u8g)
//  54246   1982    30789   87017  153e9   (tmk+u8g)

// Teensy 2.0++:
//  Flash:  130048
//  RAM:      8192
//  EEPROM:   4096


/***************************************************************************/

// Globals:
u8g_t u8g;


/***************************************************************************/

void display_busy( bool is_busy ) {

#ifdef LED_CONTROLLER_ENABLE

    static bool was_busy = false;
    static uint16_t values[ 6 ];
    static uint8_t prior_flags = 0;
    pwm_rgb_led_t * led = &led_config.leds[ LED_DISPLAY ];

    // Save LED state:
    if ( is_busy && ! was_busy ) {

        prior_flags = led->flags;
        for ( int i = 0; i < 6; i++ ) {
            values[ i ] = led->values[ i ];
        }

        // Turn on LED:
        led->flags |= PWM_LED_FLAGS_ON;
        pwm_rgb_led_set_percent( led, PWM_RED, 3 );
        pwm_rgb_led_set_percent( led, PWM_GREEN, 0 );
        pwm_rgb_led_set_percent( led, PWM_BLUE, 0 );
        pwm_set_rgb_led( led );
        pwm_commit( true );

        was_busy = true;
    }

    // Restore LED state:
    else if ( ! is_busy && was_busy ) {

        // Restore LED state:
        led->flags = prior_flags;
        for ( int i = 0; i < 6; i++ ) {
            led->values[ i ] = values[ i ];
        }
        pwm_set_rgb_led( led );
        pwm_commit( true );

        was_busy = false;
    }

#endif
}


/***************************************************************************/

void display_clear() {
    u8g_SetRGB( &u8g, 0, 0, 0 );
    u8g_DrawBox( &u8g, 0, 0, 128, 128 );
}


/***************************************************************************/

void display_draw( bool sleep ) {

    // Turn on the busy LED:
    display_busy( true );

    // Blank the display if requested:
    if ( sleep ) {
        u8g_SleepOn( &u8g );
    }

    // Render the image:
    u8g_FirstPage( &u8g );
    do {
        // Stop if the draw function doesn't want to continue:
        if ( ! ui_draw( &u8g ) ) {
            break;
        }
    } while ( u8g_NextPage( &u8g ) );
    u8g_Delay( 100 );

    // Turn the display back on if blanking was requested:
    if ( sleep ) {
        u8g_SleepOff( &u8g );
    }

    // Turn off the busy LED:
    display_busy( false );
}


/***************************************************************************/

void display_draw_bitmap(
    u8g_uint_t x, u8g_uint_t y, u8g_uint_t width, u8g_uint_t height,
    const u8g_pgm_uint8_t *image
) {
    u8g_DrawColorBitmapP(
        &u8g,
        0, 0, // x, y
        128, // width
        124, // height
        image
    );
}


/***************************************************************************/

void display_draw_full_screen_bitmap( const u8g_pgm_uint8_t *image ) {

    u8g_SleepOn( &u8g );
    u8g_DrawFullScreenBitmapP( &u8g, (uint8_t *) image );
    u8g_SleepOff( &u8g );
}


/***************************************************************************/

void display_init() {

//    u8g_InitSPI(
//        &u8g,
//        &u8g_dev_ssd1351_128x128_18bpp_sw_spi,
//        // sck, mosi, cs, a0 (C/D), reset
//        PN(1, 1), PN(1, 2), PN(1, 0), PN(0, 6), PN(0, 7)
//    );

    u8g_InitHWSPI(
        &u8g,
        &u8g_dev_ssd1351_128x128_18bpp_hw_spi,
        // cs, a0 (C/D), reset
        PN(1, 0), PN(0, 6), PN(0, 7)
    );

    display_draw( true );
}


/***************************************************************************/

void display_set_draw_color( display_color_t * color ) {
    u8g_SetRGB( &u8g, color->r, color->g, color->b );
}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
