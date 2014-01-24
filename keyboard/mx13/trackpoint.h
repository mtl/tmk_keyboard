/****************************************************************************
 *
 *  TrackPoint support
 *
 ***************************************************************************/

#ifndef MX13_TRACKPOINT_H
#define MX13_TRACKPOINT_H

#include <stdbool.h>
//#include "report.h"


/****************************************************************************
 * Typedefs
 ***************************************************************************/

typedef int TP_STATUS;


/****************************************************************************
 * Constants and macros
 ***************************************************************************/

#define TP_RESPONSE_BUFFER_SIZE 4

#define tp_ram_bit_toggle( location, bit ) tp_ram_xor( (location), (1<<(bit)) )

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

typedef enum {
    TP_RAM_REG00 = 0x00,
    TP_RAM_REG01,
    TP_RAM_STRCNT,
    TP_RAM_SAMCNT,
    TP_RAM_ANSPRTH = 0x04,
    TP_RAM_REG04 = 0x04,
    TP_RAM_REG05,
    TP_RAM_REG06,
    TP_RAM_REG07,
    TP_RAM_RATE,
    TP_RAM_ZPT,
    TP_RAM_RAMSAVE = 0x0a,
    TP_RAM_SAVER1 = 0x0a,
    TP_RAM_BPT,
    TP_RAM_RES,
    TP_RAM_ANSPRTL,
    TP_RAM_NBU,
    TP_RAM_FLIP,
    TP_RAM_PQTR,
    TP_RAM_LXMIT1,
    TP_RAM_LXMIT2,
    TP_RAM_LXMIT3,
    TP_RAM_QTR,
    TP_RAM_DFTCNT1,
    TP_RAM_MRATE,
    TP_RAM_XDEVTYP,
    TP_RAM_XMSB,
    TP_RAM_XLSB,
    TP_RAM_XPOT,
    TP_RAM_PPOT,
    TP_RAM_YMSB,
    TP_RAM_YLSB,
    TP_RAM_YPOT,
    TP_RAM_LASTMAG,
    TP_RAM_REG20, // bitfield
    TP_RAM_REG21, // bitfield
    TP_RAM_REG22, // bitfield
    TP_RAM_REG23, // bitfield
    TP_RAM_DRFTCNT2,
    TP_RAM_POST, // bitfield
    TP_RAM_MOUSTAT, // bitfield
    TP_RAM_CURSTAT, // bitfield
    TP_RAM_REG28, // bitfield
    TP_RAM_REG29, // bitfield
    TP_RAM_REG2A, // bitfield
    TP_RAM_REG2B, // bitfield
    TP_RAM_CONFIG, // bitfield
    TP_RAM_REG2D, // bitfield
    TP_RAM_REG2E, // bitfield
    TP_RAM_BDOWN,
    TP_RAM_XRAVGL,
    TP_RAM_XRAVGH,
    TP_RAM_YAVGL,
    TP_RAM_YAVGH,
    TP_RAM_PAVGL,
    TP_RAM_PAVGH,
    TP_RAM_XORIGIN,
    TP_RAM_YORIGIN,
    TP_RAM_PORIGIN,
    TP_RAM_BD,
    TP_RAM_LSLAST,
    TP_RAM_CURLAST,
    TP_RAM_WTHR,
    TP_RAM_XLAST,
    TP_RAM_YLAST,
    TP_RAM_PLAST,
    TP_RAM_CPT,
    TP_RAM_B1MASK,
    TP_RAM_B2MASK,
    TP_RAM_B3MASK,
    TP_RAM_DELAYL, // def:0x80 (see also DELAYHZ)
    TP_RAM_DELAYH, // def:0xfa
    TP_RAM_XYAVG_FACTOR, // def:0x80
    TP_RAM_OPADELAY, // def:0x74
    TP_RAM_DACDELAY, // def:0xc8
    TP_RAM_GAPDELAY, // def:0xc8
    TP_RAM_SNSTVTY, // def:0x80
    TP_RAM_LASTS,
    TP_RAM_HPDELAY, // def:0x32
    TP_RAM_INERTIA, // def:0x06
    TP_RAM_PDRIFTLIM, // def:0x03
    TP_RAM_PDRIFT_REL, // def:0x64
    TP_RAM_PDRIFTCNT,
    TP_RAM_BURST1, // def:0x3f
    TP_RAM_BURST2, // def:0x3d
    TP_RAM_BURST3, // def:0x3e
    TP_RAM_BDWPTR,
    TP_RAM_BDRPTR,
    TP_RAM_DIRFAC,
    TP_RAM_REACH,
    TP_RAM_DRAGHYS, // def:0xff
    TP_RAM_MINDRAG, // def:0x14
    TP_RAM_UTHR, // def:0xff
    TP_RAM_BUTRAD,
    TP_RAM_THR, // def:0x08
    TP_RAM_JKCUR, // def:0x87
    TP_RAM_ZTC, // def:0x26
    TP_RAM_RSTDFT1, // def:0x05
    TP_RAM_VALUE6, // def:0x61
    TP_RAM_MOVDEL, // def:0x26
    TP_RAM_DELAYHZ, // def:0xfd
    TP_RAM_DRIFT, // def:0xfe
    TP_RAM_XYDRIFTAVG, // def:0x40 or 0x80 (hw dep.)
    TP_RAM_XYAVGTHR, // def:0xff
    TP_RAM_SXY_LO = 0x66,
    TP_RAM_TPOT = 0x66,
    TP_RAM_ZTEMP = 0x66,
    TP_RAM_SXY_HI = 0x67,
    TP_RAM_POTTARGET = 0x67,
    TP_RAM_XTEMP = 0x68,
    TP_RAM_TORIGIN = 0x68,
    TP_RAM_YTEMP,
    TP_RAM_SAMCNTREL,
    TP_RAM_XLOWL,
    TP_RAM_XLOWH,
    TP_RAM_PSAM,
    TP_RAM_POLLDEL,
    TP_RAM_MXSTATE = 0x6f,
    TP_RAM_MREC_CNTR = 0x6f,
    TP_RAM_MBYTE1,
    TP_RAM_MBYTE2,
    TP_RAM_MBYTE = 0x72,
    TP_RAM_MXBYTE = 0x72,
    TP_RAM_MBYTE3,
    TP_RAM_SAVEBD,
    TP_RAM_YLOWL,
    TP_RAM_YLOWH,
    TP_RAM_MVDEL,
    TP_RAM_UWTHR,
    TP_RAM_UNASSIGNED_79,
    TP_RAM_BDLAST,
    TP_RAM_XLOWOFF,
    TP_RAM_YLOWOFF,
    TP_RAM_LOWX,
    TP_RAM_LOWY,
    TP_RAM_UNASSIGNED_7F,
    TP_RAM_DEBUG,
    TP_RAM_POT0,
    TP_RAM_UNASSIGNED_82,
    TP_RAM_UNASSIGNED_83,
    TP_RAM_UNASSIGNED_84,
    TP_RAM_UNASSIGNED_85,
    TP_RAM_BDS = 0x86,
    TP_RAM_ZF = 0x8a,
    TP_RAM_LS = 0x8f,
    TP_RAM_CUR = 0x94,
    TP_RAM_XB = 0xa9,
    TP_RAM_YB = 0xc2,
    TP_RAM_SP = 0xdb
} tp_ram_location_t;

