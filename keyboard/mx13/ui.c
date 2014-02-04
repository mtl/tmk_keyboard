/****************************************************************************
 *
 *  User Interface
 *
 ***************************************************************************/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h> 
#include "dt_logo.h"
#include "mx13_logo.h"
#include "mx13_logo_commando.h"
#include "display.h"
#include "keycode.h"
#include "led-local.h"
#include "pwm-driver.h"
#include "settings.h"
#include "trackpoint.h"
#include "ui.h"

#ifdef TRACKPOINT_ENABLE
#    include "trackpoint.h"
#endif

extern ui_menu_t ui_led_menu;
extern ui_menu_t ui_menu;

/***************************************************************************/
// Static prototypes:

static void calculate_dimensions( void );
static void draw_log( void );
static void draw_menu( ui_menu_t * );
static void draw_num_selector( void );
static void draw_page( char * );
static void draw_rgb_config( void );
static bool enter_menu( ui_menu_t *, char * );
static bool get_flag_state( ui_number_t, uint8_t );
static void handle_key_num( uint8_t, int, bool );
static void handle_key_rgb( uint8_t, int, bool );
static void initialize( u8g_t * );
#ifdef TRACKPOINT_ENABLE
static uint8_t map_number_to_tp_ram( ui_number_t );
#endif
static uint8_t nibchar( uint8_t );
static void set_flag_state( ui_number_t, uint8_t, bool );
static void set_indicator( bool, bool );
static void start_num_selector( ui_menu_t *, ui_menu_item_t * );
static void start_rgb_selector( ui_menu_t *, ui_menu_item_t * );


/***************************************************************************/
// Globals:



static int num_font_vsize = 0;
static int num_font_height = 0;
static int num_vpad = 10;

static u8g_pgm_uint8_t const * num_selector_num_font = u8g_font_fub25n; 

static uint16_t num_widget_max = 9999;
static uint16_t num_widget_min = 0;
static ui_number_t num_widget_number = 0;
static uint16_t num_widget_value = 22222;
static char * num_widget_title = "Num selector";








bool ui_active = false;

static ui_input_mode_t input_mode = UI_INPUT_MENU;
static ui_menu_t * menu_stack[ UI_MAX_MENU_DEPTH ] = { 0 };
static int menu_stack_pos = 0;

static int page_body_start = 0;

static int menu_title_font_vsize = 0;
static int menu_title_font_height = 0;
static int menu_title_gap = 4;
static int menu_title_pad = 2;

static int menu_list_font_vsize = 0;
static int menu_list_font_height = 0;
static int menu_list_hpad = 3;
static int menu_list_vpad = 1;

static int widget_focus_frame_pad = 1;

// Fonts:
static u8g_pgm_uint8_t const * menu_title_font = u8g_font_profont12; 
static u8g_pgm_uint8_t const * menu_list_font = u8g_font_profont12; 

// Colors:
static display_color_t menu_title_color_bg = {{ 100, 100, 200 }};
static display_color_t menu_title_color_fg = {{ 255, 255, 255 }};
static display_color_t menu_list_color_bg = {{ 0, 0, 128 }};
static display_color_t menu_list_color_fg = {{ 255, 255, 255 }};
static display_color_t menu_list_disabled_color_fg = {{ 0, 0, 64 }};
static display_color_t rgb_red = {{ 128, 0, 0 }};
static display_color_t rgb_green = {{ 0, 128, 0 }};
static display_color_t rgb_blue = {{ 0, 0, 64 }};

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

static void calculate_dimensions() {

    // When we query font dimensions, base them on the largest extent of all
    // the glyphs in the font:
    u8g_SetFontRefHeightAll( u8g );

    u8g_SetFont( u8g, num_selector_num_font );
    num_font_height = u8g_GetFontAscent( u8g );
    num_font_vsize = num_font_height - u8g_GetFontDescent( u8g );

    u8g_SetFont( u8g, menu_title_font );
    menu_title_font_height = u8g_GetFontAscent( u8g );
    menu_title_font_vsize = menu_title_font_height - u8g_GetFontDescent( u8g );

    u8g_SetFont( u8g, menu_list_font );
    menu_list_font_height = u8g_GetFontAscent( u8g );
    menu_list_font_vsize = menu_list_font_height - u8g_GetFontDescent( u8g );

}


