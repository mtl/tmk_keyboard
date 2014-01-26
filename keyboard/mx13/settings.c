/****************************************************************************
 *
 *  Persistent settings
 *
 ***************************************************************************/

#include <avr/eeprom.h> 
#include <avr/pgmspace.h> 
#include "settings.h"

#ifdef TRACKPOINT_ENABLE
#    include "trackpoint.h"
#endif


/***************************************************************************/
// Static prototoypes:

static uint8_t checksum( void *, int );
static bool initialize( void );
static bool settings_available( void );


/***************************************************************************/
// Reserved EEPROM memory:

//static setting_led_block_t EEMEM led_settings;
static setting_area_info_t EEMEM ee_areas;
static tp_config_t EEMEM tp_area;


/***************************************************************************/
// Globals:

static setting_area_info_t areas;


/***************************************************************************/

static uint8_t checksum( void * block, int size ) {

    uint8_t cs = 0;
    uint8_t * ptr = (uint8_t*) block;

    for ( int i = 0; i < size; i++ ) {
        cs += *ptr;
    }

    return cs;
}


/***************************************************************************/

static bool initialize() {

    static bool initialized = false;

    if ( ! initialized ) {

        if ( MX13_SET_INVALIDATE || ! settings_available() ) {
            areas.signifier = MX13_SET_SIGNIFIER;
            areas.leds.present = false;

#ifdef TRACKPOINT_ENABLE
            areas.trackpoint.present = false;
#endif

        }

        initialized = true;
    }

    return true;
}


/***************************************************************************/


static bool settings_available() {

    static bool available = false;

    if ( available ) {
        return true;
    }

    // Read the EEPROM storage area descriptions:
    eeprom_read_block( &areas, &ee_areas, sizeof( setting_area_info_t ) );
    if ( areas.signifier != MX13_SET_SIGNIFIER ) {
        return false;
    }

    // Basic sanity checks on EEPROM area types, sizes and locations.
    if ( false

#ifdef TRACKPOINT_ENABLE
        || ( areas.trackpoint.present && (
            areas.trackpoint.type != MX13_SET_TRACKPOINT ||
            areas.trackpoint.address != &tp_area ||
            areas.trackpoint.size != sizeof( tp_config_t )
        ) )
#endif
    ) {
        return false;
    }

    available = true;
    return true;
}


/***************************************************************************/

bool settings_load( setting_area_type_t type, void * block ) {

    // Initialization check:
    if ( ! initialize() ) {
        return false;
    }

    switch ( type ) {

#ifdef TRACKPOINT_ENABLE

        // Load TrackPoint settings:
        case MX13_SET_TRACKPOINT:

            if ( areas.trackpoint.present ) {

                // Read data from EEPROM:
                eeprom_read_block(
                    block, &tp_area, sizeof( tp_config_t )
                );

                // Validate checksum:
                return (
                    areas.trackpoint.checksum ==
                    checksum( block, sizeof( tp_config_t ) )
                );
            }
            break;
#endif

        // Load LED settings:
        case MX13_SET_LEDS:
            break;
    }

    return false;
}


/***************************************************************************/

bool settings_save( setting_area_type_t type, void * block ) {

    // Initialize areas info:
    if ( ! initialize() ) {
        return false;
    }

    bool updated = false;
    switch ( type ) {

#ifdef TRACKPOINT_ENABLE

        // Save TrackPoint settings:
        case MX13_SET_TRACKPOINT:

            // Write data to EEPROM:
            eeprom_write_block(
                block, &tp_area, sizeof( tp_config_t )
            );

            // Set area info:
            areas.trackpoint.present = true;
            areas.trackpoint.type = MX13_SET_TRACKPOINT;
            areas.trackpoint.address = &tp_area;
            areas.trackpoint.size = sizeof( tp_config_t );
            areas.trackpoint.checksum = checksum( block, sizeof( tp_config_t ) );
            updated = true;
            break;
#endif

        // Load LED settings:
        case MX13_SET_LEDS:
            return false;
    }

    if ( updated ) {
        eeprom_write_block(
            &areas, &ee_areas, sizeof( setting_area_info_t )
        );
        return true;
    }

    return false;
}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
