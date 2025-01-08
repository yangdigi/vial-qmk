#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "progmem.h"
#include "timer.h"
#include "action.h"
#include "rgblight.h"
#include "debug.h"
#include "lufa.h"
#include "ble51.h"
#include "led.h"
#include "quantum.h"

//#define RGBLIGHT_FADING_ONOFF_ENABLE
#define RGBLIGHT_ALL_DISPLAY_CONNECTION_ENABLE
#define rgblight_timer_init() 
void rgblight_timer_enable(void) { DDRD  |= (1<<7); PORTD |= (1<<7); _delay_us(9);}
void rgblight_timer_disable(void) { PORTD &= ~(1<<7); DDRD  &= ~(1<<7); }
#define rgblight_timer_enabled (PORTD & (1<<7))

#define RGBLED_TEMP  RGBLED_NUM
#define RGB_INDICATOR_NUM 1

struct cRGB rgbled[RGBLED_NUM+RGB_INDICATOR_NUM+1]; 
struct cRGB *led = &rgbled[RGB_INDICATOR_NUM];

const uint8_t RGBLED_BREATHING_TABLE[64] PROGMEM= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 17, 20, 24, 28, 32, 36, 41, 46, 51, 57, 63, 70, 76, 83, 91, 98, 106, 113, 121, 129, 138, 146, 154, 162, 170, 178, 185, 193, 200, 207, 213, 220, 225, 231, 235, 240, 244, 247, 250, 252, 253, 254, 255};

#ifdef RGBLIGHT_FADING_ONOFF_ENABLE
static uint8_t rgb_fading_index = 0;
#endif

rgblight_config_t rgblight_config = {.enable = 0, .mode = 3,.hue = 192, .sat = 255, .val = 255};

void sethsv(uint8_t hue, uint8_t saturation, uint8_t brightness, struct cRGB *led1)
{
    uint8_t temp[5];
    uint16_t hue16 = hue*3;

    uint8_t n = hue16 >> 8;
    uint8_t x = ((((hue16 & 255) * saturation) >> 8) * brightness) >> 8;
    uint8_t s = ((256 - saturation) * brightness) >> 8;

    temp[0] = temp[3] = x + s;
    temp[1] = temp[4] = brightness - x;
    temp[2] = s;
    memcpy(led1, &temp[n], 3);
}

void setrgb(uint8_t r, uint8_t g, uint8_t b, struct cRGB *led1)
{
    (*led1).r = r;
    (*led1).g = g;
    (*led1).b = b;
}


uint32_t eeconfig_read_rgblight(void)
{
    return eeprom_read_dword(EECONFIG_RGBLIGHT);
}

void eeconfig_write_rgblight(uint32_t val)
{
    eeprom_update_dword(EECONFIG_RGBLIGHT, val);
}

void eeconfig_write_rgblight_default(void)
{
    dprintf("eeconfig_write_rgblight_default\n");

    eeconfig_write_rgblight(rgblight_config.raw);
}

void eeconfig_debug_rgblight(void) {
    dprintf("rgblight_config eeprom\n");
    dprintf("enable = %d\n", rgblight_config.enable);
    dprintf("mode = %d\n", rgblight_config.mode);
    dprintf("hue = %d\n", rgblight_config.hue);
    dprintf("sat = %d\n", rgblight_config.sat);
    dprintf("val = %d\n", rgblight_config.val);
}

void rgblight_init(void)
{
    dprintf("rgblight_init start!\n");
#if 1
    if (!eeconfig_is_enabled()) {
        dprintf("rgblight_init eeconfig is not enabled.\n");
        eeconfig_init();
        eeconfig_write_rgblight_default();
    }
#endif
    rgblight_config.raw = eeconfig_read_rgblight();
    if (rgblight_config.val == 0) rgblight_config.val = 127;

    eeconfig_debug_rgblight();

    rgblight_timer_init();

    rgblight_mode(rgblight_config.mode);
}

uint8_t limit_value_0to255(int16_t value) {
    if (value > 255) return 255;
    else if (value < 0) return 0;
    else return value;
}

void rgblight_mode(uint8_t mode)
{
    if (!rgblight_config.enable) {
#ifdef RGBLIGHT_FADING_ONOFF_ENABLE
        rgblight_clear();
        rgblight_set();
        rgb_fading_index = 0; 
#endif
        rgblight_timer_disable();
    } else {
    // rgb on
        #if 1 //less than 0 always be 0, can save 6B
        if (mode == 0xff) mode = RGBLIGHT_MODES - 1;
        else
        #endif
        if (mode >= RGBLIGHT_MODES) mode = 0;
        rgblight_config.mode = mode;
        dprintf("rgblight mode: %u\n", rgblight_config.mode);

        rgblight_timer_enable();
    }
    rgblight_sethsv(rgblight_config.hue, rgblight_config.sat, rgblight_config.val);
}