/***************************************************************************/

static void draw_log() {

    // Render title bar and background:
    draw_page( "Log" );

    // Draw list:
    u8g_SetFont( u8g, menu_list_font );
    display_set_draw_color( &menu_list_color_fg );
    int y = page_body_start + menu_list_vpad + menu_list_font_height + 1;
    int step = menu_list_font_vsize + ( menu_list_vpad << 1 ) + 1;
    for ( int i = 0; i < UI_LOG_ROWS; i++ ) {

        // Draw item label:
        u8g_DrawStr( u8g, menu_list_hpad, y, (const char *) &log[ i ] );
        y += step;
    }
}


/***************************************************************************/

static void draw_menu( ui_menu_t * menu ) {

    // Render title bar and background:
    draw_page( menu->title );

    // Draw list:
    u8g_SetFont( u8g, menu_list_font );
    int y = page_body_start + menu_list_vpad + menu_list_font_height + 1;
    int step = menu_list_font_vsize + ( menu_list_vpad << 1 ) + 1;
    int indent = menu_list_hpad + u8g_GetStrWidth( u8g, "2. " );
    ui_menu_item_t * items = menu->items;
    for ( int i = 0; i < menu->num_items; i++ ) {

        // Check if y descends beyond bottom of display:
        if ( y > 127 ) {
            break;
        }

        // Set item color:
        ui_menu_item_t * item = &items[ i ];
        switch ( item->type ) {

            case UI_DUMMY:
                display_set_draw_color( &menu_list_disabled_color_fg );
                break;

            case UI_FLAG:
                if ( ! get_flag_state( item->number, item->bit ) ) {
                    display_set_draw_color( &menu_list_disabled_color_fg );
                    break;
                }

            default:
                display_set_draw_color( &menu_list_color_fg );
        }

        // Draw item number:
        char number[] = "n. ";
        number[ 0 ] = 0x31 + i;
        u8g_DrawStr( u8g, menu_list_hpad, y, number );

        // Draw item label:
        u8g_DrawStr( u8g, indent, y, item->label );
        y += step;
    }
}


/***************************************************************************/

static void draw_num_selector() {

    char number[ 6 ] = "     ";
    char * num;

    // Render title bar and background:
    draw_page( num_widget_title );

    // Compute first line y pos and line height:
    int y = page_body_start + num_font_height + num_vpad - 1;
    int step = ( menu_list_vpad << 1 ) + num_font_vsize - 1;

    // Create string from value:
    int val = num_widget_value;
    int num_digits = 0;
    num = &number[ 0 ] + 5;
    for ( int i = 0; i < 5; i++ ) {

        num--;
        *num = 0x30 + val % 10;
        num_digits++;
        val /= 10;

        if ( ! val ) break;
    }
    if ( ! num_digits ) {
        *num = '0';
    }

    // Draw current value:
    display_set_draw_color( &menu_list_color_fg );
    u8g_SetFont( u8g, num_selector_num_font );
    u8g_DrawStr(
        u8g,
        // Make sure this is positive if using a very wide font:
        ( 128 - u8g_GetStrWidth( u8g, num ) ) / 2,
        y,
        num
    );
}


/***************************************************************************/