enum TP_RAM_REG20_BITS {
    TP_BIT_MSKIP = 0x00,
    TP_BIT_MMOVE,
    TP_BIT_INVLD,
    TP_BIT_JOYSKIP,
    TP_BIT_SAMDIS, // def:0
    TP_BIT_CAUGHTUP,
    TP_BIT_REG20_6, // unassigned
    TP_BIT_TAGBIT // def:0
};

enum TP_RAM_REG21_BITS {
    TP_BIT_XSBIT = 0x00,
    TP_BIT_YSBIT,
    TP_BIT_MWAIT,
    TP_BIT_XDEVIN,
    TP_BIT_FLOPS,
    TP_BIT_REG21_5, // unassigned
    TP_BIT_LXMIT,
    TP_BIT_SXMIT
};

enum TP_RAM_REG22_BITS {
    TP_BIT_WRAP = 0x00,
    TP_BIT_STRANSP,
    TP_BIT_FORCEB3, // def:0
    TP_BIT_REG22_3, // reserved
    TP_BIT_SCALE,
    TP_BIT_MENB,
    TP_BIT_REMOTE,
    TP_BIT_REG22_7 // reserved
};

enum TP_RAM_REG23_BITS {
    TP_BIT_BLOCK3 = 0x00, // def:0
    TP_BIT_MCOMDIS, // def:0
    TP_BIT_POWERUP,
    TP_BIT_SKIPPOTS, // def:0
    TP_BIT_SETPOTS, // def:0
    TP_BIT_RERROR,
    TP_BIT_BYTE1X,
    TP_BIT_SKIPDRIFT // def:0
};

enum TP_RAM_POST_BITS {
    TP_BIT_RAMFAIL = 0x00,
    TP_BIT_ROMFAIL,
    TP_BIT_POST_2, // unassigned
    TP_BIT_XFAIL,
    TP_BIT_YFAIL,
    TP_BIT_MOUFAIL,
    TP_BIT_POST_6, // unassigned
    TP_BIT_POST_7 // unassigned
};

