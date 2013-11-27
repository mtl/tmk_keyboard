/****************************************************************************
 *
 *  SSD1351 support
 *  128 x 128 262-color (?) OLED
 *
 ***************************************************************************/

#include <avr/interrupt.h>
#include <avr/io.h>
#include "display.h"
#include "u8g.h"


/***************************************************************************/

// Globals:
u8g_t u8g;


/***************************************************************************/


//  text    data    bss     dec    hex
//  15968     16      289   16273   3f91   (tmk    )
//  14304     64     3152   17520   4470   (    u8g)
//  54246   1982    30789   87017  153e9   (tmk+u8g)

// Teensy 2.0++:
//  Flash:  130048
//  RAM:      8192
//  EEPROM:   4096


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

    //for(;;) {  
        u8g_FirstPage( &u8g );
        do {
            display_draw();
        } while ( u8g_NextPage( &u8g ) );
        u8g_Delay( 100 );
    //} 
}


/***************************************************************************/

void display_draw() {

    u8g_SetRGB(&u8g, 0, 0, 255 );
    display_draw_logo(2);
    u8g_SetRGB(&u8g, 0, 255, 0 );
    display_draw_logo(1);
    u8g_SetRGB(&u8g, 255, 0, 0 );
    display_draw_logo(0);

    u8g_SetRGB(&u8g, 0, 255, 255 );
    display_draw_url();
}


/***************************************************************************/

void display_draw_logo( uint8_t d ) {

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

void display_draw_url() {

	//uint8_t ybase = 10;

    u8g_SetFont( &u8g, u8g_font_4x6 );
    if ( u8g_GetHeight(&u8g) < 59 ) {
        u8g_DrawStr( &u8g, 53, 9,"code.google.com" );
        u8g_DrawStr( &u8g, 77, 18,"/p/u8glib" );
    }
    else {
        u8g_DrawStr( &u8g, 1, 54,"code.google.com/p/u8glib" );
    }
}

/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
