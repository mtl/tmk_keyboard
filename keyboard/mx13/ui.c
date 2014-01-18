/****************************************************************************
 *
 *  User Interface
 *
 ***************************************************************************/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h> 
//#include "dt_logo.h"
#include "mx13_logo.h"
#include "display.h"
#include "keycode.h"
#include "ui.h"


/***************************************************************************/

// Globals:
static bool ui_active = false;

static ui_input_mode_t input_mode = UI_INPUT_MENU;
static ui_menu_t * menu_stack[ UI_MAX_MENU_DEPTH ] = { 0 };
static int menu_stack_pos = 0;

static ui_menu_t led_menu = UI_MENU( "", 3,
    UI_MENU_ITEM_DUMMY( "Color - on" ),
    UI_MENU_ITEM_DUMMY( "Color - off" ),
    UI_MENU_ITEM_DUMMY( "Master control" ),
);

static ui_menu_t menu = UI_MENU( "MX13 Config", 5,
    UI_MENU_ITEM_SUBMENU( "Lights", NULL, 5,
        UI_MENU_ITEM_SUBMENU( "Caps lock", NULL, 2,
            UI_MENU_ITEM_LED_CONFIG( "Panel", LED_CAPS_LOCK_0 ),
            UI_MENU_ITEM_LED_CONFIG( "In-key", LED_CAPS_LOCK_1 )
        ),
        UI_MENU_ITEM_SUBMENU( "Num lock", NULL, 2,
            UI_MENU_ITEM_LED_CONFIG( "Panel", LED_NUM_LOCK_0 ),
            UI_MENU_ITEM_LED_CONFIG( "In-key", LED_NUM_LOCK_1 )
        ),
        UI_MENU_ITEM_SUBMENU( "Scroll lock", NULL, 2,
            UI_MENU_ITEM_LED_CONFIG( "Panel", LED_SCROLL_LOCK_0 ),
            UI_MENU_ITEM_LED_CONFIG( "In-key", LED_SCROLL_LOCK_1 )
        ),
        UI_MENU_ITEM_LED_CONFIG( "TrackPoint", LED_TRACKPOINT ),
        UI_MENU_ITEM_LED_CONFIG( "Layer", LED_DISPLAY )
    ),
    UI_MENU_ITEM_SUBMENU( "TrackPoint", NULL, 3,
        UI_MENU_ITEM_DUMMY( "Acceleration" ),
        UI_MENU_ITEM_DUMMY( "Middle scroll" ),
        UI_MENU_ITEM_DUMMY( "Debug" )
    ),
    UI_MENU_ITEM_SUBMENU( "Keyboard", NULL, 2,
        UI_MENU_ITEM_SUBMENU( "Key repeat", NULL, 2,
            UI_MENU_ITEM_DUMMY( "Delay" ),
            UI_MENU_ITEM_DUMMY( "Rate" )
        ),
        UI_MENU_ITEM_DUMMY( "Debug" )
    ),
    UI_MENU_ITEM_SUBMENU( "Firmware", NULL, 3,
        UI_MENU_ITEM_DUMMY( "Background" ),
        UI_MENU_ITEM_DUMMY( "Bootloader" ),
        UI_MENU_ITEM_DUMMY( "Restart" )
    ),
    UI_MENU_ITEM_DUMMY( "Save..." )
);

static int ui_menu_title_font_vsize = 0;
static int ui_menu_title_font_height = 0;
static int ui_menu_title_gap = 4;
static int ui_menu_title_pad = 2;

static int ui_menu_list_font_vsize = 0;
static int ui_menu_list_font_height = 0;
static int ui_menu_list_hpad = 3;
static int ui_menu_list_vpad = 1;

// Fonts:
static u8g_pgm_uint8_t const * ui_menu_title_font = u8g_font_profont12; 
static u8g_pgm_uint8_t const * ui_menu_list_font = u8g_font_profont12; 

// Colors:
static display_color_t ui_menu_title_color_bg = {{ 100, 100, 200 }};
static display_color_t ui_menu_title_color_fg = {{ 255, 255, 255 }};
static display_color_t ui_menu_list_color_bg = {{ 0, 0, 128 }};
static display_color_t ui_menu_list_color_fg = {{ 255, 255, 255 }};

static u8g_t * u8g = NULL;

// Log screen stuff:
static uint8_t log[ UI_LOG_ROWS ][ UI_LOG_COLS + 1 ] = {
    UI_LOG_NEW_ROW,
    UI_LOG_NEW_ROW,
    UI_LOG_NEW_ROW,
    UI_LOG_NEW_ROW,
    UI_LOG_NEW_ROW,
    UI_LOG_NEW_ROW,
    UI_LOG_NEW_ROW,
    UI_LOG_NEW_ROW
};
static int log_cursor_row = 0;
static int log_cursor_column = 0;

