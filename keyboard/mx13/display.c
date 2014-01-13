/****************************************************************************
 *
 *  SSD1351 support
 *  128 x 128 262-color (?) OLED
 *
 ***************************************************************************/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h> 
#include "display.h"
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

void display_clear() {
    u8g_SetRGB( &u8g, 0, 0, 0 );
    u8g_DrawBox( &u8g, 0, 0, 128, 128 );
}


/***************************************************************************/

void display_draw() {

    u8g_FirstPage( &u8g );
    do {
        ui_draw();
    } while ( u8g_NextPage( &u8g ) );
    u8g_Delay( 100 );
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

void display_draw_menu() {

    int display_menu_title_font_vsize = 0;
    int display_menu_title_font_height = 0;
    int display_menu_title_gap = 4;
    int display_menu_title_pad = 2;
    u8g_pgm_uint8_t * display_menu_title_font = u8g_font_profont12; 

    int display_menu_list_font_vsize = 0;
    int display_menu_list_font_height = 0;
    int display_menu_list_hpad = 2;
    int display_menu_list_vpad = 1;
    u8g_pgm_uint8_t * display_menu_list_font = u8g_font_profont12; 

    // When we query font dimensions, base them on the largest extent of all
    // the glyphs in the font:
    u8g_SetFontRefHeightAll( &u8g );

    // Calculate title bar font dimensions:
    u8g_SetFont( &u8g, display_menu_title_font );
    display_menu_title_font_height = u8g_GetFontAscent( &u8g );
    display_menu_title_font_vsize = display_menu_title_font_height - u8g_GetFontDescent( &u8g );

    // Draw title bar:
    u8g_SetRGB( &u8g, 100, 100, 200 ); // Title bar color
    int title_bar_height = display_menu_title_font_vsize + ( display_menu_title_pad << 1 );
    u8g_DrawBox( &u8g, 0, 0, 128, title_bar_height );
    u8g_SetRGB( &u8g, 255, 255, 255 ); // Title font color
    u8g_DrawStr(
        &u8g,
        display_menu_title_pad + 1,
        display_menu_title_font_height + display_menu_title_pad,
        "MX13 Config" // Title.  If in PROGMEM use DrawStrP.
    );

    // Draw list background:
    u8g_SetRGB( &u8g, 0, 0, 128 ); // Background color
    int y = title_bar_height + display_menu_title_gap;
    u8g_DrawBox( &u8g, 0, y, 128, 128 - y );

    // Calculate list font dimensions:
    u8g_SetFont( &u8g, display_menu_list_font );
    display_menu_list_font_height = u8g_GetFontAscent( &u8g );
    display_menu_list_font_vsize = display_menu_list_font_height - u8g_GetFontDescent( &u8g );

    // Draw list:
    u8g_SetRGB( &u8g, 255, 255, 255 ); // Unselected list item color
    y += display_menu_list_vpad + display_menu_list_font_height + 1;
    int step = display_menu_list_font_vsize + ( display_menu_list_vpad << 1 ) + 1;
    u8g_DrawStr( &u8g, display_menu_list_hpad + 1, y,"Keyboard" );
    y += step;
    u8g_DrawStr( &u8g, display_menu_list_hpad, y,"TrackPoint" );
    y += step;
    u8g_DrawStr( &u8g, display_menu_list_hpad, y,"Lights" );
    y += step;
    u8g_DrawStr( &u8g, display_menu_list_hpad, y,"Firmware" );
    y += step;
    u8g_DrawStr( &u8g, display_menu_list_hpad, y,"Save..." );
}


/***************************************************************************/

void display_draw_u8g_logo() {

    u8g_SetRGB(&u8g, 0, 0, 255 );
    display_draw_u8g_name(2);
    u8g_SetRGB(&u8g, 0, 255, 0 );
    display_draw_u8g_name(1);
    u8g_SetRGB(&u8g, 255, 0, 0 );
    display_draw_u8g_name(0);

    u8g_SetRGB(&u8g, 0, 255, 255 );
    display_draw_u8g_url();
}


/***************************************************************************/

void display_draw_u8g_name( uint8_t d ) {

	uint8_t ybase = 10;
	//uint8_t ybase = 100;

    u8g_SetFont(&u8g, u8g_font_gdr25r);
    u8g_DrawStr(&u8g, 0+d, ybase + 20+d, "U");
    u8g_SetFont(&u8g, u8g_font_gdr30n);
    u8g_DrawStr90(&u8g, 23+d,ybase+d,"8");
    u8g_SetFont(&u8g, u8g_font_gdr25r);
    u8g_DrawStr(&u8g, 53+d,ybase+20+d,"g");

    u8g_DrawHLine(&u8g, 2+d, ybase+25+d, 47);
    u8g_DrawVLine(&u8g, 45+d, ybase + 22+d, 12);
}


/***************************************************************************/

void display_draw_u8g_url() {

	//uint8_t ybase = 10;

    u8g_SetFont( &u8g, u8g_font_4x6 );
    u8g_DrawStr( &u8g, 1, 54,"code.google.com/p/u8glib" );
}

/***************************************************************************/

void display_init() {

//    u8g_InitSPI(
//        &u8g,
//        &u8g_dev_ssd1351_128x128_262k_sw_spi,
//        // sck, mosi, cs, a0 (C/D), reset
//        PN(1, 1), PN(1, 2), PN(1, 0), PN(0, 6), PN(0, 7)
//    );

    u8g_InitHWSPI(
        &u8g,
        &u8g_dev_ssd1351_128x128_262k_hw_spi,
        // cs, a0 (C/D), reset
        PN(1, 0), PN(0, 6), PN(0, 7)
    );

    display_draw();
}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
