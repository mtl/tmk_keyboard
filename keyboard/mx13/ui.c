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
#include "led-local.h"
#include "pwm-driver.h"
#include "ui.h"


/***************************************************************************/

// Globals:
bool ui_active = false;

static ui_input_mode_t input_mode = UI_INPUT_MENU;
static ui_menu_t * menu_stack[ UI_MAX_MENU_DEPTH ] = { 0 };
static int menu_stack_pos = 0;

static ui_menu_t led_menu = UI_MENU( "", 3,
    UI_MENU_ITEM_RGB_SELECTOR( "Color - on", 0 ),
    UI_MENU_ITEM_RGB_SELECTOR( "Color - off", 0 ),
    UI_MENU_ITEM_DUMMY( "Master control" ),
);

static ui_menu_t menu = UI_MENU( "MX13 Config", 6,
    UI_MENU_ITEM_RGB_SELECTOR( "RGB Selector", LED_CAPS_LOCK_0 ),
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

static int ui_widget_focus_frame_pad = 1;

// Fonts:
static u8g_pgm_uint8_t const * ui_menu_title_font = u8g_font_profont12; 
static u8g_pgm_uint8_t const * ui_menu_list_font = u8g_font_profont12; 

// Colors:
static display_color_t ui_menu_title_color_bg = {{ 100, 100, 200 }};
static display_color_t ui_menu_title_color_fg = {{ 255, 255, 255 }};
static display_color_t ui_menu_list_color_bg = {{ 0, 0, 128 }};
static display_color_t ui_menu_list_color_fg = {{ 255, 255, 255 }};
static display_color_t ui_rgb_red = {{ 128, 0, 0 }};
static display_color_t ui_rgb_green = {{ 0, 128, 0 }};
static display_color_t ui_rgb_blue = {{ 0, 0, 64 }};

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

bool ui_draw( u8g_t * u8g_ref ) {

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
                break;

            case UI_INPUT_RGB:
                ui_draw_rgb_config();
                break;
        }

        return true;
    } else {
        display_draw_full_screen_bitmap( (u8g_pgm_uint8_t *) mx13_logo );
        return false;
    }
}


/***************************************************************************/

static display_color_t rgb_focus_color = {{ 255, 255, 0 }};
static bool rgb_focus_locked = false;
static pwm_rgb_led_t * rgb_led = NULL;
static uint16_t rgb_max_value = 0xfff;
static uint8_t rgb_prior_flags = 0;
static uint16_t rgb_widget_color[ 3 ] = { 0, 0, 0 };
static ui_rgb_widgets_t rgb_widget_focus = UI_RGB_BAR_RED;
static char * rgb_widget_title = NULL;

