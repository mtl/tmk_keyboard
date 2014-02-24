/****************************************************************************
 *
 *  Persistent settings
 *
 ***************************************************************************/

#include <avr/eeprom.h> 
#include <avr/pgmspace.h> 
#include "settings.h"

#ifdef EEPROM_LED_SETTINGS
#    include "led-local.h"
#endif


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

#ifdef EEPROM_LED_SETTINGS
//static setting_led_block_t EEMEM led_settings;
static led_config_t EEMEM led_area;
#endif

static setting_area_info_t EEMEM ee_areas;

#ifdef TRACKPOINT_ENABLE
static tp_config_t EEMEM tp_area;
#endif


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

#ifdef EEPROM_LED_SETTINGS
            areas.leds.present = false;
#endif

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
    if (
        false

#ifdef EEPROM_LED_SETTINGS
        || ( areas.leds.present && (
            areas.leds.type != MX13_SET_LEDS ||
            areas.leds.address != &led_area ||
            areas.leds.size != sizeof( led_config_t )
        ) )
#endif

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

    void * eeprom_src_address = NULL;
    int size = 0;
    bool present = false;
    uint8_t expected_checksum = 0;

    switch ( type ) {

#ifdef TRACKPOINT_ENABLE

        // Load TrackPoint settings:
        case MX13_SET_TRACKPOINT:

            present = areas.trackpoint.present;
            eeprom_src_address = &tp_area;
            size = sizeof( tp_config_t );
            expected_checksum = areas.trackpoint.checksum;
            break;
#endif

#ifdef EEPROM_LED_SETTINGS

        // Load LED settings:
        case MX13_SET_LEDS:

            present = areas.leds.present;
            eeprom_src_address = &led_area;
            size = sizeof( led_config_t );
            expected_checksum = areas.leds.checksum;
            break;
#endif

        default: break;
    }

    if ( present ) {

        // Read data from EEPROM:
        eeprom_read_block( block, eeprom_src_address, size );

        // Validate checksum:
        return expected_checksum == checksum( block, size );
    }
    return false;
}


/***************************************************************************/

bool settings_save( setting_area_type_t type, void * block ) {

    // Initialize areas info:
    if ( ! initialize() ) {
        return false;
    }

    bool update = false;
    void * eeprom_dest_address = NULL;
    int size = 0;

    switch ( type ) {

#ifdef TRACKPOINT_ENABLE

        // Save TrackPoint settings:
        case MX13_SET_TRACKPOINT:

            areas.trackpoint.present = true;
            areas.trackpoint.type = MX13_SET_TRACKPOINT;
            areas.trackpoint.address = eeprom_dest_address = &tp_area;
            areas.trackpoint.size = size = sizeof( tp_config_t );
            areas.trackpoint.checksum = checksum( block, sizeof( tp_config_t ) );
            update = true;
            break;
#endif

#ifdef EEPROM_LED_SETTINGS

        // Load LED settings:
        case MX13_SET_LEDS:

            areas.leds.present = true;
            areas.leds.type = MX13_SET_LEDS;
            areas.leds.address = eeprom_dest_address = &led_area;
            areas.leds.size = size = sizeof( led_config_t );
            areas.leds.checksum = checksum( block, sizeof( led_config_t ) );
            update = true;
            break;
#endif

        default: break;
    }

    if ( update ) {

        // Write data to EEPROM:
        eeprom_write_block( block, eeprom_dest_address, size );

        // Write updated areas info:
        eeprom_write_block(
            &areas, &ee_areas, sizeof( setting_area_info_t )
        );
        return true;
    }

    return false;
}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
