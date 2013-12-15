/****************************************************************************
 *
 *  SSD1351 support
 *  128 x 128 262-color (?) OLED
 *
 ***************************************************************************/

#ifndef MX13SS_DISPLAY_H
#define MX13SS_DISPLAY_H

#include <stdbool.h>


/****************************************************************************
 * Typedefs
 ***************************************************************************/


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
void display_draw( void );
void display_draw_logo( uint8_t );


/***************************************************************************/

#endif

/* vi: set et sts=4 sw=4 ts=4: */