inline
void rgblight_toggle(void)
{
    rgblight_config.enable = !rgblight_config.enable;
    dprintf("rgblight toggle: rgblight_config.enable = %u\n", rgblight_config.enable);
    rgblight_mode(rgblight_config.mode);
}


void rgblight_action(uint8_t action)
{
    /* QMK: + before -
    0 toggle
    1 mode+    2 mode-
    3 hue+     4 hue-
    5 sat+     6 sat-
    7 val+     8 val-
    */
    uint8_t hue = rgblight_config.hue;
    uint8_t sat = rgblight_config.sat;
    uint8_t val = rgblight_config.val;
    int8_t increament = -1;
    if (action & 1) increament = 1;
    if (get_mods() & MOD_BIT(KC_LSHIFT)) {
        increament *= -1;
    } 
    switch (action) {
        case 0: 
            rgblight_toggle();
            break;
        case 1:
        case 2:
            rgblight_mode(rgblight_config.mode + increament);
            break;
        case 3:
        case 4:
            hue = rgblight_config.hue + RGBLIGHT_HUE_STEP * increament;
            break;
        case 5:
        case 6:
            sat = limit_value_0to255(rgblight_config.sat + RGBLIGHT_SAT_STEP * increament);
            break;
        case 7:
        case 8:
            val = limit_value_0to255(rgblight_config.val + RGBLIGHT_VAL_STEP * increament);
            break;
        default:
            break;
    }
    if (action >= 3) rgblight_sethsv(hue, sat, val);
}

void rgblight_sethsv_noeeprom(uint8_t hue, uint8_t sat, uint8_t val)
{
    if (rgblight_config.enable) {
        sethsv(hue, sat, val, &led[RGBLED_TEMP]);
        for (uint8_t i=0; i< RGBLED_NUM; i++) {
            led[i] = led[RGBLED_TEMP];
        }
        rgblight_set();
    }
}

void rgblight_sethsv(uint8_t hue, uint8_t sat, uint8_t val)
{
    //hue = hue_fix(hue);
    if (rgblight_config.enable) {
        #if 0   // will do rgblight_sethsv_noeeprom() in rgblight_task
        if (rgblight_config.mode == 1) {
            // same static color
            rgblight_sethsv_noeeprom(hue, sat, val);
        } 
        #endif
        rgblight_config.hue = hue;
        rgblight_config.sat = sat;
        rgblight_config.val = val;
        dprintf("rgblight set hsv [EEPROM]: %u,%u,%u\n", rgblight_config.hue, rgblight_config.sat, rgblight_config.val);
    }
    eeconfig_write_rgblight(rgblight_config.raw);
}

void rgblight_set_all_as(struct cRGB *led1)
{
    for (uint8_t i=0; i<(RGBLED_NUM + RGB_INDICATOR_NUM); i++) {
        rgbled[i] = *led1;
    }
}

void rgblight_setrgb(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint8_t i=0; i<(RGBLED_NUM + RGB_INDICATOR_NUM); i++) {
        rgbled[i].r = r;
        rgbled[i].g = g;
        rgbled[i].b = b;
    }
    rgblight_set();
}

void rgblight_clear(void)
{
    memset(&rgbled[RGB_INDICATOR_NUM], 0, RGBLED_NUM * 3);
}

#define RGB_FADING_STEP 2
void rgblight_set(void)
{
#ifdef RGBLIGHT_FADING_ONOFF_ENABLE

    bool rgb_fading = (rgb_fading_index < (63 - RGB_FADING_STEP) ||  kb_idle_times >= 13);
    if (rgb_fading) {
        if      (kb_idle_times >= 13 && rgb_fading_index >= RGB_FADING_STEP) rgb_fading_index -= RGB_FADING_STEP; // fading in
        else if (kb_idle_times <  13) rgb_fading_index += RGB_FADING_STEP; // fading out
        uint8_t *p = (uint8_t *)(&led[0]);
        for (uint8_t i=0;i<RGBLED_NUM*3;i++, *p++) {
            *p = (*p) * pgm_read_byte(&RGBLED_BREATHING_TABLE[rgb_fading_index]) / 256;
        }
    }
#endif
    ws2812_setleds(rgbled);
}

bool prevent_rgblight_task(void) {
    // 在某些情况下，不更新rgb灯效。比如低电量，或者底灯被用作指示时。
    if (low_battery) return 1;
#ifdef RGBLIGHT_ALL_DISPLAY_CONNECTION_ENABLE
    if (display_connection_status_check_times && ble51_boot_on) return 1;
#endif
    return 0;
}

