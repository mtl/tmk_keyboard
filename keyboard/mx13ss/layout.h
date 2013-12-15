// SSMX Layout
static const uint8_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* 0: qwerty */
    //KEYMAP(\
        //ESC, NLCK,     GRV, 1,   2,   3,   4,   5,   6,   7,   8,   9,   0,   MINS,EQL, BSPC,      INS, HOME,PGUP, \
        //PSCR,SLCK,     TAB, Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,   LBRC,RBRC,BSLS,      DEL, END, PGDN, \
        //FN0, BRK,      CAPS,A,   S,   D,   F,   G,   H,   J,   K,   L,   SCLN,QUOT,     ENT,                       \
                       //LSFT,Z,   X,   C,   V,   B,   N,   M,   COMM,DOT, SLSH,          RSFT,           UP,        \
                       //LCTL,LGUI,LALT,               SPC,                RALT,RGUI,     RCTL,      LEFT,DOWN,RGHT)
    KEYMAP(\
        ESC, NLCK,     V,   M,   K,   X,   E,   H,   C,   D,   LSFT,BRK, 0,   MINS,EQL, BSPC,      INS, HOME,PGUP, \
        PSCR,SLCK,     TAB, Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,   LBRC,RBRC,BSLS,      DEL, END, PGDN, \
        FN0, BRK,      CAPS,A,   S,   D,   F,   G,   H,   J,   K,   L,   SCLN,QUOT,     ENT,                       \
                       LSFT,Z,   X,   C,   V,   B,   N,   M,   COMM,DOT, SLSH,          RSFT,           UP,        \
                       LCTL,LGUI,LALT,               BRK,                RALT,RGUI,     RCTL,      LEFT,DOWN,RGHT)
    //KEYMAP(\
        //A,   B,        GRV, 1,   2,   3,   4,   5,   6,   7,   8,   9,   0,   MINS,EQL, C,         D,   E,   F, \
        //G,   H,        I,   Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,   J,   K,   L,         M,   N,   O, \
        //P,   Q,        R,   A,   S,   D,   F,   G,   H,   J,   K,   L,   S,   T,        U,                      \
                       //V,   Z,   X,   C,   V,   B,   N,   M,   COMM,DOT, SLSH,          W,              X,      \
                       //Y,   Z,   0,                  1,                  2,   3,        4,         5,   6,   7)
    //KEYMAP(\
        //A,   B,        V,   M,   K,   X,   E,   H,   C,   D,   LSFT,9,   0,   MINS,EQL, C,         D,   E,   F, \
        //G,   H,        I,   Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,   J,   K,   L,         M,   N,   O, \
        //P,   Q,        CAPS,   A,   S,   D,   F,   G,   H,   J,   K,   L,   S,   T,        U,                      \
                       //V,   Z,   X,   C,   V,   B,   N,   M,   COMM,DOT, SLSH,          W,              X,      \
                       //Y,   Z,   0,                  BRK,                2,   NO,       4,         5,   6,   7)
};
static const uint16_t PROGMEM fn_actions[] = {
    [0] = ACTION_LAYER_MOMENTARY(1)
};

/* vi: set et sts=4 sw=4 ts=4: */
