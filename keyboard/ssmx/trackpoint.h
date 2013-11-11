/****************************************************************************
 *
 *  TrackPoint support
 *
 ***************************************************************************/

#ifndef TRACKPOINT_H
#define TRACKPOINT_H

#include <stdbool.h>
#include "report.h"


/****************************************************************************
 * Typedefs
 ***************************************************************************/

typedef int TP_STATUS;


/****************************************************************************
 * Constants and macros
 ***************************************************************************/

#define TP_RESPONSE_BUFFER_SIZE 4

// Status codes:
#define TP_OK 0
#define TP_DISABLED 1
#define TP_FAIL 2
#define TP_PS2_ERROR 3
#define TP_BAD_RESPONSE 4
#define TP_POST_FAIL 5

#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1)
#define VA_NUM_ARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, N, ...) N
#define tp_command(...) \
    _tp_command( VA_NUM_ARGS( __VA_ARGS__ ), __VA_ARGS__ )

// Command byte responses:
#define TP_CMD_ACK 0xfa
#define TP_CMD_RESEND 0xfe
#define TP_CMD_ERROR 0xfc

// Commands:
#define TP_CMD_SET_REMOTE_MODE 0xf0
#define TP_CMD_SET_STREAM_MODE 0xea
#define TP_CMD_ENABLE 0xf4
#define TP_CMD_DISABLE 0xf5
#define TP_CMD_READ_DATA 0xeb
#define TP_CMD_RESET 0xff

enum tp_ram_locations {
    TP_RAM_POST_REG = 0x25
};


/****************************************************************************
 * Externs
 ***************************************************************************/

extern uint8_t tp_last_response_byte;
extern uint8_t tp_response[];


/****************************************************************************
 * Prototypes
 ***************************************************************************/

TP_STATUS _tp_command( int, ... );
TP_STATUS tp_get_report( report_mouse_t * );
TP_STATUS tp_read_data( void );
TP_STATUS tp_ram_read( uint8_t );
TP_STATUS tp_ram_write( uint8_t, uint8_t );
TP_STATUS tp_ram_xor( uint8_t, uint8_t );
bool tp_ready( void );
TP_STATUS tp_recv( void );
TP_STATUS tp_recv_response( int );
TP_STATUS tp_reset( void );
TP_STATUS tp_send( uint8_t );
TP_STATUS tp_send_command_byte( uint8_t );
void tp_zero_response( void );
TP_STATUS trackpoint_init( void );
TP_STATUS trackpoint_poll( void );


/***************************************************************************/

#endif

/* vi: set et sts=4 sw=4 ts=4: */
