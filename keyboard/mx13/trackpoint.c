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
#include "display.h"
#include "ui.h"


/***************************************************************************/

//#ifdef PS2_MOUSE_DEBUG
//#   include "print.h"
//#   include "debug.h"
//#else
//#   define print(s)
//#   define printf(...)
//#   define phex(h)
//#   define phex16(h)
//#endif

#define RET_ON_ERROR() if ( status != TP_OK ) return status;

#define tp_log( msg ) \
    ui_log_append_str( (msg) ); \
    display_draw( false );


/***************************************************************************/

// Globals:
static uint8_t buttons_status = 0;
static bool initialized = false;
//static const char * ps2_prefix = "[PS2] ";
static int status = 0; // tp_status_t of last operation
uint8_t tp_last_response_byte = 0; // Most recent byte returned by the TP
//static const char * tp_prefix = "[TP] ";
uint8_t tp_response[ TP_RESPONSE_BUFFER_SIZE ]; // Response to most recent TP command


/***************************************************************************/

// Send a sequence of command bytes.
tp_status_t _tp_command( int num_bytes, ... ) {

    // Ensure the TrackPoint is enabled:
    if ( ! initialized ) return TP_DISABLED;

    va_list ap;
    va_start( ap, num_bytes );

    for ( int i = 0; i < num_bytes; i++ ) {

        status = tp_send_command_byte( va_arg( ap, int ) );
        switch ( status ) {
            case TP_OK: continue;
            case TP_BAD_RESPONSE:
//                if ( tp_last_response_byte == 0xfe ) {
//                    printf( "%sCommand not supported.\n", tp_prefix );
//                }

            // tp_send_command_byte() also returns TP_FAIL.
            //case TP_FAIL:

            default:
//                printf(
//                    "%sFailed to send command byte %i of %i.\n",
//                    tp_prefix, i + 1, num_bytes
//                );

                va_end( ap );
                return status;
        }
    }

    va_end( ap );
    return TP_OK;
}


/***************************************************************************/

// Initialize the TrackPoint.
tp_status_t tp_init() {

    // Clear response buffer:
    tp_zero_response();

    tp_log( "TP: Resetting\n" );

    // Reset the TrackPoint:
    tp_reset();
    initialized = true;
    //debug_config.mouse = true;

    tp_log( "TP: Enabling reports\n" );

    // Enable the TrackPoint (starts data reporting):
    status = tp_command( TP_CMD_ENABLE );
    RET_ON_ERROR();

    tp_log( "TP: Set remote\n" );

    // Set remote mode:
    status = tp_command( TP_CMD_SET_REMOTE_MODE );
    RET_ON_ERROR();

    //--------------------

    tp_log( "TP: Enable PtS\n" );

    status = tp_ram_bit_set( TP_RAM_CONFIG, TP_BIT_PTSON );
    RET_ON_ERROR();

    //--------------------

    tp_log( "TP: Sens. factor\n" );

    status = tp_ram_write( TP_RAM_SNSTVTY, 0xc0 );
    RET_ON_ERROR();

    //--------------------

//    tp_log( "TP: Invert Y\n" );
//
//    status = tp_ram_bit_clear( TP_RAM_CONFIG, TP_BIT_FLIPY );
//    RET_ON_ERROR();

    //--------------------

    tp_log( "TP: Secondary ID\n" );

    status = tp_command( TP_CMD_READ_SECONDARY_ID );
    RET_ON_ERROR();
    status = tp_recv_response( 2 ); // Mine reads: 0b01
    RET_ON_ERROR();

    ui_log_append_str( "TP: ID2 is [" );
    ui_log_append_byte( tp_response[ 1 ] );
    ui_log_append_byte( tp_response[ 0 ] );
    ui_log_append_str( "]\n" );
    display_draw( false );

    //--------------------

//    tp_log( "TP: Extended ID:\n" );
//
//    status = tp_command( TP_CMD_READ_EXTENDED_ID );
//    RET_ON_ERROR();
//    status = tp_recv_extended_id();
//    RET_ON_ERROR();

    //--------------------

    tp_log( "TP: Init ok\n" );
    return TP_OK;
}


