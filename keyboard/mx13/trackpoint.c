/****************************************************************************
 *
 *  TrackPoint support
 *
 *  by Mike Ter Louw
 *
 *  Public Domain
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

/*

Settings:

    Hard set - Once configured, a hard reset is required to reconfigure
        What about power-on-reset (e2 7f) command?
    Medium set - Once configured, will persist across a soft reset
    Soft set - Once configured, will revert to defaults on a soft reset
    Unconfigurable - Either not a configurable setting or not documented

    location - HS - MS - SS - UC - Default

    location - value


    Non-RAM config items:
        trackpoint enabled/disabled
        soft/hard transparent modes
        data report modes?
        sample rate (2 bytes)
        stream mode
        remote mode
        wrap mode
        resolution 02
        scaling reset
        burst mode (ram config)


    Config interactions
        reset command and sample rate, stream mode, disabled, resolution, scaling
        set sample rate command, two_handed and sticky2 bits
        soft/hard resets, cancel soft transparent mode command,  and soft/hard transparent modes

    After hard reset, learn default value for TP_RAM_XYDRIFTAVG.


*/

/*


    If hard reset, just apply default-to-config delta
    If soft reset or defaults, set defaults for medium items, then apply default-to-config delta
        Easier just to hard reset!


*/

/*
typedef enum {
    TP_HARD_RESET,
    TP_SOFT_RESET,
    TP_DEFAULTS,
    TP_CONFIGURED
} tp_config_state_t;
*/

/***************************************************************************/

#define RET_ON_ERROR() if ( status != TP_OK ) return status;

#define tp_log( msg ) \
    ui_log_append_str( (msg) ); \
    display_draw( false );

// Hard set - Once configured, a hard reset is required to reconfigure.
tp_ram_location_info_t tp_ram_hard_sets[] = {

    // Setting hard transparent mode via the TP_CMD_SET_HARD_TRANS_MODE
    // command is sticky until a hard reset, however it isn't documented
    // that this is tied to the FTRANS bit.  Need to confirm.
    { .location = TP_RAM_CONFIG, .value = (1<<TP_BIT_FTRANS) }
};

// Medium set - Once configured, will persist across a soft reset or set
// defaults command.
// Every item in this array *must* have a corresponding item in the
// tp_ram_defaults[] array.
tp_ram_location_info_t tp_ram_medium_sets[] = {

    { TP_RAM_REG20,
        (1<<TP_BIT_SAMDIS) |
        (1<<TP_BIT_TAGBIT)
    },
    { TP_RAM_REG22,
        (1<<TP_BIT_FORCEB3)
    },
    { TP_RAM_REG23,
        (1<<TP_BIT_BLOCK3) |
        (1<<TP_BIT_MCOMDIS) |
        (1<<TP_BIT_SKIPPOTS) |
        (1<<TP_BIT_SETPOTS) |
        (1<<TP_BIT_SKIPDRIFT)
    },
    { TP_RAM_CONFIG,
        (1<<TP_BIT_PTSON) |
        (1<<TP_BIT_HALFTAC) |
        (1<<TP_BIT_BUTTON2) |
        (1<<TP_BIT_FLIPX) |
        (1<<TP_BIT_FLIPY) |
        (1<<TP_BIT_FLIPZ) |
        (1<<TP_BIT_SWAPXY)
    },
    { TP_RAM_REG2D,
        (1<<TP_BIT_NMBBIT) |
        (1<<TP_BIT_STICKY2) |
        (1<<TP_BIT_SKIPBACK) |
        (1<<TP_BIT_REMMOUENB) |
        (1<<TP_BIT_SKIPZSTEP) |
        (1<<TP_BIT_MSFIX)
    },
    { TP_RAM_REG2E,
        (1<<TP_BIT_SKIPTAC) |
        (1<<TP_BIT_STOPF4)
    },
    { TP_RAM_DELAYL, 0xff },
    { TP_RAM_DELAYH, 0xff },
    { TP_RAM_XYAVG_FACTOR, 0xff },
    { TP_RAM_OPADELAY, 0xff },
    { TP_RAM_DACDELAY, 0xff },
    { TP_RAM_GAPDELAY, 0xff },
    { TP_RAM_SNSTVTY, 0xff },
    { TP_RAM_HPDELAY, 0xff },
    { TP_RAM_INERTIA, 0xff },
    { TP_RAM_PDRIFTLIM, 0xff },
    { TP_RAM_PDRIFT_REL, 0xff },
    { TP_RAM_BURST1, 0xff },
    { TP_RAM_BURST2, 0xff },
    { TP_RAM_BURST3, 0xff },
    { TP_RAM_REACH, 0xff },
    { TP_RAM_DRAGHYS, 0xff },
    { TP_RAM_MINDRAG, 0xff },
    { TP_RAM_UTHR, 0xff },
    { TP_RAM_THR, 0xff },
    { TP_RAM_JKCUR, 0xff },
    { TP_RAM_ZTC, 0xff },
    { TP_RAM_RSTDFT1, 0xff },
    { TP_RAM_VALUE6, 0xff },
    { TP_RAM_MOVDEL, 0xff },
    { TP_RAM_DELAYHZ, 0xff },
    { TP_RAM_DRIFT, 0xff },
    { TP_RAM_XYDRIFTAVG, 0xff },
    { TP_RAM_XYAVGTHR, 0xff }

};