static void draw_page( char * title ) {

    // Draw title bar:
    display_set_draw_color( &menu_title_color_bg );
    int title_bar_height = menu_title_font_vsize + ( menu_title_pad << 1 );
    u8g_DrawBox( u8g, 0, 0, 128, title_bar_height );
    display_set_draw_color( &menu_title_color_fg );
    u8g_SetFont( u8g, menu_title_font );
    u8g_DrawStr(
        u8g,
        menu_title_pad + 1,
        menu_title_font_height + menu_title_pad,
        title
    );

    // Draw list background:
    display_set_draw_color( &menu_list_color_bg );
    page_body_start = title_bar_height + menu_title_gap;
    u8g_DrawBox( u8g, 0, page_body_start, 128, 128 - page_body_start );

    return;
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

static void draw_rgb_config() {

    int frame_width = 1;

    // Render title bar and background:
    draw_page( rgb_widget_title );

    // Compute first line y pos and line height:
    int y = (
        page_body_start + frame_width + widget_focus_frame_pad +
        menu_list_font_height + menu_list_hpad - 1
    );
    int step = ( frame_width << 1 ) + ( widget_focus_frame_pad << 1 ) + menu_list_font_vsize - 1;

    // Compute x pos for numbers:
    u8g_SetFont( u8g, menu_list_font );
    int char_width = u8g_GetStrWidth( u8g, "w" );
    int num_chars = 3;
    if ( rgb_max_value > 255 ) {
        num_chars = 4;
    }
    int num_x = 128 - menu_list_hpad - frame_width - num_chars * char_width;

    // Compute bar dimensions:
    int bar_frame_x = menu_list_hpad;
    int bar_x = bar_frame_x + frame_width + widget_focus_frame_pad;
    uint16_t max_bar_width = num_x - bar_x - ( widget_focus_frame_pad << 1 ) - 1;
    uint32_t bar_width = 0;

    // Draw bars:
    display_set_draw_color( &rgb_red );
    int y_red = y;
    bar_width = max_bar_width;
    bar_width *= rgb_widget_color[ 0 ];
    bar_width /= rgb_max_value;
    u8g_DrawBox(
        u8g,
        bar_x,
        y_red - menu_list_font_height,
        bar_width,
        menu_list_font_vsize
    );

    display_set_draw_color( &rgb_green );
    int y_green = y_red + step;
    bar_width = max_bar_width;
    bar_width *= rgb_widget_color[ 1 ];
    bar_width /= rgb_max_value;
    u8g_DrawBox(
        u8g,
        bar_x,
        y_green - menu_list_font_height,
        bar_width,
        menu_list_font_vsize
    );

    display_set_draw_color( &rgb_blue );
    int y_blue = y_green + step;
    bar_width = max_bar_width;
    bar_width *= rgb_widget_color[ 2 ];
    bar_width /= rgb_max_value;
    u8g_DrawBox(
        u8g,
        bar_x,
        y_blue - menu_list_font_height,
        bar_width,
        menu_list_font_vsize
    );

    // Draw numbers:
    display_set_draw_color( &menu_title_color_fg );
    char digits[] = "   ";
    if ( rgb_max_value < 256 ) {
        digits[ 2 ] = 0;
    }
    int d = 0;
    if ( rgb_max_value > 255 ) {
        digits[ d++ ] = nibchar( rgb_widget_color[ 0 ] >> 8 );
    }
    digits[ d++ ] = nibchar( rgb_widget_color[ 0 ] >> 4 );
    digits[ d ] = nibchar( rgb_widget_color[ 0 ] );
    u8g_DrawStr( u8g, num_x, y_red, digits );
    d = 0;
    if ( rgb_max_value > 255 ) {
        digits[ d++ ] = nibchar( rgb_widget_color[ 1 ] >> 8 );
    }
    digits[ d++ ] = nibchar( rgb_widget_color[ 1 ] >> 4 );
    digits[ d ] = nibchar( rgb_widget_color[ 1 ] );
    u8g_DrawStr( u8g, num_x, y_green, digits );
    d = 0;
    if ( rgb_max_value > 255 ) {
        digits[ d++ ] = nibchar( rgb_widget_color[ 2 ] >> 8 );
    }
    digits[ d++ ] = nibchar( rgb_widget_color[ 2 ] >> 4 );
    digits[ d ] = nibchar( rgb_widget_color[ 2 ] );
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
            u8g, num_x - widget_focus_frame_pad - 1,
            y - menu_list_font_height - widget_focus_frame_pad - frame_width,
            128 - menu_list_hpad - frame_width - widget_focus_frame_pad - num_x + 4,
            menu_list_font_vsize + ( widget_focus_frame_pad << 1 ) + ( frame_width << 1 )
        );
    } else {
        u8g_DrawFrame(
            u8g, menu_list_hpad,
            y - menu_list_font_height - widget_focus_frame_pad - frame_width,
            max_bar_width + ( widget_focus_frame_pad << 1 ) + ( frame_width << 1 ),
            menu_list_font_vsize + ( widget_focus_frame_pad << 1 ) + ( frame_width << 1 )
        );
    }
}


