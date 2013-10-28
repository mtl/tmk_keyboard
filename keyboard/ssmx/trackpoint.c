/****************************************************************************
 *
 *  TrackPoint support
 *
 ***************************************************************************/

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "ps2.h"
#include "ps2_mouse.h"
#include "trackpoint.h"
#include "usb_mouse.h"
#include "util.h"


/***************************************************************************/

#ifdef PS2_MOUSE_DEBUG
#   include "print.h"
#   include "debug.h"
#else
#   define print(s)
#   define printf(...)
#   define phex(h)
#   define phex16(h)
#endif

#define RET_ON_ERROR() if ( status != TP_OK ) return status;

void inhibit(void);

/***************************************************************************/

// Globals:
static uint8_t buttons_status = 0;
static bool initialized = false;
static const char * ps2_prefix = "[PS2] ";
static int status = 0;
uint8_t tp_last_response_byte = 0;
static const char * tp_prefix = "[TP] ";
uint8_t tp_response[TP_RESPONSE_BUFFER_SIZE];


/***************************************************************************/

void inhibit() {
    PS2_CLOCK_PORT &= ~(1<<PS2_CLOCK_BIT);
    PS2_CLOCK_DDR  |=  (1<<PS2_CLOCK_BIT);
}


/***************************************************************************/

TP_STATUS _tp_command( int num_bytes, ... ) {

	// Ensure the TrackPoint is enabled:
    if ( ! tp_ready() ) return TP_DISABLED;

	va_list ap;
	va_start( ap, num_bytes );

	for ( int i = 0; i < num_bytes; i++ ) {

		status = tp_send_command_byte( va_arg( ap, int ) );
        switch ( status ) {
            case TP_OK: break;
            case TP_BAD_RESPONSE:
                if ( tp_last_response_byte == 0xfe ) {
                    printf( "%sCommand not supported.\n", tp_prefix );
                }
            default:
                printf(
                    "%sFailed to send command byte %i of %i.\n",
                    tp_prefix, i + 1, num_bytes
                );

                va_end( ap );
                return status;
        }
	}

	va_end( ap );
    inhibit();
	return TP_OK;
}


/***************************************************************************/

/*
typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t v;
    int8_t h;
} __attribute__ ((packed)) report_mouse_t;
*/

// Read the TP status.  If status hasn't changed, report.buttons will be 0xff.
TP_STATUS tp_get_report( report_mouse_t * report ) {

	// Ensure the TrackPoint is enabled:
    if ( ! tp_ready() ) return TP_DISABLED;

    uint8_t prev_buttons_status = buttons_status;

    status = tp_read_data();
    RET_ON_ERROR();

    // Return if no change:
    if (
        buttons_status == prev_buttons_status &&
        ! ( tp_response[ 0 ] & 0xf0 ) ||
        ! tp_response[ 1 ] ||
        ! tp_response[ 2 ]
    ) {
        report->buttons = 0xff;
        return TP_OK;
    }








	return TP_OK;
}


/***************************************************************************/

// TP Command: Read data
TP_STATUS tp_read_data() {

	// Ensure the TrackPoint is enabled:
    if ( ! tp_ready() ) return TP_DISABLED;

    status = tp_command( TP_CMD_READ_DATA );
    RET_ON_ERROR();
	status = tp_recv_response( 3 );
	RET_ON_ERROR();

    buttons_status = tp_response[ 0 ] & 7;
	return TP_OK;
}


/***************************************************************************/

// TP Command: Read controller RAM
TP_STATUS tp_ram_read( uint8_t location ) {

	// Ensure the TrackPoint is enabled:
    if ( ! tp_ready() ) return TP_DISABLED;

    if ( location <= 0x3f ) {
        status = tp_command( 0xe2, location );
    } else {
        status = tp_command( 0xe2, 0x80, location );
    }
    RET_ON_ERROR();

	// Receive the RAM contents:
	status = tp_recv_response( 1 );
	RET_ON_ERROR();

	return TP_OK;
}


/***************************************************************************/

// TP Command: Write controller RAM
TP_STATUS tp_ram_write( uint8_t location, uint8_t value ) {

	// Ensure the TrackPoint is enabled:
    if ( ! tp_ready() ) return TP_DISABLED;

    status = tp_command( 0xe2, 0x81, location, value );
    RET_ON_ERROR();

	return TP_OK;
}


/***************************************************************************/

// TP Command: XOR controller RAM
TP_STATUS tp_ram_xor( uint8_t location, uint8_t bitmask ) {

	// Ensure the TrackPoint is enabled:
    if ( ! tp_ready() ) return TP_DISABLED;

    status = tp_command( 0xe2, 0x47, location, bitmask );
    RET_ON_ERROR();

	return TP_OK;
}


/***************************************************************************/

// Check to see if the TrackPoint can be used.
bool tp_ready() {
/*
    if ( ps2_mouse_enable ) {
        print( "ps2_mouse_enable = true\n" );
    } else {
        print( "ps2_mouse_enable = false\n" );
    }
    if ( initialized ) {
        print( "initialized = true\n" );
    } else {
        print( "initialized = false\n" );
    }
*/
    return ps2_mouse_enable && initialized;
}


/***************************************************************************/

TP_STATUS tp_recv() {

    uint8_t r = ps2_host_recv_response();

	if ( ps2_error ) {
		printf( "%sRecv error: x%02X.\n", ps2_prefix, ps2_error );
		return TP_PS2_ERROR;
	}

    inhibit();
    print( "Received: " );
    phex( r );
    print( ".\n" );
    _delay_ms(100);

	tp_last_response_byte = r;
	return TP_OK;
}