// Soft set - Once configured, will revert to defaults on a soft reset or
// set defaults command.
tp_ram_location_info_t tp_ram_soft_sets[] = {

    { TP_RAM_REG28, (1<<TP_BIT_KBURST) },
    { TP_RAM_REG2D, (1<<TP_BIT_NOSYNC ) }

};

// Unconfigurable - Either not a configurable setting or not documented.
tp_ram_location_info_t tp_ram_no_sets[] = {

    { TP_RAM_REG21, (1<<TP_BIT_XDEVIN) },
    { TP_RAM_POST, 0xff }

};

// Documented default values for RAM locations.
tp_ram_location_info_t tp_ram_defaults[] = {

    // Bitfields:
    { TP_RAM_REG20, 0x00 },
    { TP_RAM_REG22, 0x00 },
    { TP_RAM_REG23, 0x00 },
    { TP_RAM_REG28, 0x00 },
    { TP_RAM_CONFIG, ((!TP_THREE_BUTTONS)<<TP_BIT_BUTTON2) },
    { TP_RAM_REG2D, 0x00 },
    { TP_RAM_REG2E, 0x00 },

    // Byte values:
    { TP_RAM_DELAYL, 0x80 },
    { TP_RAM_DELAYH, 0xfa },
    { TP_RAM_XYAVG_FACTOR, 0x80 },
    { TP_RAM_OPADELAY, 0x74 },
    { TP_RAM_DACDELAY, 0xc8 },
    { TP_RAM_GAPDELAY, 0xc8 },
    { TP_RAM_SNSTVTY, 0x80 },
    { TP_RAM_HPDELAY, 0x32 },
    { TP_RAM_INERTIA, 0x06 },
    { TP_RAM_PDRIFTLIM, 0x03 },
    { TP_RAM_PDRIFT_REL, 0x64 },
    { TP_RAM_BURST1, 0x3f },
    { TP_RAM_BURST2, 0x3d },
    { TP_RAM_BURST3, 0x3e },
    { TP_RAM_REACH, 0x0a },
    { TP_RAM_DRAGHYS, 0xff },
    { TP_RAM_MINDRAG, 0x14 },
    { TP_RAM_UTHR, 0xff },
    { TP_RAM_THR, 0x08 },
    { TP_RAM_JKCUR, 0x87 },
    { TP_RAM_ZTC, 0x26 },
    { TP_RAM_RSTDFT1, 0x05 },
    { TP_RAM_VALUE6, 0x61 },
    { TP_RAM_MOVDEL, 0x26 },
    { TP_RAM_DELAYHZ, 0xfd },
    { TP_RAM_DRIFT, 0xfe },
    { TP_RAM_XYDRIFTAVG, 0x40 }, // or 0x80
    { TP_RAM_XYAVGTHR, 0xff }

};



