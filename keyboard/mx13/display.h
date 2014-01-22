/****************************************************************************
 *
 *  SSD1351 support
 *  128 x 128 262-color (?) OLED
 *
 ***************************************************************************/

#ifndef MX13_DISPLAY_H
#define MX13_DISPLAY_H

#include <stdbool.h>
#include "u8g.h"


/****************************************************************************
 * Typedefs
 ***************************************************************************/

typedef union display_color {
    uint8_t channels[ 3 ];
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    struct {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };
} display_color_t;


/****************************************************************************
 * Constants and macros
 ***************************************************************************/


/****************************************************************************
 * Externs
 ***************************************************************************/


/****************************************************************************
 * Prototypes
 ***************************************************************************/

void display_init( void );
void display_clear( void );
void display_draw( bool );
void display_draw_bitmap( u8g_uint_t, u8g_uint_t, u8g_uint_t, u8g_uint_t, const u8g_pgm_uint8_t * );
void display_draw_full_screen_bitmap( const u8g_pgm_uint8_t * );
void display_draw_menu( void );
void display_draw_u8g_logo( void );
void display_draw_u8g_name( uint8_t );
void display_draw_u8g_url( void );
void display_set_draw_color( display_color_t * );


/***************************************************************************/

#endif

/* vi: set et sts=4 sw=4 ts=4: */