/***************************************************************************/

static bool enter_menu( ui_menu_t * menu, char * default_title ) {

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

static bool get_flag_state( ui_number_t number, uint8_t bit ) {

#ifdef TRACKPOINT_ENABLE
    // Handle TrackPoint bits:
    uint8_t tp_ram_location = map_number_to_tp_ram( number );
    if ( tp_ram_location != 255 ) {

        bool state = false;
        tp_status_t result = tp_ram_bit_get( tp_ram_location, bit, &state );
        if ( result != TP_OK ) {
            return false;
        }
        return state;
    }
#endif

    return false;
}


/***************************************************************************/

static void set_flag_state( ui_number_t number, uint8_t bit, bool state ) {

#ifdef TRACKPOINT_ENABLE
    // Handle TrackPoint bits:
    uint8_t tp_ram_location = map_number_to_tp_ram( number );
    if ( tp_ram_location != 255 ) {

        if ( state ) {
            tp_ram_bit_set( tp_ram_location, bit );
        } else {
            tp_ram_bit_clear( tp_ram_location, bit );
        }
        return;
    }
#endif

}


/***************************************************************************/

static void handle_key_num( uint8_t layer, int keycode, bool is_pressed ) {

    if ( ! is_pressed ) {
        return;
    }

    int number = -1;
    int new_value = 0;
#ifdef TRACKPOINT_ENABLE
    uint8_t tp_ram_location = 0;
#endif

    switch ( keycode ) {

        case KC_ESC:
            input_mode = UI_INPUT_MENU;
            display_draw( true );
            break;

        case KC_ENTER:

#ifdef TRACKPOINT_ENABLE
            tp_ram_location = map_number_to_tp_ram( num_widget_number );
            if ( tp_ram_location != 255 ) {
                tp_ram_write( tp_ram_location, num_widget_value );

                // Update sesitivity variables:
                if ( tp_ram_location == TP_RAM_SNSTVTY ) {
                    tp_normal_sensitivity = tp_sensitivity = num_widget_value;
                }
            }

            else {
#endif
                switch ( num_widget_number ) {

                    case UI_NUM_LED_TP_INTENSITY:
                        led_config.trackpoint.intensity = num_widget_value ;
                        led_set_trackpoint( led_config.trackpoint.on );
                        break;

                    case UI_NUM_TP_SCROLL_H:
                        tp_scroll_divisor_h = num_widget_value;
                        break;

                    case UI_NUM_TP_SCROLL_V:
                        tp_scroll_divisor_v = num_widget_value;
                        break;

                    default:
                        break;
                }
#ifdef TRACKPOINT_ENABLE
            }
#endif

            input_mode = UI_INPUT_MENU;
            display_draw( true );
            break;

        case KC_BSPACE:
            new_value = num_widget_value / 10;
            if ( new_value < num_widget_min ) {
                new_value = num_widget_min;
            }
            num_widget_value = new_value;
            display_draw( false );
            break;

        case KC_0: number = 0; break;
        case KC_1: number = 1; break;
        case KC_2: number = 2; break;
        case KC_3: number = 3; break;
        case KC_4: number = 4; break;
        case KC_5: number = 5; break;
        case KC_6: number = 6; break;
        case KC_7: number = 7; break;
        case KC_8: number = 8; break;
        case KC_9: number = 9; break;

    }

    if ( number != -1 ) {

        new_value = num_widget_value * 10 + number;
        if ( new_value <= num_widget_max ) {
            num_widget_value = new_value;
        }
        display_draw( false );
    }
}


/***************************************************************************/

static void handle_key_rgb( uint8_t layer, int keycode, bool is_pressed ) {

    bool rgb_adjust = false;
    int rgb_bit = 0;
    bool rgb_inc = false;

    if ( ! is_pressed ) {
        return;
    }

    switch ( keycode ) {

        case KC_ESC:
            if ( rgb_focus_locked ) {
                rgb_focus_locked = false;
                display_draw( false );
            } else {
                input_mode = UI_INPUT_MENU;

                rgb_led->flags = rgb_prior_flags;
                if ( rgb_led->flags & PWM_LED_FLAGS_TEENSY ) {
                    led_set_teensy_led( rgb_led );
                } else {
                    pwm_set_rgb_led( rgb_led );
                    pwm_commit( true );
                }

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
            rgb_bit = rgb_max_value > 255 ? 8 : 4;
            rgb_inc = true;
            rgb_adjust = true;
            break;

        case KC_DELETE:
            rgb_bit = rgb_max_value > 255 ? 8 : 4;
            rgb_inc = false;
            rgb_adjust = true;
            break;

        case KC_HOME:
            rgb_bit = rgb_max_value > 255 ? 4 : 0;
            rgb_inc = true;
            rgb_adjust = true;
            break;

        case KC_END:
            rgb_bit = rgb_max_value > 255 ? 4 : 0;
            rgb_inc = false;
            rgb_adjust = true;
            break;

        case KC_PGUP:
            if ( rgb_max_value > 255 ) {
                rgb_bit = 0;
                rgb_inc = true;
                rgb_adjust = true;
            }
            break;

        case KC_PGDN:
            if ( rgb_max_value > 255 ) {
                rgb_bit = 0;
                rgb_inc = false;
                rgb_adjust = true;
            }
            break;

        case KC_0:
            rgb_widget_color[ rgb_widget_focus >> 1 ] = 1;
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
                led_set_teensy_led( rgb_led );
            } else {
                rgb_led->values[ index + 0 ] = 0;
                rgb_led->values[ index + 1 ] = rgb_widget_color[ color ];
                pwm_set_rgb_led( rgb_led );
                pwm_commit( true );
            }

            display_draw( false );
        }
    }
}