void ui_draw_rgb_config() {

    int frame_width = 1;

    // Render title bar and background:
    int y = ui_draw_page( rgb_widget_title );

    // Calculate list font dimensions:
    u8g_SetFont( u8g, ui_menu_list_font );
    ui_menu_list_font_height = u8g_GetFontAscent( u8g );
    ui_menu_list_font_vsize = ui_menu_list_font_height - u8g_GetFontDescent( u8g );

    // Compute first line y pos and line height:
    y += frame_width + ui_widget_focus_frame_pad + ui_menu_list_font_height + ui_menu_list_hpad - 1;
    int step = ( frame_width << 1 ) + ( ui_widget_focus_frame_pad << 1 ) + ui_menu_list_font_vsize - 1;

    // Compute x pos for numbers:
    int char_width = u8g_GetStrWidth( u8g, "w" );
    int num_chars = 3;
    if ( rgb_max_value > 255 ) {
        num_chars = 4;
    }
    int num_x = 128 - ui_menu_list_hpad - frame_width - num_chars * char_width;

    // Compute bar dimensions:
    int bar_frame_x = ui_menu_list_hpad;
    int bar_x = bar_frame_x + frame_width + ui_widget_focus_frame_pad;
    uint16_t max_bar_width = num_x - bar_x - ( ui_widget_focus_frame_pad << 1 ) - 1;
    uint32_t bar_width = 0;

    // Draw bars:
    display_set_draw_color( &ui_rgb_red );
    int y_red = y;
    bar_width = max_bar_width;
    bar_width *= rgb_widget_color[ 0 ];
    bar_width /= rgb_max_value;
    u8g_DrawBox(
        u8g,
        bar_x,
        y_red - ui_menu_list_font_height,
        bar_width,
        ui_menu_list_font_vsize
    );

    display_set_draw_color( &ui_rgb_green );
    int y_green = y_red + step;
    bar_width = max_bar_width;
    bar_width *= rgb_widget_color[ 1 ];
    bar_width /= rgb_max_value;
    u8g_DrawBox(
        u8g,
        bar_x,
        y_green - ui_menu_list_font_height,
        bar_width,
        ui_menu_list_font_vsize
    );

    display_set_draw_color( &ui_rgb_blue );
    int y_blue = y_green + step;
    bar_width = max_bar_width;
    bar_width *= rgb_widget_color[ 2 ];
    bar_width /= rgb_max_value;
    u8g_DrawBox(
        u8g,
        bar_x,
        y_blue - ui_menu_list_font_height,
        bar_width,
        ui_menu_list_font_vsize
    );

    // Draw numbers:
    display_set_draw_color( &ui_menu_title_color_fg );
    char digits[] = "   ";
    if ( rgb_max_value < 256 ) {
        digits[ 2 ] = 0;
    }
    int d = 0;
    if ( rgb_max_value > 255 ) {
        digits[ d++ ] = ui_log_nibchar( rgb_widget_color[ 0 ] >> 8 );
    }
    digits[ d++ ] = ui_log_nibchar( rgb_widget_color[ 0 ] >> 4 );
    digits[ d ] = ui_log_nibchar( rgb_widget_color[ 0 ] );
    u8g_DrawStr( u8g, num_x, y_red, digits );
    d = 0;
    if ( rgb_max_value > 255 ) {
        digits[ d++ ] = ui_log_nibchar( rgb_widget_color[ 1 ] >> 8 );
    }
    digits[ d++ ] = ui_log_nibchar( rgb_widget_color[ 1 ] >> 4 );
    digits[ d ] = ui_log_nibchar( rgb_widget_color[ 1 ] );
    u8g_DrawStr( u8g, num_x, y_green, digits );
    d = 0;
    if ( rgb_max_value > 255 ) {
        digits[ d++ ] = ui_log_nibchar( rgb_widget_color[ 2 ] >> 8 );
    }
    digits[ d++ ] = ui_log_nibchar( rgb_widget_color[ 2 ] >> 4 );
    digits[ d ] = ui_log_nibchar( rgb_widget_color[ 2 ] );
    u8g_DrawStr( u8g, num_x, y_blue, digits );

    // Draw focus:
    if ( rgb_focus_locked ) {
        display_set_draw_color( &rgb_focus_color );
    }
    int color = rgb_widget_focus >> 1;
    if ( ! color ) {
        y = y_red;
    } else if ( color == 1 ) {
        y = y_green;
    } else {
        y = y_blue;
    }
    if ( rgb_widget_focus % 2 ) {
        u8g_DrawFrame(
            u8g, num_x - ui_widget_focus_frame_pad - 1,
            y - ui_menu_list_font_height - ui_widget_focus_frame_pad - frame_width,
            128 - ui_menu_list_hpad - frame_width - ui_widget_focus_frame_pad - num_x + 4,
            ui_menu_list_font_vsize + ( ui_widget_focus_frame_pad << 1 ) + ( frame_width << 1 )
        );
    } else {
        u8g_DrawFrame(
            u8g, ui_menu_list_hpad,
            y - ui_menu_list_font_height - ui_widget_focus_frame_pad - frame_width,
            max_bar_width + ( ui_widget_focus_frame_pad << 1 ) + ( frame_width << 1 ),
            ui_menu_list_font_vsize + ( ui_widget_focus_frame_pad << 1 ) + ( frame_width << 1 )
        );
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

    if ( ui_active ) {
        return;
    }

    ui_active = true;

    menu_stack[ 0 ] = &menu;
    menu_stack_pos = 0;
    input_mode = UI_INPUT_MENU;

//    input_mode = UI_INPUT_LOG;

    display_draw( true );

#ifdef LED_CONTROLLER_ENABLE

    // Turn on LED:
    pwm_rgb_led_t * led = &leds[ LED_DISPLAY ];
    led->flags |= PWM_LED_FLAGS_ON;
    pwm_rgb_led_set_percent( led, PWM_RED, 0 );
    pwm_rgb_led_set_percent( led, PWM_GREEN, 1 );
    pwm_rgb_led_set_percent( led, PWM_BLUE, 0 );
    pwm_set_rgb_led( led );
    pwm_commit( true );
#endif
}


/***************************************************************************/

bool ui_enter_menu( ui_menu_t * menu, char * default_title ) {

    // Enforce menu stack limit:
    if ( menu_stack_pos >= UI_MAX_MENU_DEPTH - 1 ) {
        return false;
    }

    // Copy lable if needed:
    if ( menu->title == NULL ) {
        menu->title = default_title;
    }

    menu_stack[ ++menu_stack_pos ] = menu;
    display_draw( true );
    return true;
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

    nibble &= 0xf;
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
            display_draw( true );
        }
        return;
    }

    // Navigate to the root menu if requested:
    if ( item_no < 0 ) {
        menu_stack[ 0 ] = &menu;
        menu_stack_pos = 0;
        display_draw( true );
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
            ui_enter_menu( &item->submenu, item->label );
            break;

        case UI_CONFIRM:
        case UI_DUMMY:
        case UI_EDITOR:
        case UI_EXIT:
        case UI_LED_CONFIG:
            led_menu.title = item->label;
            ui_enter_menu( &led_menu, NULL );

        case UI_NAV_PREV:
            break;

        case UI_RGB_SELECTOR:
            
            rgb_led = &leds[ item->led_channel ];
            if ( rgb_led->flags & PWM_LED_FLAGS_TEENSY ) {
                rgb_max_value = 255;
            } else {
                rgb_max_value = 4095;
            }

            rgb_widget_color[ 0 ] = 0;
            rgb_widget_color[ 1 ] = 0;
            rgb_widget_color[ 2 ] = 0;
            rgb_focus_locked = false;

            // Configure the LED:
            rgb_prior_flags = rgb_led->flags;
            rgb_led->flags |= PWM_LED_FLAGS_ON | PWM_LED_FLAGS_ENABLED;
            for ( int i = 0; i < 6; i++ ) {
                rgb_led->values[ i ] = 0;
            }
            pwm_set_rgb_led( rgb_led );
            pwm_commit( true );

            rgb_widget_focus = UI_RGB_BAR_RED;
            rgb_widget_title = item->label;
            input_mode = UI_INPUT_RGB;
            display_draw( true );
            break;
    }
}


