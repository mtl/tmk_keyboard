/*
Copyright 2011 Jun Wako <wakojun@gmail.com>

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

#include <stdbool.h>
#include<avr/io.h>
#include<util/delay.h>
#include "ps2.h"
#include "ps2_mouse.h"
#include "usb_mouse.h"

#define PS2_MOUSE_DEBUG
#ifdef PS2_MOUSE_DEBUG
#   include "print.h"
#   include "debug.h"
#else
#   define print(s)
#   define phex(h)
#   define phex16(h)
#endif

// disable when errors occur 255 times.
#define ERROR_RETURN() do { \
    if (ps2_error) { \
        if (ps2_mouse_error_count < 255) { \
            ps2_mouse_error_count++; \
        } else { \
            ps2_mouse_error_count = 0; \
            ps2_mouse_enable = false; \
        } \
		print( "PS/2 Error. :(\n" ); \
        return ps2_error; \
    } \
} while (0)


/*
TODO
----
- Stream mode
- Tracpoint command support: needed
- Middle button + move = Wheel traslation
*/
bool ps2_mouse_enable = true;
uint8_t ps2_mouse_x = 0;
uint8_t ps2_mouse_y = 0;
uint8_t ps2_mouse_btn = 0;
uint8_t ps2_mouse_error_count = 0;

static uint8_t ps2_mouse_btn_prev = 0;


uint8_t ps2_mouse_init(void) {
    uint8_t rcv;

	bool post_error = false;

    if (!ps2_mouse_enable) return 1;

    ps2_host_init();

    // Reset
    rcv = ps2_host_send(0xFF);
    print("ps2_mouse_init: send 0xFF: ");
    phex(rcv); phex(ps2_error); print("\n");
    ERROR_RETURN();

    // ACK
    rcv = ps2_host_recv();
    print("ps2_mouse_init: read ACK: ");
    phex(rcv); phex(ps2_error); print("\n");
    ERROR_RETURN();
	if ( rcv == 0xfc ) {
		post_error = true;
	}

    // BAT takes some time
    _delay_ms(100);
    rcv = ps2_host_recv();
    print("ps2_mouse_init: read BAT: ");
    phex(rcv); phex(ps2_error); print("\n");
    ERROR_RETURN();

	if ( post_error ) {
		rcv = ps2_host_send(0xe2);
		print("ps2_mouse_init: send 0xE2: ");
		phex(rcv); phex(ps2_error); print("\n");
		ERROR_RETURN();

		rcv = ps2_host_send(0x25);
		print("ps2_mouse_init: send 0x25: ");
		phex(rcv); phex(ps2_error); print("\n");
		ERROR_RETURN();

		rcv = ps2_host_recv();
		print("ps2_mouse_init: read POST error: ");
		phex(rcv); phex(ps2_error); print("\n");
		ERROR_RETURN();
	}

    // Device ID
    rcv = ps2_host_recv();
    print("ps2_mouse_init: read DevID: ");
    phex(rcv); phex(ps2_error); print("\n");
    ERROR_RETURN();

    // Enable data reporting
    ps2_host_send(0xF4);
    print("ps2_mouse_init: send 0xF4: ");
    phex(ps2_error); print("\n");
    ERROR_RETURN();

    // ACK
    rcv = ps2_host_recv();
    print("ps2_mouse_init: read ACK: ");
    phex(rcv); phex(ps2_error); print("\n");
    ERROR_RETURN();

    // Set Remote mode
    ps2_host_send(0xF0);
    print("ps2_mouse_init: send 0xF0: ");
    phex(ps2_error); print("\n");
    ERROR_RETURN();

    // ACK
    rcv = ps2_host_recv();
    print("ps2_mouse_init: read ACK: ");
    phex(rcv); phex(ps2_error); print("\n");
    ERROR_RETURN();

    return 0;
}