/***************************************************************************/

static void initialize( u8g_t * u8g_ref ) {

    static bool initialized = false;

    if ( ! initialized ) {
        u8g = u8g_ref;
        calculate_dimensions();
        initialized = true;
    }
}


/***************************************************************************/

#ifdef TRACKPOINT_ENABLE
static uint8_t map_number_to_tp_ram( ui_number_t number ) {

    if (
        (
            number > UI_NUM_TP_RAM_BYTE_START &&
            number < UI_NUM_TP_RAM_BYTE_END
        ) ||
        (
            number > UI_NUM_TP_RAM_BIT_START &&
            number < UI_NUM_TP_RAM_BIT_END
        )
    ) {
        switch ( number ) {

            case UI_NUM_TP_CONFIG: return TP_RAM_CONFIG;
            case UI_NUM_TP_DRAGHYS: return TP_RAM_DRAGHYS;
            case UI_NUM_TP_DRIFT: return TP_RAM_DRIFT;
            case UI_NUM_TP_INERTIA: return TP_RAM_INERTIA;
            case UI_NUM_TP_JKCUR: return TP_RAM_JKCUR;
            case UI_NUM_TP_MINDRAG: return TP_RAM_MINDRAG;
            case UI_NUM_TP_PDRIFTLIM: return TP_RAM_PDRIFTLIM;
            case UI_NUM_TP_PDRIFT_REL: return TP_RAM_PDRIFT_REL;
            case UI_NUM_TP_REACH: return TP_RAM_REACH;
            case UI_NUM_TP_REG23: return TP_RAM_REG23;
            case UI_NUM_TP_REG2D: return TP_RAM_REG2D;
            case UI_NUM_TP_RSTDFT1: return TP_RAM_RSTDFT1;
            case UI_NUM_TP_SNSTVTY: return TP_RAM_SNSTVTY;
            case UI_NUM_TP_THR: return TP_RAM_THR;
            case UI_NUM_TP_UTHR: return TP_RAM_UTHR;
            case UI_NUM_TP_VALUE6: return TP_RAM_VALUE6;
            case UI_NUM_TP_XYAVG_FACTOR: return TP_RAM_XYAVG_FACTOR;
            case UI_NUM_TP_XYDRIFTAVG: return TP_RAM_XYDRIFTAVG;
            case UI_NUM_TP_ZTC: return TP_RAM_ZTC;
            default: break;
        }
    }

    return 255;
}
#endif


/***************************************************************************/

static uint8_t nibchar( uint8_t nibble ) {

    nibble &= 0xf;
    if ( nibble <= 9 ) {
        return 0x30 + nibble;
    } else {
//        return 0x41 - 10 + nibble;
        return 0x61 - 10 + nibble;
    }
}


