/****************************************************************************
 *
 *  User Interface - Menu definition
 *
 ***************************************************************************/

#include "ui.h"

#ifdef TRACKPOINT_ENABLE
#    include "trackpoint.h"
#    define UI_MAIN_MENU_ITEMS 5
#else
#    define UI_MAIN_MENU_ITEMS 4
#endif


/***************************************************************************/

ui_menu_t ui_led_menu = UI_MENU( NULL, 2,
    UI_MENU_ITEM_DUMMY( "Enable/disable" ),
    UI_MENU_ITEM_RGB_SELECTOR( "Color", 0 ),
);


/***************************************************************************/

ui_menu_t ui_menu = UI_MENU( "MX13 Config", UI_MAIN_MENU_ITEMS,
//    UI_MENU_ITEM_RGB_SELECTOR( "RGB Selector", LED_CAPS_LOCK_1 ),
    UI_MENU_ITEM_SUBMENU( "Keyboard", NULL, 2,
        UI_MENU_ITEM_SUBMENU( "Key repeat", NULL, 2,
            UI_MENU_ITEM_DUMMY( "Delay" ),
            UI_MENU_ITEM_DUMMY( "Rate" )
        ),
        UI_MENU_ITEM_DUMMY( "Debug" )
    ),

#ifdef TRACKPOINT_ENABLE
    UI_MENU_ITEM_SUBMENU( "TrackPoint", NULL, 4,

        // tactile output pulse commands?

        UI_MENU_ITEM_SUBMENU( "Basic config", NULL, 5,
            UI_MENU_ITEM_NUM_SELECTOR( "Pointer speed", UI_NUM_TP_SNSTVTY ),
            UI_MENU_ITEM_NUM_SELECTOR( "Precision speed", UI_NUM_TP_PRECISION ),
            UI_MENU_ITEM_NUM_SELECTOR( "V-scroll div", UI_NUM_TP_SCROLL_V ),
            UI_MENU_ITEM_NUM_SELECTOR( "H-scroll div", UI_NUM_TP_SCROLL_H ),
            UI_MENU_ITEM_FLAG( "Press-to-select", UI_NUM_TP_CONFIG, TP_BIT_PTSON )
        ),
        UI_MENU_ITEM_SUBMENU( "Advanced config", NULL, 4,
            UI_MENU_ITEM_SUBMENU( "Motion", NULL, 5,
                UI_MENU_ITEM_NUM_SELECTOR( "Mid speed plateau", UI_NUM_TP_VALUE6 ),
                UI_MENU_ITEM_NUM_SELECTOR( "Negative inertia", UI_NUM_TP_INERTIA ),
                UI_MENU_ITEM_FLAG( "Invert X axis", UI_NUM_TP_CONFIG, TP_BIT_FLIPX ),
                UI_MENU_ITEM_FLAG( "Invert Y axis", UI_NUM_TP_CONFIG, TP_BIT_FLIPY ),
                UI_MENU_ITEM_FLAG( "Xchg X/Y axes", UI_NUM_TP_CONFIG, TP_BIT_SWAPXY )
            ),
            UI_MENU_ITEM_SUBMENU( "Press-to-select", NULL, 7,
                UI_MENU_ITEM_SUBMENU( "Thresholds", NULL, 2,
                    UI_MENU_ITEM_NUM_SELECTOR( "Down", UI_NUM_TP_THR ),
                    UI_MENU_ITEM_NUM_SELECTOR( "Up", UI_NUM_TP_UTHR )
                ),
                UI_MENU_ITEM_NUM_SELECTOR( "Backup range", UI_NUM_TP_REACH ),
                UI_MENU_ITEM_NUM_SELECTOR( "Drag hysteresis", UI_NUM_TP_DRAGHYS ),
                UI_MENU_ITEM_NUM_SELECTOR( "Minimum drag", UI_NUM_TP_MINDRAG ),
                UI_MENU_ITEM_NUM_SELECTOR( "Z time constant", UI_NUM_TP_ZTC ),
                UI_MENU_ITEM_NUM_SELECTOR( "Jenks Curvature", UI_NUM_TP_JKCUR ),
                UI_MENU_ITEM_FLAG( "Skip backups", UI_NUM_TP_REG2D, TP_BIT_SKIPBACK )
            ),
            UI_MENU_ITEM_SUBMENU( "Drift control", NULL, 7,
                UI_MENU_ITEM_FLAG( "Disabled", UI_NUM_TP_REG23, TP_BIT_SKIPDRIFT ),
                UI_MENU_ITEM_NUM_SELECTOR( "Drift threshold", UI_NUM_TP_DRIFT ),
                UI_MENU_ITEM_NUM_SELECTOR( "Counter 1 reset", UI_NUM_TP_RSTDFT1 ),
                UI_MENU_ITEM_NUM_SELECTOR( "XY avg threshold", UI_NUM_TP_XYAVGTHR ),
                UI_MENU_ITEM_NUM_SELECTOR( "XY avg time const", UI_NUM_TP_XYDRIFTAVG ),
                UI_MENU_ITEM_NUM_SELECTOR( "Z drift limit", UI_NUM_TP_PDRIFTLIM ),
                UI_MENU_ITEM_NUM_SELECTOR( "Z drift reload", UI_NUM_TP_PDRIFT_REL )
            ),
            UI_MENU_ITEM_SUBMENU( "Calibration", NULL, 5,
                UI_MENU_ITEM_DUMMY( "Recalibrate now" ), // must wait 310 ms after
                UI_MENU_ITEM_FLAG( "Recalib pots now", UI_NUM_TP_REG23, TP_BIT_SETPOTS ), // wait 310 ms after
                UI_MENU_ITEM_FLAG( "Pot auto recalib", UI_NUM_TP_REG23, TP_BIT_SKIPPOTS ),
                UI_MENU_ITEM_FLAG( "Skip Z step", UI_NUM_TP_REG2D, TP_BIT_SKIPZSTEP ),
                UI_MENU_ITEM_NUM_SELECTOR( "XY origin time", UI_NUM_TP_XYAVG_FACTOR )
            )
        ),
        UI_MENU_ITEM_SUBMENU( "Info", NULL, 4,
            UI_MENU_ITEM_DUMMY( "Page 1" ),
            UI_MENU_ITEM_DUMMY( "Page 2" ),
            UI_MENU_ITEM_DUMMY( "Page 3" ),
            UI_MENU_ITEM_DUMMY( "Page 4" )
        ),
        UI_MENU_ITEM_SUBMENU( "Debug", NULL, 5,
            UI_MENU_ITEM_DUMMY( "Reload config" ), // reapply config from EEPROM
            UI_MENU_ITEM_DUMMY( "View POST results" ),
            UI_MENU_ITEM_DUMMY( "Reset to defaults" ),
            UI_MENU_ITEM_DUMMY( "Soft reset" ),
            UI_MENU_ITEM_DUMMY( "Hard reset" )
        )
    ),
#endif

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
        UI_MENU_ITEM_NUM_SELECTOR( "TrackPoint", UI_NUM_LED_TP_INTENSITY ),
        UI_MENU_ITEM_LED_CONFIG( "Layer", LED_DISPLAY )
    ),
    UI_MENU_ITEM_SUBMENU( "Firmware", NULL, 3,
        UI_MENU_ITEM_DUMMY( "Background" ),
        UI_MENU_ITEM_DUMMY( "Bootloader" ),
        UI_MENU_ITEM_DUMMY( "Restart" )
    ),
    UI_MENU_ITEM_SAVE( "Save" )
);


/***************************************************************************/

/* vi: set et sts=4 sw=4 ts=4: */