/*
Data format:
    bit: 7       6       5       4       3       2       1       0
-----------------------------------------------------------------------
0   btn: Yovflw  Xovflw  Ysign   Xsign   1       Middle  Right   Left
1   x:   X movement(0-255)
2   y:   Y movement(0-255)
*/
uint8_t ps2_mouse_read(void)
{
    uint8_t rcv;

    if (!ps2_mouse_enable) return 1;

    rcv=ps2_host_send(0xEB);
    ERROR_RETURN();

    if(rcv==0xFA) {
        ps2_mouse_btn = ps2_host_recv_response();
        ERROR_RETURN();
        ps2_mouse_x = ps2_host_recv_response();
        ERROR_RETURN();
        ps2_mouse_y = ps2_host_recv_response();
        ERROR_RETURN();
		//print( "Mouse read ok.\n" );
		//_delay_ms( 100 );
    }
	else {
		print( "Mouse read failed: " );
		_delay_ms( 100 );
		phex( rcv );
		print( ".\n" );
	}
    return 0;
}

bool ps2_mouse_changed(void)
{
    return (ps2_mouse_x || ps2_mouse_y || (ps2_mouse_btn & PS2_MOUSE_BTN_MASK) != ps2_mouse_btn_prev);
}

#define PS2_MOUSE_SCROLL_BUTTON 0x04
void ps2_mouse_usb_send(void)
{
    static bool scrolled = false;

    if (!ps2_mouse_enable) return;

    if (ps2_mouse_changed()) {
		print( "Mouse changed!!.\n" );
		_delay_ms( 100 );
        int8_t x, y, v, h;
        x = y = v = h = 0;

        // convert scale of X, Y: PS/2(-256/255) -> USB(-127/127)
        if (ps2_mouse_btn & (1<<PS2_MOUSE_X_SIGN))
            x = ps2_mouse_x > 128 ? (int8_t)ps2_mouse_x : -127;
        else
            x = ps2_mouse_x < 128 ? (int8_t)ps2_mouse_x : 127;

        if (ps2_mouse_btn & (1<<PS2_MOUSE_Y_SIGN))
            y = ps2_mouse_y > 128 ? (int8_t)ps2_mouse_y : -127;
        else
            y = ps2_mouse_y < 128 ? (int8_t)ps2_mouse_y : 127;

        // Y is needed to reverse
        y = -y;

        if (ps2_mouse_btn & PS2_MOUSE_SCROLL_BUTTON) {
            // scroll
            if (x > 0 || x < 0) h = (x > 64 ? 64 : (x < -64 ? -64 :x));
            if (y > 0 || y < 0) v = (y > 64 ? 64 : (y < -64 ? -64 :y));
            if (h || v) {
                scrolled = true;
				print( "usb_mouse_send 1.\n" );
				_delay_ms( 100 );
                usb_mouse_send(0,0, -v/16, h/16, 0);
                _delay_ms(100);
            }
        } else if (!scrolled && (ps2_mouse_btn_prev & PS2_MOUSE_SCROLL_BUTTON)) {
			print( "usb_mouse_send 2.\n" );
			_delay_ms( 100 );
            usb_mouse_send(0,0,0,0, PS2_MOUSE_SCROLL_BUTTON);
            _delay_ms(100);
			print( "usb_mouse_send 3.\n" );
            usb_mouse_send(0,0,0,0, 0);
        } else { 
            scrolled = false;
			print( "usb_mouse_send 4.\n" );
			_delay_ms( 100 );
            usb_mouse_send(x, y, 0, 0, ps2_mouse_btn & PS2_MOUSE_BTN_MASK);
        }

        ps2_mouse_btn_prev = (ps2_mouse_btn & PS2_MOUSE_BTN_MASK);
        ps2_mouse_print();
    }
    ps2_mouse_x = 0;
    ps2_mouse_y = 0;
    ps2_mouse_btn = 0;
}

void ps2_mouse_print(void)
{
    if (!debug_mouse) return;
    print("ps2_mouse[btn|x y]: ");
    phex(ps2_mouse_btn); print("|");
    phex(ps2_mouse_x); print(" ");
    phex(ps2_mouse_y); print("\n");
}