/***************************************************************************/


/***************************************************************************/

void ui_draw( u8g_t * u8g_ref ) {

    u8g = u8g_ref;

    if ( ui_active ) {
        switch ( input_mode ) {
            case UI_INPUT_MENU:
                ui_draw_menu( menu_stack[ menu_stack_pos ] );
                break;

            case UI_INPUT_LOG:
                ui_draw_log();
                break;

            case UI_INPUT_YES_NO:
            case UI_INPUT_EDIT:
            case UI_INPUT_RGB:
                break;
        }
    } else {
        display_draw_bitmap( 0, 0, 128, 124, (u8g_pgm_uint8_t *) mx13_logo );
    }
}


/***************************************************************************/

void ui_draw_log() {

    // Render title bar and background:
    int y = ui_draw_page( "Log" );

    // Calculate list font dimensions:
    u8g_SetFont( u8g, ui_menu_list_font );
    ui_menu_list_font_height = u8g_GetFontAscent( u8g );
    ui_menu_list_font_vsize = ui_menu_list_font_height - u8g_GetFontDescent( u8g );

    // Draw list:
    display_set_draw_color( &ui_menu_list_color_fg );
    y += ui_menu_list_vpad + ui_menu_list_font_height + 1;
    int step = ui_menu_list_font_vsize + ( ui_menu_list_vpad << 1 ) + 1;
    for ( int i = 0; i < UI_LOG_ROWS; i++ ) {

        // Draw item label:
        u8g_DrawStr( u8g, ui_menu_list_hpad, y, &log[ i ] );
        y += step;
    }
}


/***************************************************************************/

void ui_draw_menu( ui_menu_t * menu ) {

    // Render title bar and background:
    int y = ui_draw_page( menu->title );

    // Calculate list font dimensions:
    u8g_SetFont( u8g, ui_menu_list_font );
    ui_menu_list_font_height = u8g_GetFontAscent( u8g );
    ui_menu_list_font_vsize = ui_menu_list_font_height - u8g_GetFontDescent( u8g );

    // Draw list:
    display_set_draw_color( &ui_menu_list_color_fg );
    y += ui_menu_list_vpad + ui_menu_list_font_height + 1;
    int step = ui_menu_list_font_vsize + ( ui_menu_list_vpad << 1 ) + 1;
    int indent = ui_menu_list_hpad + u8g_GetStrWidth( u8g, "2. " );
    ui_menu_item_t * items = menu->items;
    for ( int i = 0; i < menu->num_items; i++ ) {

        // Check if y descends beyond bottom of display:
        if ( y > 127 ) {
            break;
        }

        // Draw item number:
        char number[] = "n. ";
        number[ 0 ] = 0x31 + i;
        u8g_DrawStr( u8g, ui_menu_list_hpad, y, number );

        // Draw item label:
        u8g_DrawStr( u8g, indent, y, items[ i ].label );
        y += step;
    }
}


/***************************************************************************/

int ui_draw_page( char * title ) {

    // When we query font dimensions, base them on the largest extent of all
    // the glyphs in the font:
    u8g_SetFontRefHeightAll( u8g );

    // Calculate title bar font dimensions:
    u8g_SetFont( u8g, ui_menu_title_font );
    ui_menu_title_font_height = u8g_GetFontAscent( u8g );
    ui_menu_title_font_vsize = ui_menu_title_font_height - u8g_GetFontDescent( u8g );

    // Draw title bar:
    display_set_draw_color( &ui_menu_title_color_bg );
    int title_bar_height = ui_menu_title_font_vsize + ( ui_menu_title_pad << 1 );
    u8g_DrawBox( u8g, 0, 0, 128, title_bar_height );
    display_set_draw_color( &ui_menu_title_color_fg );
    u8g_DrawStr(
        u8g,
        ui_menu_title_pad + 1,
        ui_menu_title_font_height + ui_menu_title_pad,
        title
    );

    // Draw list background:
    display_set_draw_color( &ui_menu_list_color_bg );
    int y = title_bar_height + ui_menu_title_gap;
    u8g_DrawBox( u8g, 0, y, 128, 128 - y );

    return y;
}


/***************************************************************************/


void ui_enter() {
    ui_active = true;

    menu_stack[ 0 ] = &menu;
    menu_stack_pos = 0;
    input_mode = UI_INPUT_MENU;

//    input_mode = UI_INPUT_LOG;

    display_draw();
}


/***************************************************************************/

void ui_log_append_byte( uint8_t c ) {

    ui_log_append_char( ui_log_nibchar( c >> 4 ) );
    ui_log_append_char( ui_log_nibchar( c & 0xf ) );
}


