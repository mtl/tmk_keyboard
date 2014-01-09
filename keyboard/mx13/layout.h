// MX13 SpaceSaver Layout
static const uint8_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    /* 0: qwerty */
    KEYMAP(\
        ESC, NLCK,     GRV, 1,   2,   3,   4,   5,   6,   7,   8,   9,   0,   MINS,EQL, BSPC,      INS, HOME,PGUP, \
        BRK, SLCK,     TAB, Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,   LBRC,RBRC,BSLS,      DEL, END, PGDN, \
        FN1, PSCR,     CAPS,A,   S,   D,   F,   G,   H,   J,   K,   L,   SCLN,QUOT,     ENT,                       \
                       LSFT,Z,   X,   C,   V,   B,   N,   M,   COMM,DOT, SLSH,          RSFT,           UP,        \
                       LCTL,LGUI,LALT,               SPC,                RALT, FN0,     RCTL,      LEFT,DOWN,RGHT ),

    /* 1: Function keys */
    KEYMAP(\
        TRNS,TRNS,     TRNS,F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9,  F10, F11, F12, EJCT,      TRNS,WHOM,VOLU, \
        TRNS,WAKE,     TRNS,TRNS,TRNS,TRNS,TRNS,CALC,TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,      TRNS,MUTE,VOLD, \
        TRNS,SLEP,     TRNS,TRNS,WSTP,TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,     TRNS,                      \
                       TRNS,TRNS,TRNS,CALC,TRNS,TRNS,TRNS,MAIL,TRNS,TRNS,TRNS,          TRNS,           TRNS,      \
                       WBAK,MENU,WFWD,               TRNS,               TRNS,TRNS,     TRNS,      MPRV,MPLY,MNXT ),

    /* 2: num-lock */
    KEYMAP(\
        TRNS,TRNS,     TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,P7  ,P8  ,P9  ,PSLS,TRNS,PEQL,TRNS,      TRNS,TRNS,TRNS, \
        TRNS,TRNS,     TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,P4  ,P5  ,P6  ,PAST,TRNS,TRNS,TRNS,      TRNS,TRNS,TRNS, \
        TRNS,TRNS,     TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,P1  ,P2  ,P3  ,PMNS,TRNS,     TRNS,                      \
                       TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,TRNS,P0  ,PCMM,PDOT,PPLS,          TRNS,           TRNS,      \
                       TRNS,TRNS,TRNS,               TRNS,               TRNS,TRNS,     TRNS,      TRNS,TRNS,TRNS )

//    KEYMAP(\
//        ESC, NLCK,     V,   M,   K,   X,   E,   H,   C,   D,   LSFT,BRK, 0,   MINS,EQL, BSPC,      INS, HOME,PGUP, \
//        PSCR,SLCK,     TAB, Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,   LBRC,RBRC,BSLS,      DEL, END, PGDN, \
//        FN0, BRK,      CAPS,A,   S,   D,   F,   G,   H,   J,   K,   L,   SCLN,QUOT,     ENT,                       \
//                       LSFT,Z,   X,   C,   V,   B,   N,   M,   COMM,DOT, SLSH,          RSFT,           UP,        \
//                       LCTL,LGUI,LALT,               BRK,                RALT,RGUI,     RCTL,      LEFT,DOWN,RGHT)
};
static const uint16_t PROGMEM fn_actions[] = {
    [0] = ACTION_LAYER_MOMENTARY(1),
    [1] = ACTION_LAYER_MOMENTARY(2)
};

/* vi: set et sts=4 sw=4 ts=4: */