/***************************************************************************/

// Globals:
static uint8_t buttons_status = 0;
static bool initialized = false;
//static const char * ps2_prefix = "[PS2] ";
static int status = 0; // tp_status_t of last operation
uint8_t tp_last_response_byte = 0; // Most recent byte returned by the TP
//static const char * tp_prefix = "[TP] ";
uint8_t tp_response[ TP_RESPONSE_BUFFER_SIZE ]; // Response to most recent TP command

// Middle-button-scroll is implemented by dividing cursor movement by these amounts:
int tp_sensitivity = 0xc0;
int tp_scroll_divisor_h = 2;
int tp_scroll_divisor_v = 2;

static int num_defaults = (
    sizeof( tp_ram_defaults ) /
    sizeof( tp_ram_defaults[ 0 ] )
);

static int num_hard_sets = (
    sizeof( tp_ram_hard_sets ) /
    sizeof( tp_ram_hard_sets[ 0 ] )
);

static int num_medium_sets = (
    sizeof( tp_ram_medium_sets ) /
    sizeof( tp_ram_medium_sets[ 0 ] )
);

static int num_no_sets = (
    sizeof( tp_ram_no_sets ) /
    sizeof( tp_ram_no_sets[ 0 ] )
);

static int num_soft_sets = (
    sizeof( tp_ram_soft_sets ) /
    sizeof( tp_ram_soft_sets[ 0 ] )
);


/***************************************************************************/

// Convert an ASCII character (e.g., "1", "A", "e", etc.) to a nibble.
uint8_t char_to_nib( uint8_t c ) {

    // Digits:
    if ( c >= 0x30 && c <= 0x39 ) {
        return c - 0x30;
    }

    // Caps:
    if ( c >= 0x41 && c <= 0x46 ) {
        return c - 0x41 + 10;
    }

    // Lower case:
    if ( c >= 0x61 && c <= 0x66 ) {
        return c - 0x61 + 10;
    }

    return -1;
}


/***************************************************************************/

// Initialize the TrackPoint.
static tp_status_t initialize() {
    
    // Clear response buffer:
    tp_zero_response();

//    tp_log( "TP: Resetting\n" );

    // Reset the TrackPoint:
    tp_reset( false );
    initialized = true;
    //debug_config.mouse = true;

//    tp_log( "TP: Enabling reports\n" );

    // Enable the TrackPoint (starts data reporting):
    status = tp_command( TP_CMD_ENABLE );
    RET_ON_ERROR();

//    tp_log( "TP: Set remote\n" );

    // Set remote mode:
    status = tp_command( TP_CMD_SET_REMOTE_MODE );
    RET_ON_ERROR();

    //--------------------

//    tp_log( "TP: Enable PtS\n" );

    status = tp_ram_bit_set( TP_RAM_CONFIG, TP_BIT_PTSON );
    RET_ON_ERROR();

    //--------------------

//    tp_log( "TP: Sens. factor\n" );

    status = tp_ram_write( TP_RAM_SNSTVTY, tp_sensitivity );
    RET_ON_ERROR();

    //--------------------

//    tp_log( "TP: Invert Y\n" );
//
//    status = tp_ram_bit_clear( TP_RAM_CONFIG, TP_BIT_FLIPY );
//    RET_ON_ERROR();

    //--------------------

//    tp_log( "TP: Secondary ID\n" );

    status = tp_command( TP_CMD_READ_SECONDARY_ID );
    RET_ON_ERROR();
    status = tp_recv_response( 2 ); // Mine reads: 0b01
    RET_ON_ERROR();

//    ui_log_append_str( "TP: ID2 is [" );
//    ui_log_append_byte( tp_response[ 1 ] );
//    ui_log_append_byte( tp_response[ 0 ] );
//    ui_log_append_str( "]\n" );
//    display_draw( false );

    //--------------------

//    tp_log( "TP: Extended ID:\n" );

//    tp_extended_id_t extended_id;
//    status = tp_recv_extended_id( &extended_id );
//    RET_ON_ERROR();

    //--------------------

//    tp_log( "TP: Init ok\n" );
    return TP_OK;
}