/***************************************************************************/

void ui_log_append_char( uint8_t c ) {

    int char_width = u8g_GetStrWidth( u8g, "w" );
    int max_cols = ( 128 - ui_menu_list_hpad ) / char_width;

    if (
        log_cursor_column > UI_LOG_COLS - 1 ||
        log_cursor_column > max_cols
    ) {
        ui_log_newline();
    }

    log[ log_cursor_row ][ log_cursor_column ] = c;
    log_cursor_column++;
}


/***************************************************************************/

void ui_log_append_str( char * str ) {

    if ( str == NULL ) {
        return;
    }

    int i = 0;
    while ( 1 ) {
        char c = str[ i++ ];
        if ( ! c ) {
            return;
        }
        switch ( c ) {
            case '\n':
                ui_log_newline();
                break;
            default:
                ui_log_append_char( c );
        }
    }
}


/***************************************************************************/

void ui_log_newline() {

    if ( log_cursor_row < UI_LOG_ROWS - 1 ) {
        log_cursor_row++;
    } else {
        for ( int i = 1; i < UI_LOG_ROWS; i++ ) {
            for ( int j = 0; j < UI_LOG_COLS + 1; j++ ) {
                log[ i - 1 ][ j ] = log[ i ][ j ];
            }
        }

        uint8_t new_row[] = UI_LOG_NEW_ROW;
        for ( int j = 0; j < UI_LOG_COLS + 1; j++ ) {
            log[ UI_LOG_ROWS - 1 ][ j ] = new_row[ j ];
        }

        //log[ UI_LOG_ROWS - 1 ] = UI_LOG_NEW_ROW;
    }
    log_cursor_column = 0;
}


/***************************************************************************/

uint8_t ui_log_nibchar( uint8_t nibble ) {
    if ( nibble <= 9 ) {
        return 0x30 + nibble;
    } else {
//        return 0x41 - 10 + nibble;
        return 0x61 - 10 + nibble;
    }
}


/***************************************************************************/


void ui_menu_select( int item_no ) {

    // If 0, navigate back/up:
    if ( ! item_no ) {
        if ( menu_stack_pos ) {
            menu_stack_pos--;
            display_draw();
        }
        return;
    }

    // Navigate to the root menu if requested:
    if ( item_no < 0 ) {
        menu_stack[ 0 ] = &menu;
        menu_stack_pos = 0;
        display_draw();
        return;
    }

    // Get current menu:
    ui_menu_t * current_menu = menu_stack[ menu_stack_pos ];

    // Ignore invalid item numbers:
    if ( item_no > current_menu->num_items ) {
        return;
    }

    ui_menu_item_t * item = &current_menu->items[ item_no - 1 ];
    switch ( item->type ) {

        case UI_SUBMENU:
            // Enforce menu stack limit:
            if ( menu_stack_pos >= UI_MAX_MENU_DEPTH - 1 ) {
                return;
            }

            // Copy lable if needed:
            if ( item->submenu.title == NULL ) {
                item->submenu.title = item->label;
            }

            menu_stack[ ++menu_stack_pos ] = &item->submenu;
            display_draw();
            break;

        case UI_CONFIRM:
        case UI_DUMMY:
        case UI_EDITOR:
        case UI_EXIT:
        case UI_LED_CONFIG:
        case UI_NAV_PREV:
        case UI_RGB_SELECTOR:
            break;
    }
}


/***************************************************************************/

void ui_handle_key( uint8_t layer, int keycode, bool is_pressed ) {

    switch ( input_mode ) {

        case UI_INPUT_MENU:
            if ( ! is_pressed ) {
                break;
            }

            switch ( keycode ) {
                case KC_0:
                case KC_ESC:
                case KC_PGUP:
                    ui_menu_select( 0 );
                    break;
                case KC_1:
                case KC_2:
                case KC_3:
                case KC_4:
                case KC_5:
                case KC_6:
                case KC_7:
                case KC_8:
                case KC_9:
                    ui_menu_select( keycode - KC_1 + 1 );
                    break;
                case KC_HOME:
                    ui_menu_select( -1 );
                    break;
            }
            break;

        case UI_INPUT_LOG:
            ui_log_append_str( "Key:[" );
            ui_log_append_byte( layer );
            ui_log_append_str( "," );
            ui_log_append_byte( keycode );
            ui_log_append_str( "," );
            ui_log_append_byte( is_pressed );
            ui_log_append_str( "]\n" );
            display_draw();
            break;

        case UI_INPUT_YES_NO:
        case UI_INPUT_EDIT:
        case UI_INPUT_RGB:
            break;
    }
}


/***************************************************************************/

void ui_leave() {
    ui_active = false;
    display_draw();
}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
