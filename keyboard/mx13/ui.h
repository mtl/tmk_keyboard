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
    UI_LED_CONFIG,      // LED configuration
    UI_NAV_PREV,        // Navigate to the previous screen
    UI_RGB_SELECTOR,    // Select an RGB value
    UI_SUBMENU,         // Navigate to a submenu
} ui_menu_item_type_t;

// A menu item:
typedef struct ui_menu_item {

    ui_menu_item_type_t type;
    char * label;
    //u8g_pgm_uint8_t label;

    union {
        led_indices_t led_channel;  // UI_LED_CONFIG
        ui_menu_t submenu;          // UI_SUBMENU
    };
} ui_menu_item_t;

// Input modes:
typedef enum {
    UI_INPUT_MENU,            // Navigating the menus
    UI_INPUT_YES_NO,          // Binary choice
    UI_INPUT_EDIT,            // Editing text
    UI_INPUT_LOG,             // Log
    UI_INPUT_RGB              // Editing an RGB value
} ui_input_mode_t;


/****************************************************************************
 * Constants and macros
 ***************************************************************************/

#define UI_MENU( name, num_items, ... ) \
    { name, num_items, (ui_menu_item_t[]) {__VA_ARGS__} }

#define UI_MENU_ITEM_DUMMY( name ) { UI_DUMMY, name }

#define UI_MENU_ITEM_LED_CONFIG( name, led_ch ) \
    { UI_LED_CONFIG, name, .led_channel = led_ch }

#define UI_MENU_ITEM_SUBMENU( name, title, num_items, ... ) \
    { UI_SUBMENU, name, .submenu = UI_MENU( title, num_items, __VA_ARGS__ ) }

#define UI_MAX_MENU_DEPTH 16

#define UI_LOG_ROWS 8
#define UI_LOG_COLS 21
#define UI_LOG_NEW_ROW "                     \0"


/****************************************************************************
 * Externs
 ***************************************************************************/


/****************************************************************************
 * Prototypes
 ***************************************************************************/

void ui_draw( u8g_t * );
void ui_draw_log( void );
void ui_draw_menu( ui_menu_t * );
int ui_draw_page( char * );
void ui_enter( void );
void ui_handle_key( uint8_t, int, bool );
void ui_leave( void );

void ui_log_append_byte( uint8_t );
void ui_log_append_char( uint8_t );
void ui_log_append_str( char * );
void ui_log_newline( void );
uint8_t ui_log_nibchar( uint8_t );

/***************************************************************************/

#endif

/* vi: set et sts=4 sw=4 ts=4: */
