/****************************************************************************
 *
 *  Persistent settings
 *
 ***************************************************************************/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

#define EEPROM_LED_SETTINGS 1


/****************************************************************************
 * Typedefs
 ***************************************************************************/

typedef enum {

#ifdef TRACKPOINT_ENABLE
    MX13_SET_TRACKPOINT,
#endif

    MX13_SET_LEDS

} setting_area_type_t;

typedef struct {

    uint8_t present;
    setting_area_type_t type;
    void * address;
    uint16_t size;
    uint8_t checksum;

} setting_area_def_t;

typedef struct {

    uint16_t signifier;
    setting_area_def_t leds;

#ifdef TRACKPOINT_ENABLE
    setting_area_def_t trackpoint;
#endif

} setting_area_info_t;


/****************************************************************************
 * Constants and macros
 ***************************************************************************/

#define MX13_SET_INVALIDATE false
#define MX13_SET_SIGNIFIER 0x1313


/****************************************************************************
 * Externs
 ***************************************************************************/



/****************************************************************************
 * Prototypes
 ***************************************************************************/

bool settings_load( setting_area_type_t, void * );
bool settings_save( setting_area_type_t, void * );


/***************************************************************************/

#endif

/* vi: set et sts=4 sw=4 ts=4: */