/***************************************************************************/

static void set_indicator( bool is_active, bool update ) {

#ifdef LED_CONTROLLER_ENABLE

    static bool was_active = false;
    static uint16_t values[ 6 ];
    static uint8_t prior_flags = 0;
    pwm_rgb_led_t * led = &led_config.leds[ LED_DISPLAY ];

    // Save LED state:
    if ( is_active && ! was_active ) {

        prior_flags = led->flags;
        for ( int i = 0; i < 6; i++ ) {
            values[ i ] = led->values[ i ];
        }

        // Turn on LED:
        led->flags |= PWM_LED_FLAGS_ON;
        pwm_rgb_led_set_percent( led, PWM_RED, 0 );
        pwm_rgb_led_set_percent( led, PWM_GREEN, 1 );
        pwm_rgb_led_set_percent( led, PWM_BLUE, 0 );

        if ( update ) {
            pwm_set_rgb_led( led );
            pwm_commit( true );
        }

        was_active = true;
    }

    // Restore LED state:
    else if ( ! is_active && was_active ) {

        // Restore LED state:
        led->flags = prior_flags;
        for ( int i = 0; i < 6; i++ ) {
            led->values[ i ] = values[ i ];
        }

        if ( update ) {
            pwm_set_rgb_led( led );
            pwm_commit( true );
        }

        was_active = false;
    }

#endif
}


/***************************************************************************/

static void start_num_selector(
    ui_menu_t * current_menu, ui_menu_item_t * item
) {

    tp_status_t status;

    num_widget_max = 255;
    num_widget_min = 0;
    num_widget_number = item->number;
    num_widget_title = item->label;
    uint8_t value = 0;

#ifdef TRACKPOINT_ENABLE
    uint8_t tp_ram_location = map_number_to_tp_ram( num_widget_number );
    if ( tp_ram_location != 255 ) {

        status = tp_ram_read( tp_ram_location, &value );
        if ( status != TP_OK ) {
            value = 0;
        }
        num_widget_value = value;
    }

    else {
#endif
        switch ( num_widget_number ) {

            case UI_NUM_LED_TP_INTENSITY:
                num_widget_value = led_config.trackpoint.intensity;
                led_set_trackpoint( true );
                break;

            case UI_NUM_TP_SCROLL_H:
                num_widget_value = tp_scroll_divisor_h;
                num_widget_max = 16256;
                break;

            case UI_NUM_TP_SCROLL_V:
                num_widget_value = tp_scroll_divisor_v;
                num_widget_max = 16256;
                break;

            default:
                break;
        }
#ifdef TRACKPOINT_ENABLE
    }
#endif

    input_mode = UI_INPUT_NUM;
    display_draw( true );
}


/***************************************************************************/

static void start_rgb_selector(
    ui_menu_t * current_menu, ui_menu_item_t * item
) {

    // Get the LED parameters:
    rgb_led = &led_config.leds[ item->led_channel ];

    // Start with current LED color:
    if ( rgb_led->flags & PWM_LED_FLAGS_TEENSY ) {
        rgb_widget_color[ 0 ] = rgb_led->values[ PWM_RED ];
        rgb_widget_color[ 1 ] = rgb_led->values[ PWM_GREEN ];
        rgb_widget_color[ 2 ] = rgb_led->values[ PWM_BLUE ];
    } else {
        rgb_widget_color[ 0 ] = rgb_led->values[ PWM_RED + 1 ];
        rgb_widget_color[ 1 ] = rgb_led->values[ PWM_GREEN + 1 ];
        rgb_widget_color[ 2 ] = rgb_led->values[ PWM_BLUE + 1 ];
    }

    // Light up the LED now:
    rgb_prior_flags = rgb_led->flags;
    rgb_led->flags |= PWM_LED_FLAGS_ON | PWM_LED_FLAGS_ENABLED;
    if ( rgb_led->flags & PWM_LED_FLAGS_TEENSY ) {
        led_set_teensy_led( rgb_led );
        rgb_max_value = 255;
    } else {
        pwm_set_rgb_led( rgb_led );
        pwm_commit( true );
        rgb_max_value = 4095;
    }

    // Configure the widget:
    rgb_widget_focus = UI_RGB_BAR_RED;
    rgb_focus_locked = false;
    rgb_widget_title = item->label;
    input_mode = UI_INPUT_RGB;
    display_draw( true );
}


