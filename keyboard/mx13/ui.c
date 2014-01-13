/****************************************************************************
 *
 *  User Interface
 *  128 x 128 262-color (?) OLED
 *
 ***************************************************************************/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h> 
#include "dt_logo.h"
#include "display.h"
#include "ui.h"


/***************************************************************************/

// Globals:
static bool ui_active = false;


/***************************************************************************/


void ui_draw() {

	if ( ui_active ) {
		display_draw_menu();
	} else {
//		display_clear();
		display_draw_bitmap( 0, 0, 128, 124, dt_logo );
	}
}


/***************************************************************************/


void ui_enter() {
	ui_active = true;
	display_draw();
}


/***************************************************************************/

void ui_handle_key( key_t key, bool is_pressed ) {
}


/***************************************************************************/


void ui_leave() {
	ui_active = false;
	display_draw();
}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