enum TP_RAM_MOUSTAT_BITS {
    TP_BIT_MLEFT = 0x00,
    TP_BIT_MRGHT,
    TP_BIT_MMIDB,
    TP_BIT_MOUSTAT_3, // unused
    TP_BIT_MOUSTAT_4, // x sign bit
    TP_BIT_MOUSTAT_5, // y sign bit
    TP_BIT_MOUSTAT_6, // overflow x bit
    TP_BIT_MOUSTAT_7  // overflow y bit
};

enum TP_RAM_CURSTAT_BITS {
    TP_BIT_LEFT = 0x00,
    TP_BIT_RIGHT,
    TP_BIT_MIDDLE,
    TP_BIT_CURSTAT_3, // unused, must be 1
    TP_BIT_XACBIT,
    TP_BIT_YACBIT,
    TP_BIT_OVERX,
    TP_BIT_OVERY
};

enum TP_RAM_REG28_BITS {
    TP_BIT_LSSIGN1 = 0x00,
    TP_BIT_LSSIGN2,
    TP_BIT_HYSFLG,
    TP_BIT_UPHIT,
    TP_BIT_JKFLG,
    TP_BIT_REL,
    TP_BIT_REG28_6, // unassigned
    TP_BIT_KBURST // def:0
};

enum TP_RAM_REG29_BITS {
    TP_BIT_REG29_0 = 0x00, // unassigned
    TP_BIT_DACDBB,
    TP_BIT_M_DIR,
    TP_BIT_DAT_BIT,
    TP_BIT_ARB,
    TP_BIT_QUIET,
    TP_BIT_TPTURN,
    TP_BIT_BACKING
};

enum TP_RAM_REG2A_BITS {
    TP_BIT_E2MATCH = 0x00,
    TP_BIT_MRESET,
    TP_BIT_DOS4FIX,
    TP_BIT_PSBIT,
    TP_BIT_XCIP,
    TP_BIT_YCIP,
    TP_BIT_PCIP,
    TP_BIT_SKIPZ
};

enum TP_RAM_REG2B_BITS {
    TP_BIT_MBIT1 = 0x00,
    TP_BIT_MBIT2,
    TP_BIT_MBIT3,
    TP_BIT_NEW_MBYTE,
    TP_BIT_LONG_MOUSE,
    TP_BIT_MPENDING,
    TP_BIT_MBUSY,
    TP_BIT_MTIMEOUT
};

enum TP_RAM_CONFIG_BITS {
    TP_BIT_PTSON = 0x00, // def:0
    TP_BIT_HALFTAC, // def:0
    TP_BIT_BUTTON2,
    TP_BIT_FLIPX, // def:0
    TP_BIT_FLIPY, // def:0
    TP_BIT_FLIPZ, // def:0
    TP_BIT_REG2C_6, // unassigned
    TP_BIT_FTRANS
};

enum TP_RAM_REG2D_BITS {
    TP_BIT_TWO_HANDED = 0x00, // def:0
    TP_BIT_NMBBIT, // def:0
    TP_BIT_STICKY2, // def:0
    TP_BIT_SKIPBACK, // def:0
    TP_BIT_REMMOUENB, // def:0
    TP_BIT_SKIPZSTEP, // def:0
    TP_BIT_MSFIX, // def:0
    TP_BIT_NOSYNC // def:0
};

enum TP_RAM_REG2E_BITS {
    TP_BIT_SAVEET1 = 0x00,
    TP_BIT_SAVETR1,
    TP_BIT_MPARITY,
    TP_BIT_DRIFTING,
    TP_BIT_STEPPING,
    TP_BIT_SKIPTAC, // def:0
    TP_BIT_BAD_COMMAND,
    TP_BIT_STOPF4 // def:0
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
TP_STATUS tp_init( void );
TP_STATUS tp_ram_bit_clear( uint8_t, uint8_t );
TP_STATUS tp_ram_bit_set( uint8_t, uint8_t );
TP_STATUS tp_ram_read( uint8_t );
TP_STATUS tp_ram_write( uint8_t, uint8_t );
TP_STATUS tp_ram_xor( uint8_t, uint8_t );
TP_STATUS tp_read_data( void );
TP_STATUS tp_recv( void );
TP_STATUS tp_recv_response( int );
TP_STATUS tp_reset( void );
TP_STATUS tp_send( uint8_t );
TP_STATUS tp_send_command_byte( uint8_t );
void tp_zero_response( void );


/***************************************************************************/

#endif

/* vi: set et sts=4 sw=4 ts=4: */