void rgblight_task(void)
{
    if (prevent_rgblight_task()) return;
    if (rgblight_config.enable) {
        switch (rgblight_config.mode) {
            case 0:
                rgblight_sethsv_noeeprom(rgblight_config.hue, rgblight_config.sat, rgblight_config.val); //falcon 2020 LEDS need this.
                break;
            case 1 ... 3:
                rgblight_effect_rainbow_mood(rgblight_config.mode); // (1 to 3)
                break;
            case 4 ... 9:
                rgblight_effect_rainbow_swirl(rgblight_config.mode-2); // (2 to 7)
                break;
            #if RGBLIGHT_MODES > 11
            case 10 ... 13:
                rgblight_effect_breathing(rgblight_config.mode-9); //(1 to 4)
                break;
            #endif
            #if RGBLIGHT_MODES > 15
            case 14 ... 19:
                rgblight_effect_snake(rgblight_config.mode-14); // 0 to 5 (0 to 2)
                break;
            #endif
            #if RGBLIGHT_MODES > 21
            case 20 ... 22:
                rgblight_effect_knight(rgblight_config.mode-19); // 1 to 3
                break;
            #endif
        }
    }
}

// effects
void rgblight_effect_breathing(uint8_t interval)
{
    static uint8_t pos = 0;
    static int8_t increament = 1;
    rgblight_sethsv_noeeprom(rgblight_config.hue, rgblight_config.sat, pgm_read_byte(&RGBLED_BREATHING_TABLE[pos]));
    pos = pos + interval*increament;
    if (pos < interval || pos+interval > 62) {
        increament *= -1;
    }
}

static uint8_t current_hue = 0;

void rgblight_effect_rainbow_mood(uint8_t interval)
{
    rgblight_sethsv_noeeprom(current_hue, rgblight_config.sat, rgblight_config.val);
    current_hue += interval;
}

void rgblight_effect_rainbow_swirl(uint8_t interval)
{
    //uint8_t interval2 = interval/2;
    if (interval & 1) interval *= -1;
    for (uint8_t i=0; i<RGBLED_NUM; i++) {
        uint8_t hue = 256/16*i+current_hue;
        sethsv(hue, rgblight_config.sat, rgblight_config.val, &led[i]);
    }
    rgblight_set();
    current_hue = current_hue - interval*2;
}

#if RGBLIGHT_MODES > 15
void rgblight_effect_snake(uint8_t interval)
{
    static int8_t pos = 0 - RGBLIGHT_EFFECT_SNAKE_LENGTH;
    static uint8_t led_step = 0;
    int8_t increament = 1;
    uint8_t interval2 = interval/2; // speed
    if (++led_step > interval2) {
        led_step = 0;
        rgblight_clear();
        if (interval%2) increament = -1;
        for (uint8_t i=0; i<RGBLIGHT_EFFECT_SNAKE_LENGTH; i++) {
            int8_t target_led = pos+i*increament;
            if (target_led < 0) target_led += RGBLED_NUM;
            else if (target_led >= RGBLED_NUM) target_led -= RGBLED_NUM;
            sethsv(rgblight_config.hue+i*16, rgblight_config.sat, rgblight_config.val, &led[target_led]);
        }
        pos += increament;
        if (pos > RGBLED_NUM) pos = 0;
        else if (pos < 0 ) pos = RGBLED_NUM;
        rgblight_set();
    }
}
#endif

#if RGBLIGHT_MODES > 21
void rgblight_effect_knight(uint8_t interval)
{
    static int8_t pos = RGBLED_NUM - 1;
    static uint8_t time_step = 0;
    static int8_t increament = 1;
    if (++time_step > interval) {
        bool need_update = 0;
        rgblight_clear();
        for (uint8_t i=0; i<RGBLIGHT_EFFECT_KNIGHT_LENGTH; i++) {
            int8_t target_col = pos+i;
            if (target_col < RGBLED_NUM && target_col >= 0){
                need_update = 1;
                led[target_col] = led[RGBLED_TEMP];
            }
        }
        if (need_update) rgblight_set(); //Keep the first or last col on when increament changes.
        pos += increament;
        if (pos <= 0 - RGBLIGHT_EFFECT_KNIGHT_LENGTH || pos >= RGBLED_NUM) {
            increament *= -1;
            current_hue = current_hue + 16;
            sethsv(current_hue, rgblight_config.sat, rgblight_config.val, &led[RGBLED_TEMP]);
        }
        time_step = 0;
    }
}
#endif

void suspend_power_down_action(void)
{
    rgblight_timer_disable();
#ifdef RGBLIGHT_FADING_ONOFF_ENABLE
    rgb_fading_index = 0;
#endif
}

void suspend_wakeup_init_action(void)
{
    rgblight_init();
    ledmapu_state_restore();
}

void hook_keyboard_loop()
{
    if (BLE51_PowerState > 1) return;
    static uint16_t rgb_update_timer = 0;
    if (timer_elapsed(rgb_update_timer) > 40) {
        rgb_update_timer = timer_read();
        rgblight_task();
        indicator_task();
    }
}

void ble51_task_user(void)
{
    return;
}