/***************************************************************************/

// TP Command: Clear a bit in controller RAM
tp_status_t tp_ram_bit_clear( uint8_t location, uint8_t bit ) {

    // Ensure the TrackPoint is enabled:
    if ( ! initialized ) return TP_DISABLED;

    status = tp_ram_read( location );
    RET_ON_ERROR();

    status = tp_ram_write( location, tp_response[ 0 ] & ~(1<<bit) );
    RET_ON_ERROR();

    return TP_OK;
}


/***************************************************************************/

// TP Command: Set a bit in controller RAM
tp_status_t tp_ram_bit_set( uint8_t location, uint8_t bit ) {

    // Ensure the TrackPoint is enabled:
    if ( ! initialized ) return TP_DISABLED;

    status = tp_ram_read( location );
    RET_ON_ERROR();

    status = tp_ram_write( location, tp_response[ 0 ] | (1<<bit) );
    RET_ON_ERROR();

    return TP_OK;
}


/***************************************************************************/

// TP Command: Read controller RAM
tp_status_t tp_ram_read( uint8_t location ) {

    // Ensure the TrackPoint is enabled:
    if ( ! initialized ) return TP_DISABLED;

    if ( location <= 0x3f ) {
        status = tp_command( TP_CMD_RAM_READ_NEAR, location );
    } else {
        status = tp_command( TP_CMD_RAM_READ_FAR, location );
    }
    RET_ON_ERROR();

    // Receive the RAM contents:
    status = tp_recv_response( 1 );
    RET_ON_ERROR();

    return TP_OK;
}


/***************************************************************************/

// TP Command: Write controller RAM
tp_status_t tp_ram_write( uint8_t location, uint8_t value ) {

    // Ensure the TrackPoint is enabled:
    if ( ! initialized ) return TP_DISABLED;

    status = tp_command( TP_CMD_RAM_WRITE, location, value );
    RET_ON_ERROR();

    return TP_OK;
}


/***************************************************************************/

// TP Command: XOR controller RAM
tp_status_t tp_ram_xor( uint8_t location, uint8_t bitmask ) {

    // Ensure the TrackPoint is enabled:
    if ( ! initialized ) return TP_DISABLED;

    status = tp_command( TP_CMD_RAM_XOR, location, bitmask );
    RET_ON_ERROR();

    return TP_OK;
}


/***************************************************************************/

// TP Command: Read data
tp_status_t tp_read_data() {

    // Ensure the TrackPoint is enabled:
    if ( ! initialized ) return TP_DISABLED;

    status = tp_command( TP_CMD_READ_DATA );
    RET_ON_ERROR();
    status = tp_recv_response( 3 );
    RET_ON_ERROR();

    buttons_status = tp_response[ 0 ] & 7;
    return TP_OK;
}


/***************************************************************************/

tp_status_t tp_recv() {

    uint8_t r = ps2_host_recv_response();

    if ( ps2_error != PS2_ERR_NONE ) {
//        printf( "%sRecv error: x%02X.\n", ps2_prefix, ps2_error );
        return TP_PS2_ERROR;
    }

//    print( "Received: " );
//    phex( r );
//    print( ".\n" );
//    _delay_ms(100);

    tp_last_response_byte = r;
    return TP_OK;
}


/***************************************************************************/

/* Extended ID for my TrackPoint is as follows, though u8glib probably
 * skipped non-printable characters or character codes above 127.
 *
 * "M 19980216 RSO($IBM3780\\MOUSE\PNP0F19\IBM TrackPoint Version 4.0 YKT3B\7F)"
 *
 */
