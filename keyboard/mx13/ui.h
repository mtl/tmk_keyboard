/****************************************************************************
 *
 *  User Interface
 *
 ***************************************************************************/

#ifndef MX13_UI_H
#define MX13_UI_H

#include <stdbool.h>
#include "keymap.h"
#include "led-local.h"
#include "u8g.h"


/****************************************************************************
 * Typedefs
 ***************************************************************************/

struct ui_menu_item;

// Numbers:
typedef enum {

#ifdef TRACKPOINT_ENABLE
    UI_NUM_TP_RAM_BYTE_START,
    //------------------------
    UI_NUM_TP_SNSTVTY,
    UI_NUM_TP_INERTIA,
    UI_NUM_TP_VALUE6,
    UI_NUM_TP_REACH,
    UI_NUM_TP_DRAGHYS,
    UI_NUM_TP_MINDRAG,
    UI_NUM_TP_THR,
    UI_NUM_TP_UTHR,
    UI_NUM_TP_ZTC,
    UI_NUM_TP_JKCUR,
    UI_NUM_TP_RSTDFT1,
    UI_NUM_TP_XYDRIFTAVG,
    UI_NUM_TP_XYAVGTHR,
    UI_NUM_TP_PDRIFTLIM,
    UI_NUM_TP_PDRIFT_REL,
    UI_NUM_TP_DRIFT,
    UI_NUM_TP_XYAVG_FACTOR,
    //------------------------
    UI_NUM_TP_RAM_BYTE_END,


    UI_NUM_TP_RAM_BIT_START,
    //------------------------
    UI_NUM_TP_CONFIG,
    UI_NUM_TP_REG23,
    UI_NUM_TP_REG2D,
    //------------------------
    UI_NUM_TP_RAM_BIT_END,

/*
            UI_MENU_ITEM_DUMMY( "VScroll speed" ), // 0 disables?
            UI_MENU_ITEM_DUMMY( "HScroll speed" ), // 0 disables?
                UI_MENU_ITEM_DUMMY( "Recalibrate now" ), // must wait 310 ms after E2 51

            info:
            UI_MENU_ITEM_DUMMY( "Page 1" ),
            UI_MENU_ITEM_DUMMY( "Page 2" ),
            UI_MENU_ITEM_DUMMY( "Page 3" ),
            UI_MENU_ITEM_DUMMY( "Page 4" )

            UI_MENU_ITEM_DUMMY( "Reload config" ), // reapply config from EEPROM
            UI_MENU_ITEM_DUMMY( "View POST results" ),
            UI_MENU_ITEM_DUMMY( "Reset to defaults" ),
            UI_MENU_ITEM_DUMMY( "Soft reset" ),
            UI_MENU_ITEM_DUMMY( "Hard reset" )
*/

    //------------------------
#endif

    UI_NUM_LED_TP_INTENSITY

} ui_number_t;

// A menu:
typedef struct ui_menu {

    char * title;
//    u8g_pgm_uint8_t U8G_PROGMEM title;
    //u8g_pgm_uint8_t * title;
    int num_items;
    struct ui_menu_item * items;

} ui_menu_t;

// Types of menu items:
typedef enum {

    UI_CONFIRM,         // A confirmation screen
    UI_DUMMY,           // A dummy menu item
    UI_EDITOR,          // A text editor
    UI_EXIT,            // Exit the UI
    UI_FLAG,            // A toggleable flag
    UI_LED_CONFIG,      // LED configuration
    UI_NAV_PREV,        // Navigate to the previous screen
    UI_NUM_SELECTOR,    // Select a numeric value
    UI_RGB_SELECTOR,    // Select an RGB value
    UI_SAVE,            // Save changes
    UI_SUBMENU          // Navigate to a submenu

} ui_menu_item_type_t;

// A menu item:
typedef struct ui_menu_item {

    ui_menu_item_type_t type;
    char * label;
    //u8g_pgm_uint8_t label;

    struct {
        union {
            led_indices_t led_channel;  // UI_LED_CONFIG, UI_RGB_SELECTOR
            ui_menu_t submenu;          // UI_SUBMENU
            ui_number_t number;         // UI_NUM_SELECTOR, UI_FLAG
        };
        union {
            uint8_t bit;                // UI_FLAG
        };
    };

} ui_menu_item_t;

// Input modes:
typedef enum {

    UI_INPUT_MENU,            // Navigating the menus
    UI_INPUT_YES_NO,          // Binary choice
    UI_INPUT_EDIT,            // Editing text
    UI_INPUT_LOG,             // Log
    UI_INPUT_NUM,             // Decimal number entry
    UI_INPUT_RGB              // Editing an RGB value

} ui_input_mode_t;

// RGB config widgets:
typedef enum {

    UI_RGB_BAR_RED,
    UI_RGB_NUM_RED,
    UI_RGB_BAR_GREEN,
    UI_RGB_NUM_GREEN,
    UI_RGB_BAR_BLUE,
    UI_RGB_NUM_BLUE
//    UI_RGB_HEX,

} ui_rgb_widgets_t;


/****************************************************************************
 * Constants and macros
 ***************************************************************************/

#define UI_MENU( name, num_items, ... ) \
    { name, num_items, (ui_menu_item_t[]) {__VA_ARGS__} }

#define UI_MENU_ITEM_DUMMY( name ) { UI_DUMMY, name }

#define UI_MENU_ITEM_FLAG( name, num, bitpos ) \
    { UI_FLAG, name, .number = num, .bit = bitpos }

#define UI_MENU_ITEM_LED_CONFIG( name, led_ch ) \
    { UI_LED_CONFIG, name, .led_channel = led_ch }

#define UI_MENU_ITEM_NUM_SELECTOR( name, num ) \
    { UI_NUM_SELECTOR, name, .number = num }

#define UI_MENU_ITEM_RGB_SELECTOR( name, led_ch ) \
    { UI_RGB_SELECTOR, name, .led_channel = led_ch }

#define UI_MENU_ITEM_SAVE( name ) { UI_SAVE, name }

#define UI_MENU_ITEM_SUBMENU( name, title, num_items, ... ) \
    { UI_SUBMENU, name, .submenu = UI_MENU( title, num_items, __VA_ARGS__ ) }

#define UI_MAX_MENU_DEPTH 16

#define UI_LOG_ROWS 8
#define UI_LOG_COLS 21
#define UI_LOG_NEW_ROW "                     "


/****************************************************************************
 * Externs
 ***************************************************************************/

extern bool ui_active;


/****************************************************************************
 * Prototypes
 ***************************************************************************/

bool ui_draw( u8g_t * );
void ui_enter( void );
void ui_handle_key( uint8_t, int, bool );
void ui_leave( void );
void ui_log_append_byte( uint8_t );
void ui_log_append_char( uint8_t );
void ui_log_append_str( char * );
void ui_log_newline( void );


/***************************************************************************/

#endif

/* vi: set et sts=4 sw=4 ts=4: */
