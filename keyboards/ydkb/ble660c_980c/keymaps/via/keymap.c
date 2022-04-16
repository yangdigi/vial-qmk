#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = {
    { KC_LEFT,KC_RCTL,KC_RALT,KC_NO,  KC_DOWN,KC_PDOT,KC_RGHT,KC_P0,  KC_X,   KC_LGUI, KC_GRV, KC_V,   KC_NO,  KC_ESC, KC_M,   KC_SPC, },
    { MO(1),  KC_DOT, KC_NO,  KC_NO,  KC_P1,  KC_PENT,KC_P2,  KC_P3,  KC_Z,   KC_LALT, KC_LCTL, KC_C,   KC_K,  KC_NO,  KC_N,   KC_B,   },
    { KC_QUOT,KC_SLSH,KC_COMM,KC_NO,  KC_P4,  KC_PPLS,KC_P5,  KC_P6,  KC_D,   KC_A,   KC_LSFT, KC_F,   KC_J,   KC_F1,  KC_H,   KC_G,   },
    { KC_RSFT,KC_SCLN,KC_L,   KC_RBRC,KC_UP,  KC_NO,  KC_NO,  KC_NO,  KC_S,   KC_Q,   KC_CAPS, KC_R,   KC_I,   KC_F3,  KC_U,   KC_T,   },
    { KC_EQL, KC_MINS,KC_0,   KC_BSLS,KC_NLCK,KC_BSPC,KC_PSLS,KC_PAST,KC_3,   KC_2,   KC_NO,   KC_4,   KC_9,   KC_F2,  KC_7,   KC_6,   },
    { KC_LBRC,KC_P,   KC_O,   KC_ENT, KC_P7,  KC_PMNS,KC_P8,  KC_P9,  KC_W,   KC_1,   KC_TAB,  KC_E,   KC_8,   KC_F4,  KC_Y,   KC_5,   },
    { KC_F11, KC_F10, KC_F9,  KC_F12, KC_DEL, KC_PGDN,KC_INS, KC_PGUP,KC_NO,  KC_NO,  KC_NO,   KC_NO,  KC_F8,  KC_F5,  KC_F7,  KC_F6,  }
    },  
  [1] = {
    { KC_HOME,KC_TRNS,KC_TRNS,KC_TRNS,KC_PGDN,KC_TRNS,KC_END, KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS, KC_GRV,KC_TRNS,KC_TRNS, },
    { KC_TRNS,KC_PGDN,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,USER03, KC_TRNS,KC_TRNS,KC_TRNS,KC_HOME,KC_TRNS,KC_TRNS,USER01,  },
    { KC_TRNS,KC_TRNS,KC_END, KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_MUTE,KC_VOLD,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS, },
    { KC_TRNS,KC_TRNS,KC_PGUP,KC_TRNS,KC_PGUP,KC_TRNS,KC_TRNS,KC_TRNS,KC_VOLU,KC_TRNS,KC_TRNS,KC_TRNS,KC_PSCR,KC_TRNS,KC_TRNS,KC_TRNS, },
    { KC_F12, KC_F11, KC_F10, KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_F3,  KC_F2,  KC_TRNS,KC_F4,  KC_F9,  KC_TRNS,KC_F7,  KC_F6,   },
    { KC_TRNS,KC_PAUS,KC_SLCK,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_F1,  USER00, USER02, KC_F8,  KC_TRNS,KC_TRNS,KC_F5,   },
    { KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS, } 
    },
  [2] = {
    {_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______, }, 
    {_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______, }, 
    {_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______, }, 
    {_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______, }, 
    {_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______, }, 
    {_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______, }, 
    {_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______,_______, } 
    },
};