tp_status_t tp_recv_extended_id() {

    tp_zero_response();

    char extended_id[ 256 ];
    int i;

    for ( int i = 0; i < 256; i++ ) {

        status = tp_recv();

        if ( status != TP_OK ) {
            return status;
        }

        extended_id[ i ] = tp_last_response_byte;
        
        if ( tp_last_response_byte == ')' ) {
            extended_id[ i++ ] = '\\';
            extended_id[ i++ ] = 'n';
            extended_id[ i++ ] = 0;
            tp_log( extended_id );
            break;
        }
    }

    return TP_OK;
}


/***************************************************************************/

tp_status_t tp_recv_response( int num_bytes ) {

    tp_zero_response();

    for ( int i = 0; i < num_bytes; i++ ) {

        status = tp_recv();

        if ( status != TP_OK ) {
//            printf(
//                "%sRecv error: Failed to read byte %i of %i.\n",
//                tp_prefix, i, num_bytes
//            );
            return status;
        }

        tp_response[ i ] = tp_last_response_byte;
    }

    return TP_OK;
}


/***************************************************************************/

// TP Command: Reset.
tp_status_t tp_reset() {

    // Send reset command and get the response:
    status = tp_command( TP_CMD_RESET );
    RET_ON_ERROR();
//    _delay_ms(100); // BAT takes some time
    status = tp_recv_response( 2 );
    RET_ON_ERROR();

    // Post completion code should be 0xAA on success or 0xFC on error.
    switch ( tp_response[ 0 ] ) {
        case 0xaa:
//            print( "Reset 0xAA\n" );
//            _delay_ms(100);
            break;
        case 0xfc:

//            print( "Reset 0xFC\n" );
//            _delay_ms(100);

            // Read POST results:
//            status = tp_ram_read( TP_RAM_POST );
//            if ( status == TP_OK ) {
//                printf( "%sPOST results: x%02X.\n", tp_prefix, tp_response[ 0 ] );
//            }

            return TP_POST_FAIL;
        default:
//            printf(
//                "%sUnknown POST completion code: x%02X.\n",
//                tp_prefix, tp_response[ 0 ]
//            );
            return TP_POST_FAIL;
    }

    return TP_OK;
}


/***************************************************************************/

// Send a single byte and indicate whether the PS/2 transmission succeeded.
tp_status_t tp_send( uint8_t message ) {

//    print( "Sending " );
//    phex( message );
//    print( "...\n" );
//    _delay_ms(100);

    uint8_t r = ps2_host_send( message );

    if ( ps2_error != PS2_ERR_NONE ) {
//        printf( "%sSend error: x%02X.\n", ps2_prefix, ps2_error );
        return TP_PS2_ERROR;
    }

//    print( "ACK: " );
//    phex( r );
//    print( ".\n" );
//    _delay_ms(100);

    tp_last_response_byte = r;
    return TP_OK;
}


/***************************************************************************/

// Send a single command byte, verify ack, handle resend, etc.
tp_status_t tp_send_command_byte( uint8_t message ) {

    for ( int tries = 2; tries > 0; tries-- ) {

        // Send the command byte:
        status = tp_send( message );
        RET_ON_ERROR();

        switch ( tp_last_response_byte ) {

            case TP_CMD_ACK:
                return TP_OK;
                
            case TP_CMD_ERROR:
//                printf(
//                    "%sFailed to send command byte x%02X.\n",
//                    tp_prefix, message
//                );
                return TP_BAD_RESPONSE;

            case TP_CMD_RESEND:
                continue;
        }
        break;
    }

    // Shouldn't get here because the TrackPoint is only supposed
    // to return the resend response once.  At this point, maybe it
    // makes sense to reset the TrackPoint.

//    printf(
//        "%sFailed to send command byte x%02X (controller returned x%02X).\n",
//        tp_prefix, message, tp_last_response_byte
//    );
    return TP_FAIL;
}


/***************************************************************************/

void inline tp_zero_response() {
    for ( int i = 0; i < TP_RESPONSE_BUFFER_SIZE; i++ ) {
        tp_response[ i ] = 0;
    }
}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