/***************************************************************************/

static uint8_t lookup_result;

static tp_status_t lookup(
    tp_ram_location_info_t ** list,
    int size, int location, uint8_t * result
) {
    int i = 0;
    while ( i < size && i <= location ) {

        if ( (*list)[ i ].location == location ) {
            *result = (*list)[ i ].value;
            return TP_OK;
        }
        i++;
    }

    return TP_FAIL;
}


/***************************************************************************/

// Receive a single response byte.
static tp_status_t recv() {

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

// Send a single byte and indicate whether the PS/2 transmission succeeded.
static tp_status_t send( uint8_t message ) {

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
static tp_status_t send_command_byte( uint8_t message ) {

    for ( int tries = 2; tries > 0; tries-- ) {

        // Send the command byte:
        status = send( message );
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

// Send a sequence of command bytes.
tp_status_t tp_do_command( int num_bytes, ... ) {

    // Ensure the TrackPoint is enabled:
    if ( ! initialized ) return TP_DISABLED;

    va_list ap;
    va_start( ap, num_bytes );

    for ( int i = 0; i < num_bytes; i++ ) {

        status = send_command_byte( va_arg( ap, int ) );
        switch ( status ) {
            case TP_OK: continue;
            case TP_BAD_RESPONSE:
//                if ( tp_last_response_byte == 0xfe ) {
//                    printf( "%sCommand not supported.\n", tp_prefix );
//                }

            // send_command_byte() also returns TP_FAIL.
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

tp_status_t tp_get_current_config( tp_config_t * config ) {

    uint8_t default_value = 0;

    config->num_items = 0;

    // Configure each provided RAM location:
    for ( int i = 0; i < num_medium_sets; i++ ) {

        // Get the next config location and value:
        tp_ram_location_info_t * info = &tp_ram_medium_sets[ i ];

        // Look up the default value:
        status = lookup(
            (tp_ram_location_info_t **) &tp_ram_defaults,
            num_defaults, info->location, &default_value
        );
        RET_ON_ERROR();

        // Only consider configurable bits:
        default_value &= info->value;

        // Read the current configuration:
        status = tp_ram_read( info->location );
        RET_ON_ERROR();

        // Store the current configuration if needed:
        uint8_t value = tp_last_response_byte & info->value;
        if ( value != default_value ) {
            config->items[ config->num_items ].location = info->location;
            config->items[ config->num_items++ ].value = value;
        }
    }

    return TP_OK;
}


/***************************************************************************/

// Initialize the TrackPoint.
tp_status_t tp_init() {

    display_busy( true );
    status = initialize();
    display_busy( false );

    return status;
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

// Read and parse the extended ID:
tp_status_t tp_recv_extended_id( tp_extended_id_t * extended_id ) {

    // Issue command to read the extended ID:
    status = tp_command( TP_CMD_READ_EXTENDED_ID );
    RET_ON_ERROR();

    // Initialize data structure:
    uint8_t * eid = (uint8_t*) &extended_id;
    for ( int i = 0; i < sizeof( tp_extended_id_t ); i++ ) {
        *eid++ = 0;
    }

    // Initialize more data:
    tp_zero_response();
    tp_extended_id_state_t state = TP_OTHER_ID;
    uint8_t checksum = 0;
    int field_pos = 0;

    // Extended ID is 256 bytes max:
    for ( int i = 0; i < 256; i++ ) {

        // Get the next byte:
        status = recv();
        RET_ON_ERROR();

        // Add to the checksum:
        checksum += tp_last_response_byte;

        // Handle the current byte:
        switch ( state ) {

            // "M 19980216 RSO"
            case TP_OTHER_ID:
                if ( tp_last_response_byte != '(' ) {
                    extended_id->other_id[ field_pos++ ] = tp_last_response_byte;
                } else {
                    field_pos = 0;
                    checksum = '(';
                    state = TP_PNP_REVISION_LEVEL;
                }
                continue;

            // 0x64 == 1.0
            case TP_PNP_REVISION_LEVEL:
                switch ( field_pos++ ) {
                    case 0:
                        extended_id->pnp_revision_level = tp_last_response_byte;
                        break;
                    case 1:
                        extended_id->pnp_revision_level = (
                            ( ( extended_id->pnp_revision_level & 0x3f ) << 6 ) |
                            ( tp_last_response_byte & 0x3f )
                        );
                        field_pos = 0;
                        state = TP_MANUFACTURER_ID;
                }
                continue;

            // "IBM"
            case TP_MANUFACTURER_ID:
                extended_id->manufacturer_id[ field_pos++ ] = tp_last_response_byte;
                if ( field_pos == 3 ) {
                    field_pos = 0;
                    state = TP_PRODUCT_NO;
                }
                continue;

            // "378"
            case TP_PRODUCT_NO:
                extended_id->product_no[ field_pos++ ] = tp_last_response_byte;
                if ( field_pos == 3 ) {
                    field_pos = 0;
                    state = TP_PRODUCT_REVISION;
                }
                continue;

            // "0" or "1" (depending on BUTTON2 bit of CONFIG register)
            case TP_PRODUCT_REVISION:
                extended_id->product_revision = tp_last_response_byte;
                state = TP_SERIAL_NO_OPTION;
                continue;

            case TP_SERIAL_NO_OPTION:
                if ( tp_last_response_byte != '\\' ) {
                    // this is invalid
                    // read out until close parenthesis and return a failure code
                }
                state = TP_SERIAL_NO;
                continue;

            // "\" == not provided (optional field)
            case TP_SERIAL_NO:
                if ( field_pos == 0 && tp_last_response_byte == '\\' ) {
                    state = TP_CLASS_ID;
                } else {
                    extended_id->serial_no = (
                        ( extended_id->serial_no << 4 ) |
                        char_to_nib( tp_last_response_byte )
                    );
                    if ( field_pos++ == 8 ) {
                        field_pos = 0;
                        state = TP_CLASS_ID_OPTION;
                    }
                }
                continue;

            case TP_CLASS_ID_OPTION:
                if ( tp_last_response_byte != '\\' ) {
                    // this is invalid
                    // read out until close parenthesis and return a failure code
                }
                state = TP_CLASS_ID;
                continue;

            // "MOUSE"
            case TP_CLASS_ID:
                if ( field_pos >= 32 ) {
                    // this is invalid
                    // read out until close parenthesis and return a failure code
                }
                if ( tp_last_response_byte == '\\' ) {
                    field_pos = 0;
                    state = TP_DRIVER_ID;
                } else {
                    extended_id->class_id[ field_pos++ ] = tp_last_response_byte;
                }
                continue;

            // "PNP0F19"
            case TP_DRIVER_ID:
                if ( field_pos >= 40 ) {
                    // this is invalid
                    // read out until close parenthesis and return a failure code
                }
                if ( tp_last_response_byte == '\\' ) {
                    field_pos = 0;
                    state = TP_USER_NAME;
                } else {
                    extended_id->driver_id[ field_pos++ ] = tp_last_response_byte;
                }
                continue;

            // "IBM TrackPoint Version 4.0 YKT3B"
            case TP_USER_NAME:
                if ( field_pos >= 40 ) {
                    // this is invalid
                    // read out until close parenthesis and return a failure code
                }
                if ( tp_last_response_byte == '\\' ) {
                    field_pos = 0;
                    state = TP_CHECKSUM;
                } else {
                    extended_id->user_name[ field_pos++ ] = tp_last_response_byte;
                }
                continue;

            // "7F"
            case TP_CHECKSUM:

                checksum -= tp_last_response_byte;
                switch ( field_pos++ ) {
                    case 0:
                        extended_id->checksum = char_to_nib( tp_last_response_byte );
                        break;
                    case 1:
                        extended_id->checksum = (
                            ( extended_id->checksum << 4 ) |
                            char_to_nib( tp_last_response_byte )
                        );

                        field_pos = 0;
                        state = TP_END_PNP;
                }
                continue;

            // ")"
            case TP_END_PNP:
                if ( tp_last_response_byte != ')' ) {
                    // this is invalid
                    // return a failure code
                }
                state = TP_EXTENDED_ID_DONE;
                extended_id->checksum_counted = checksum;

            case TP_EXTENDED_ID_DONE:
                break;
        }

        if ( state == TP_EXTENDED_ID_DONE ) {
            break;
        }
    }

    return TP_OK;
}


/***************************************************************************/

// Receive the given number of bytes and store them into the response buffer.
tp_status_t tp_recv_response( int num_bytes ) {

    tp_zero_response();

    for ( int i = 0; i < num_bytes; i++ ) {

        status = recv();
        RET_ON_ERROR();

//        if ( status != TP_OK ) {
//            printf(
//                "%sRecv error: Failed to read byte %i of %i.\n",
//                tp_prefix, i, num_bytes
//            );
//            return status;
//        }

        tp_response[ i ] = tp_last_response_byte;
    }

    return TP_OK;
}


/***************************************************************************/

// TP Command: Reset.
tp_status_t tp_reset( bool hard ) {

    // Send reset command and get the response:
    if ( hard ) {
        status = tp_command( TP_CMD_POWER_ON_RESET );
    } else {
        status = tp_command( TP_CMD_RESET );
    }
    RET_ON_ERROR();
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

tp_status_t tp_set_current_config( tp_config_t * config ) {

    uint8_t config_bitmask = 0;

    // Execute hard reset to put device config into known state:
    status = tp_reset( true );
    RET_ON_ERROR();

    // Configure each provided RAM location:
    for ( int i = 0; i < config->num_items; i++ ) {

        // Get the next config location and value:
        tp_ram_location_info_t * info = &config->items[ i ];

        // Look up the configurable bits:
        status = lookup(
            (tp_ram_location_info_t **) &tp_ram_defaults,
            num_defaults, info->location, &config_bitmask
        );
        RET_ON_ERROR();

        // Read the current device config:
        status = tp_ram_read( info->location );
        RET_ON_ERROR();
        uint8_t ram_value = tp_last_response_byte;

        // Configure the location if needed:
        uint8_t current_config = ram_value & config_bitmask;
        uint8_t requested_config = info->value & config_bitmask;
        if ( current_config != requested_config ) {

            // Merge configurable bits with unconfigurable bits:
            uint8_t new_config = (
                ( ram_value & ~config_bitmask ) |
                requested_config
            );

            // The spec says this bit must be set:
            if ( info->location == TP_RAM_CURSTAT ) {
                new_config |= (1<<TP_BIT_CURSTAT_3);
            }

            tp_ram_write( info->location, new_config );
            RET_ON_ERROR();
        }
    }

    return TP_OK;
}


/***************************************************************************/

// Clear the response buffer.
void inline tp_zero_response() {
    for ( int i = 0; i < TP_RESPONSE_BUFFER_SIZE; i++ ) {
        tp_response[ i ] = 0;
    }
}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