/***************************************************************************/

TP_STATUS tp_recv_response( int num_bytes ) {

    tp_zero_response();

	for ( int i = 0; i < num_bytes; i++ ) {

		status = tp_recv();

		if ( status != TP_OK ) {
			printf(
				"%sRecv error: Failed to read byte %i of %i.\n",
				tp_prefix, i, num_bytes
			);
			return status;
		}

		tp_response[ i ] = tp_last_response_byte;
	}

	return TP_OK;
}


/***************************************************************************/

// TP Command: Reset.
TP_STATUS tp_reset() {

	// Ensure the TrackPoint is enabled:
    if ( ! ps2_mouse_enable ) return TP_DISABLED;

    // Send reset command and get the response:
	status = tp_command( TP_CMD_RESET );
	RET_ON_ERROR();
//    _delay_ms(100); // BAT takes some time
	status = tp_recv_response( 2 );
	RET_ON_ERROR();

	// Post completion code should be 0xAA on success or 0xFC on error.
	switch ( tp_response[ 0 ] ) {
		case 0xaa:
            print( "Reset 0xAA\n" );
            _delay_ms(100);
            break;
		case 0xfc:

            print( "Reset 0xFC\n" );
            _delay_ms(100);

            // Read POST results:
            status = tp_ram_read( TP_RAM_POST_REG );
            if ( status == TP_OK ) {
                printf( "%sPOST results: x%02X.\n", tp_prefix, tp_response[ 0 ] );
            }

			return TP_POST_FAIL;
		default:
			printf(
				"%sUnknown POST completion code: x%02X.\n",
				tp_prefix, tp_response[ 0 ]
			);
			return TP_POST_FAIL;
	}

	return TP_OK;
}


/***************************************************************************/

TP_STATUS tp_send( uint8_t message ) {

    print( "Sending " );
    phex( message );
    print( "...\n" );
    _delay_ms(100);

	uint8_t r = ps2_host_send( message );

	if ( ps2_error ) {
		printf( "%sSend error: x%02X.\n", ps2_prefix, ps2_error );
		return TP_PS2_ERROR;
	}

    inhibit();
    print( "ACK: " );
    phex( r );
    print( ".\n" );
    _delay_ms(100);

	tp_last_response_byte = r;
	return TP_OK;
}


/***************************************************************************/

// Send a single command byte, handle resend, verify ack, etc.
TP_STATUS tp_send_command_byte( uint8_t message ) {

	status = tp_send( message );
	RET_ON_ERROR();

    // Resend the byte if the TP returned the resend command:
	if ( tp_last_response_byte == TP_CMD_RESEND ) {
        status = tp_send( message );
        RET_ON_ERROR();

        // Handle the error code:
        if ( tp_last_response_byte == TP_CMD_ERROR ) {

            printf(
                "%sFailed to send command byte x%02X.\n",
                tp_prefix, message
            );
            return TP_BAD_RESPONSE;
        }

        // This shouldn't occur, but verify ACK just to be sure:
        else if ( tp_last_response_byte != TP_CMD_ACK ) {
            printf(
                "%sFailed to send command byte x%02X (controller returned x%02X).\n",
                tp_prefix, message, tp_last_response_byte
            );
            return TP_BAD_RESPONSE;
        }
    }

	return TP_OK;
}


/***************************************************************************/

void inline tp_zero_response() {
	for ( int i = 0; i < TP_RESPONSE_BUFFER_SIZE; i++ ) {
		tp_response[ i ] = 0;
	}
}


/***************************************************************************/

// Initialize the TrackPoint.
TP_STATUS trackpoint_init() {

	// Ensure the TrackPoint is enabled:
    if ( ! ps2_mouse_enable ) return TP_DISABLED;

	// Initialize the PS/2 host driver:
    ps2_host_init();

	// Clear response buffer:
	tp_zero_response();

	// Reset the TrackPoint:
	tp_reset();
    initialized = true;
    debug_config.mouse = true;

    // Enable the TrackPoint (starts data reporting):
	status = tp_command( TP_CMD_ENABLE );
	RET_ON_ERROR();

    // Set remote mode:
	status = tp_command( TP_CMD_SET_REMOTE_MODE );
	RET_ON_ERROR();

	return TP_OK;
}


/***************************************************************************/

// Poll the TrackPoint for updates.
TP_STATUS trackpoint_poll() {

	// Ensure the TrackPoint is enabled:
    if ( ! tp_ready() ) return TP_DISABLED;

//    print( "Poll..\n" );
//    _delay_ms( 100 );
//	status = tp_command( TP_CMD_READ_DATA );
//	RET_ON_ERROR();
//	status = tp_recv_response( 3 );
//	RET_ON_ERROR();
//    print( "Response: " );
//    phex( tp_response[ 0 ] );
//    print( ", " );
//    phex( tp_response[ 1 ] );
//    print( ", " );
//    phex( tp_response[ 2 ] );
//    print( ".\n" );
//    _delay_ms( 100 );

    // Poll the TrackPoint for updates:
    uint8_t result = ps2_mouse_read();
    if ( result ) {
        return TP_PS2_ERROR;
    }

    /*print( "Sending report..\n" );*/
    /*_delay_ms( 100 );*/

    // Send any updates to the USB host:
    ps2_mouse_usb_send();

    /*print( "Report sent (maybe).\n" );*/
    /*_delay_ms( 100 );*/

	return TP_OK;
}


/***************************************************************************/


/* vi: set et sts=4 sw=4 ts=4: */