/***************************************************************************/

bool ui_draw( u8g_t * u8g_ref ) {

    initialize( u8g_ref );

//    draw_num_selector();
//    draw_log();
//    return true;

    if ( ui_active ) {

        switch ( input_mode ) {
            case UI_INPUT_MENU:
                draw_menu( menu_stack[ menu_stack_pos ] );
                break;

            case UI_INPUT_LOG:
                draw_log();
                break;

            case UI_INPUT_NUM:
                draw_num_selector();
                break;

            case UI_INPUT_YES_NO:
            case UI_INPUT_EDIT:
                break;

            case UI_INPUT_RGB:
                draw_rgb_config();
                break;
        }

        return true;
    } else {
        display_draw_full_screen_bitmap( (u8g_pgm_uint8_t *) mx13_logo_commando );
        return false;
    }
}


/***************************************************************************/

void ui_enter() {

    if ( ui_active ) {
        return;
    }

    ui_active = true;

    menu_stack[ 0 ] = &ui_menu;
    menu_stack_pos = 0;
    input_mode = UI_INPUT_MENU;

//    input_mode = UI_INPUT_LOG;
//    input_mode = UI_INPUT_NUM;

    display_draw( true );
    set_indicator( true, true );
}


/***************************************************************************/

void ui_log_append_byte( uint8_t c ) {

    ui_log_append_char( nibchar( c >> 4 ) );
    ui_log_append_char( nibchar( c & 0xf ) );
}


/***************************************************************************/

void ui_log_append_char( uint8_t c ) {

    int char_width = u8g_GetStrWidth( u8g, "w" );
    int max_cols = ( 128 - menu_list_hpad ) / char_width;

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
        menu_stack[ 0 ] = &ui_menu;
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
            enter_menu( &item->submenu, item->label );
            break;

        case UI_FLAG:
            set_flag_state(
                item->number, item->bit,
                ! get_flag_state( item->number, item->bit )
            );
            display_draw( false );
            break;

        case UI_LED_CONFIG:
            ui_led_menu.title = item->label;
            ui_led_menu.items[ 1 ].led_channel = item->led_channel;
            enter_menu( &ui_led_menu, NULL );
            break;

        case UI_NAV_PREV:
            break;

        case UI_NUM_SELECTOR:
            start_num_selector( current_menu, item );
            break;

        case UI_RGB_SELECTOR:
            start_rgb_selector( current_menu, item );
            break;

        case UI_SAVE:
            // Temporarily restore display LED so the UI indicator does not get saved:
            set_indicator( false, false );
            settings_save( MX13_SET_LEDS, &led_config );
#ifdef TRACKPOINT_ENABLE
            tp_save();
#endif
            set_indicator( true, false );
            break;

        default:
            break;
    }
}


/***************************************************************************/

void ui_handle_key( uint8_t layer, int keycode, bool is_pressed ) {

    switch ( input_mode ) {

        case UI_INPUT_NUM:
            handle_key_num( layer, keycode, is_pressed );
            break;

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
            handle_key_rgb( layer, keycode, is_pressed );
            break;
    }
}


/***************************************************************************/

void ui_leave() {

    if ( ! ui_active ) {
        return;
    }

    ui_active = false;

    // Reset LED if one was being configured:
    if ( input_mode == UI_INPUT_RGB ) {

        rgb_led->flags = rgb_prior_flags;
        if ( rgb_led->flags & PWM_LED_FLAGS_TEENSY ) {
            led_set_teensy_led( rgb_led );
        } else {
            pwm_set_rgb_led( rgb_led );
            pwm_commit( true );
        }
    }

    // Reset TrackPoint LED if it was being configured:
    else if (
        input_mode == UI_INPUT_NUM &&
        num_widget_number == UI_NUM_LED_TP_INTENSITY
    ) {
        led_set_trackpoint( led_config.trackpoint.on );
    }

    display_draw( true );
    set_indicator( false, true );
}


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
