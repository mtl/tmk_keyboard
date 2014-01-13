/*
Copyright 2012,2013 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdint.h>
#include <stdbool.h>
#include <avr/pgmspace.h>
#include "keycode.h"
#include "action.h"
#include "action_macro.h"
#include "report.h"
#include "host.h"
#include "print.h"
#include "debug.h"
#include "keymap.h"
#include "matrix.h"


#define KEYMAP( \
    K0A, K0B, K0C, K0D, K0E, K0F, K0G, K0H, K0I, K0J, K0K, K0L, K0M, K0N, K0O, K0P, K0Q, K0R, K0S, \
    K1A, K1B, K1C, K1D, K1E, K1F, K1G, K1H, K1I, K1J, K1K, K1L, K1M, K1N, K1O, K1P, K1Q, K1R, K1S, \
    K2A, K2B, K2C, K2D, K2E, K2F, K2G, K2H, K2I, K2J, K2K, K2L, K2M, K2N,      K2P,                \
              K3C, K3D, K3E, K3F, K3G, K3H, K3I, K3J, K3K, K3L, K3M,           K3P,      K3R,      \
              K4C, K4D, K4E,                K4I,                K4M, K4N,      K4P, K4Q, K4R, K4S  \
) { \
/*             0         1         2         3         4         5         6         7         8         9         10        11        12        13        14        15        16        17        18   */  \
/* 0 */   { KC_##K0A, KC_##K0B, KC_##K0C, KC_##K0D, KC_##K0E, KC_##K0F, KC_##K0G, KC_##K0H, KC_##K0I, KC_##K0J, KC_##K0K, KC_##K0L, KC_##K0M, KC_##K0N, KC_##K0O, KC_##K0P, KC_##K0Q, KC_##K0R, KC_##K0S}, \
/* 1 */   { KC_##K1A, KC_##K1B, KC_##K1C, KC_##K1D, KC_##K1E, KC_##K1F, KC_##K1G, KC_##K1H, KC_##K1I, KC_##K1J, KC_##K1K, KC_##K1L, KC_##K1M, KC_##K1N, KC_##K1O, KC_##K1P, KC_##K1Q, KC_##K1R, KC_##K1S}, \
/* 2 */   { KC_##K2A, KC_##K2B, KC_##K2C, KC_##K2D, KC_##K2E, KC_##K2F, KC_##K2G, KC_##K2H, KC_##K2I, KC_##K2J, KC_##K2K, KC_##K2L, KC_##K2M, KC_##K2N, KC_NO,    KC_##K2P, KC_NO,    KC_NO,    KC_NO   }, \
/* 3 */   { KC_NO,    KC_NO,    KC_##K3C, KC_##K3D, KC_##K3E, KC_##K3F, KC_##K3G, KC_##K3H, KC_##K3I, KC_##K3J, KC_##K3K, KC_##K3L, KC_##K3M, KC_NO,    KC_NO,    KC_##K3P, KC_NO,    KC_##K3R, KC_NO   }, \
/* 4 */   { KC_NO,    KC_NO,    KC_##K4C, KC_##K4D, KC_##K4E, KC_NO,    KC_NO,    KC_NO,    KC_##K4I, KC_NO,    KC_NO,    KC_NO,    KC_##K4M, KC_##K4N, KC_NO,    KC_##K4P, KC_##K4Q, KC_##K4R, KC_##K4S}  \
}

#include "layout.h"

#define KEYMAPS_SIZE    (sizeof(keymaps) / sizeof(keymaps[0]))
#define FN_ACTIONS_SIZE (sizeof(fn_actions) / sizeof(fn_actions[0]))

#define MX13_UI_LOCK KC_FN1
bool keymap_ui_lock = false;

bool keymap_is_pressed( key_t key ) {
    matrix_row_t matrix_row = matrix_get_row( key.row );
    return matrix_row & ((matrix_row_t)1<<key.col);
}

/* translates key to keycode */
uint8_t keymap_key_to_keycode(uint8_t layer, key_t key)
{

    uint8_t keycode = 0;

    if (layer < KEYMAPS_SIZE) {
        keycode = pgm_read_byte(&keymaps[(layer)][(key.row)][(key.col)]);
    } else {
        // XXX: this may cuaes bootlaoder_jump inconsistent fail.
        //debug("key_to_keycode: base "); debug_dec(layer); debug(" is invalid.\n");
        // fall back to layer 0
        keycode = pgm_read_byte(&keymaps[0][(key.row)][(key.col)]);
    }

    // Handle UI lock key:
    if ( keycode == MX13_UI_LOCK ) {
        keymap_ui_lock = keymap_is_pressed( key );
        return KC_NO;
    }

    // Redirect keypresses during UI lock:
    if ( keymap_ui_lock ) {

        // call UI...
        // ui_handle_key( key, keymap_is_pressed( key ) );

        return KC_NO;
    }

    // Let TMK handle the keycode:
    return keycode;
}

/* translates Fn keycode to action */
action_t keymap_fn_to_action(uint8_t keycode)
{
    action_t action;

    action.code = ACTION_NO;

    if (FN_INDEX(keycode) < FN_ACTIONS_SIZE) {
        action.code = pgm_read_word(&fn_actions[FN_INDEX(keycode)]);
    } else {
        action.code = ACTION_NO;
    }

    return action;
}

/* vi: set et sts=4 sw=4 ts=4: */