/***************************************************************************/

void ui_handle_key( uint8_t layer, int keycode, bool is_pressed ) {

    bool rgb_adjust = false;
    int rgb_bit = 0;
    bool rgb_inc = false;

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
            display_draw( false );
            break;

        case UI_INPUT_YES_NO:
        case UI_INPUT_EDIT:
            break;

        case UI_INPUT_RGB:
            if ( ! is_pressed ) {
                break;
            }

            switch ( keycode ) {
                case KC_ESC:
                    if ( rgb_focus_locked ) {
                        rgb_focus_locked = false;
                        display_draw( false );
                    } else {
                        input_mode = UI_INPUT_MENU;
                        rgb_led->flags = rgb_prior_flags;
                        pwm_set_rgb_led( rgb_led );
                        pwm_commit( true );
                        display_draw( true );
                    }
                    break;
                case KC_UP:
                    if ( ! rgb_focus_locked && rgb_widget_focus > 1 ) {
                        rgb_widget_focus -= 2;
                        display_draw( false );
                    }
                    break;
                case KC_DOWN:
                    if ( ! rgb_focus_locked && rgb_widget_focus < 4 ) {
                        rgb_widget_focus += 2;
                        display_draw( false );
                    }
                    break;
                case KC_LEFT:
                    if ( ! rgb_focus_locked && rgb_widget_focus % 2 ) {
                        rgb_widget_focus -= 1;
                        display_draw( false );
                    }
                    break;
                case KC_RIGHT:
                    if ( ! rgb_focus_locked && ! ( rgb_widget_focus % 2 ) ) {
                        rgb_widget_focus += 1;
                        display_draw( false );
                    }
                    break;
                case KC_ENTER:
                    rgb_focus_locked = ! rgb_focus_locked;
                    display_draw( false );
                    break;
                case KC_INSERT:
                    if ( rgb_max_value > 255 ) {
                        rgb_bit = 8;
                        rgb_inc = true;
                        rgb_adjust = true;
                    }
                    break;
                case KC_DELETE:
                    if ( rgb_max_value > 255 ) {
                        rgb_bit = 8;
                        rgb_inc = false;
                        rgb_adjust = true;
                    }
                    break;
                case KC_HOME:
                    rgb_bit = 4;
                    rgb_inc = true;
                    rgb_adjust = true;
                    break;
                case KC_END:
                    rgb_bit = 4;
                    rgb_inc = false;
                    rgb_adjust = true;
                    break;
                case KC_PGUP:
                    rgb_bit = 0;
                    rgb_inc = true;
                    rgb_adjust = true;
                    break;
                case KC_PGDN:
                    rgb_bit = 0;
                    rgb_inc = false;
                    rgb_adjust = true;
                    break;

            }
            if ( rgb_adjust ) {
                int color = rgb_widget_focus >> 1;
                uint8_t nibble = 0xf & ( rgb_widget_color[ color ] >> rgb_bit );
                if ( rgb_inc ) {
                    if ( nibble < 0xf ) {
                        rgb_widget_color[ color ] += 1 << rgb_bit;
                    } else {
                        rgb_adjust = false;
                    }
                } else {
                    if ( nibble > 0 ) {
                        rgb_widget_color[ color ] -= 1 << rgb_bit;
                    } else {
                        rgb_adjust = false;
                    }
                }
                if ( rgb_adjust ) {

                    int index = color << 1;
                    if ( rgb_led->flags & PWM_LED_FLAGS_TEENSY ) {
                        rgb_led->values[ index + 0 ] = rgb_widget_color[ color ];
                    } else {
                        rgb_led->values[ index + 0 ] = 0;
                        rgb_led->values[ index + 1 ] = rgb_widget_color[ color ];
                    }
                    pwm_set_rgb_led( rgb_led );
                    pwm_commit( true );
                    display_draw( false );
                }
            }
            break;
    }
}


/***************************************************************************/

void ui_leave() {

    if ( ! ui_active ) {
        return;
    }

    ui_active = false;

#ifdef LED_CONTROLLER_ENABLE

    // Turn off LED:
    pwm_rgb_led_t * led = &leds[ LED_DISPLAY ];
    led->flags &= ~PWM_LED_FLAGS_ON;
    pwm_set_rgb_led( led );
    pwm_commit( true );
#endif

    display_draw( true );
}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
