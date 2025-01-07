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
#include "ble51_task.h"
#include "backlight.h"
#include "led.h"
#include "quantum.h"

#define rgblight_timer_init() do { DDRD |= (1<<6);} while(0)
#define rgblight_timer_enable() do { PORTD &= ~(1<<6);} while(0)
#define rgblight_timer_disable() do { PORTD |= (1<<6);} while(0)
#define rgblight_timer_enabled (~PORTD & (1<<6))

#define RGBLED_TEMP  RGBLED_NUM
#define INDICATOR_NUM 0
struct cRGB rgbled[RGBLED_NUM+INDICATOR_NUM+1]; 
struct cRGB *led = &rgbled[INDICATOR_NUM];

//const uint8_t RGBLED_BREATHING_TABLE[128] PROGMEM= {0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,17,18,20,21,23,25,27,29,31,33,35,37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,76,79,82,85,88,90,93,97,100,103,106,109,112,115,118,121,124,127,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255};
const uint8_t RGBLED_BREATHING_TABLE[64] PROGMEM= { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 17, 20, 24, 28, 32, 36, 41, 46, 51, 57, 63, 70, 76, 83, 91, 98, 106, 113, 121, 129, 138, 146, 154, 162, 170, 178, 185, 193, 200, 207, 213, 220, 225, 231, 235, 240, 244, 247, 250, 252, 253, 254, 255};


rgblight_config_t rgblight_config = {.enable = 0, .mode = 8,.hue = 214, .sat = 255, .val = 255};

void sethsv(uint8_t hue, uint8_t saturation, uint8_t brightness, struct cRGB *led1)
{
    /*
    original code: https://blog.adafruit.com/2012/03/14/constant-brightness-hsb-to-rgb-algorithm/
    use 8bit hue, hue16 = hue*3 to reach 0 to 767.
    */
    uint8_t r, g, b;
    uint8_t temp[5];
    uint16_t hue16 = hue*3;

    uint8_t n = hue16 >> 8;
    uint8_t x = ((((hue16 & 255) * saturation) >> 8) * brightness) >> 8;
    uint8_t s = ((256 - saturation) * brightness) >> 8;

    //temp[n] g r b as struct cRGB of ws2812. save 18B
    temp[0] = temp[3] = x + s;
    temp[1] = temp[4] = brightness - x;
    temp[2] = s;
    memcpy(led1, &temp[n], 3);
}

#if 0  // my old code. the new way use 8bit hue saves about 134B
uint16_t hue_fix(uint16_t hue)
{
    // hue needs to be 0x100 to 0x3ff
    hue += 0x300; 
    while (hue > 0x3ff) hue -= 0x300;  
    return hue;
}

void sethsv(uint16_t hue, uint8_t saturation, uint8_t brightness, struct cRGB *led1)
{
    /*
    original code: https://blog.adafruit.com/2012/03/14/constant-brightness-hsb-to-rgb-algorithm/
    when calculating hue, it may below 0.
    So I save hue as 0x100 to 0x3ff (256 to 1023) instead of (0 to 767).
    And n changes from 0-2 to 1-3.
    */
    uint8_t r, g, b;
    uint8_t temp[5];
    // uint8_t n = (hue >> 8) % 3; 
    hue = hue_fix(hue);
    uint8_t n = hue >> 8;
    if (n > 3) return;  // 0 would be error. just leave it.
    uint8_t x = ((((hue & 255) * saturation) >> 8) * brightness) >> 8;
    uint8_t s = ((256 - saturation) * brightness) >> 8;
#if 0 //temp[n-1] b g r
    temp[0] = temp[3] = s;
    temp[1] = temp[4] = x + s;
    temp[2] = brightness - x;
    r = temp[n + 1];
    g = temp[n];
    b = temp[n - 1];
    setrgb(r,g,b, led1);
#else //temp[n-1] g r b as struct cRGB of ws2812. save 18B
    temp[0] = temp[3] = x + s;
    temp[1] = temp[4] = brightness - x;
    temp[2] = s;
    memcpy(led1, &temp[n-1], 3);
#endif
}
#endif

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

    eeconfig_write_rgblight(rgblight_config.raw); //rgblight_config.raw);
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
#if 0
    if (!eeconfig_is_enabled()) {
        dprintf("rgblight_init eeconfig is not enabled.\n");
        eeconfig_init();
        eeconfig_write_rgblight_default();
    }
#endif
    rgblight_config.raw = eeconfig_read_rgblight();
    if (rgblight_config.val == 0) rgblight_config.val = 127;

    eeconfig_debug_rgblight(); // display current eeprom values

    rgblight_timer_init(); // setup the timer

    rgblight_mode(rgblight_config.mode);
}

uint8_t limit_value_0to255(int16_t value) {
    if (value > 255) return 255;
    else if (value < 0) return 0;
    else return value;
}

void rgblight_mode(int8_t mode)
{
    // rgb off, new way to save about 60B
    if (!rgblight_config.enable) {
        //rgblight_clear();
        //rgblight_set();
        rgblight_timer_disable();
    } else {
    // rgb on
        #if 0 //less than 0 can always be 0. save 8B
        if (mode < 0) mode = RGBLIGHT_MODES - 1;
        else
        #endif
        if (mode >= RGBLIGHT_MODES) mode = 0;
        rgblight_config.mode = mode;
        dprintf("rgblight mode: %u\n", rgblight_config.mode);

        rgblight_timer_enable();
    }
    // save config. rgblight_sethsv() will save config.
    //eeconfig_write_rgblight(rgblight_config.raw);
    rgblight_sethsv(rgblight_config.hue, rgblight_config.sat, rgblight_config.val);
}

inline
void rgblight_toggle(void)
{
    rgblight_config.enable ^= 1;
    dprintf("rgblight toggle: rgblight_config.enable = %u\n", rgblight_config.enable);
    rgblight_mode(rgblight_config.mode);
}


void rgblight_action(uint8_t action)
{
    /* QMK 键值顺序先加后减
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
        //rgblight_set();
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
    // when rgblight_config.enable == 0, just save config.raw.
    eeconfig_write_rgblight(rgblight_config.raw);
}

void rgblight_setrgb(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint8_t i=0; i<(RGBLED_NUM + INDICATOR_NUM); i++) {
        rgbled[i].r = r;
        rgbled[i].g = g;
        rgbled[i].b = b;
    }
    //rgblight_set();
}

void rgblight_clear(void)
{
    memset(rgbled, 0, sizeof(rgbled));
}

void rgblight_set(void)
{
    ws2812_setleds(rgbled);
}

inline
void rgblight_task(void)
{
    //if (rgblight_config.enable && rgblight_timer_enabled) {
    if (!low_battery) {
      if (rgblight_config.enable) {
        // Mode = 1, static light, do nothing here
        switch (rgblight_config.mode+1) {
            case 1:
                rgblight_sethsv_noeeprom(rgblight_config.hue, rgblight_config.sat, rgblight_config.val); //falcon 2020 LEDS need this.
                break;
            case 2 ... 5:
                rgblight_effect_breathing(rgblight_config.mode);
                break;
            case 6 ... 8:
                rgblight_effect_rainbow_mood(rgblight_config.mode-4);
                break;
            case 9 ... 14:
                rgblight_effect_rainbow_swirl(rgblight_config.mode-6);
                break;
            #if RGBLIGHT_MODES > 15
            case 15 ... 20:
                rgblight_effect_snake(rgblight_config.mode-14); // 0 to 5 (0 to 2)
                break;
            #endif
            #if RGBLIGHT_MODES > 21
            case 21 ... 23:
                rgblight_effect_knight(rgblight_config.mode-19);
                break;
            #endif
        }
        rgblight_set();
      } else {
        rgblight_timer_disable();
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
    //if (pos < interval || pos+interval > 126) {
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
    //rgblight_set();
    current_hue = current_hue - interval*2;
}

#if RGBLIGHT_MODES > 15
void rgblight_effect_snake(uint8_t interval)
{
    static int8_t pos = 0 - RGBLIGHT_EFFECT_SNAKE_LENGTH;
    static uint8_t led_step = 0;
    uint8_t i;
    int8_t increament = 1;
    uint8_t interval2 = interval/2; // speed
    if (++led_step > interval2) {
        led_step = 0;
        rgblight_clear();
        if (interval%2) increament = -1;
        for (i=0; i<RGBLIGHT_EFFECT_SNAKE_LENGTH; i++) {
            int8_t target_led = pos+i*increament;
            if (target_led < 0) target_led += RGBLED_NUM;
            else if (target_led >= RGBLED_NUM) target_led -= RGBLED_NUM;
            sethsv(rgblight_config.hue+i*16, rgblight_config.sat, rgblight_config.val, &led[target_led]);
        }
        pos += increament;
        if (pos > RGBLED_NUM) pos = 0;
        else if (pos < 0 ) pos = RGBLED_NUM;
        //rgblight_set();
    }
}
#endif

#if RGBLIGHT_MODES > 21
void rgblight_effect_knight(uint8_t interval)
{
    static int8_t pos = RGBLED_NUM - 1;
    static uint8_t sled_step = 0;
    uint8_t i;
    static int8_t increament = 1;
    if (++sled_step > interval) {
        bool need_update = 0;
        sled_step = 0;
        rgblight_clear();
        for (i=0; i<RGBLIGHT_EFFECT_KNIGHT_LENGTH; i++) {
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
    }
}
#endif

void suspend_power_down_action(void)
{
    PORTE &= ~(1<<6);
    PORTF &= ~(1<<0);
    PORTB &= ~(1<<2);
    DDRC &= ~(1<<6); //backlight_disable();
    
    rgblight_timer_disable(); //RGB_VCC off    
}

void suspend_wakeup_init_action(void)
{
    rgblight_init();
    DDRC |= (1<<6);
}

void hook_keyboard_loop()
{
    if (BLE51_PowerState > 1) return;
    static uint16_t rgb_update_timer = 0;
    static uint8_t steps = 0;
    if (timer_elapsed(rgb_update_timer) > 40) {
        rgb_update_timer = timer_read();
        //if (!display_connection_status_check_times || !ble51_boot_on) rgblight_task();
        rgblight_task();

        if ((steps++ & 0b11)) return;
        //led_status_task();
        // run every 4*40 = 160ms
        PORTE &= ~(1<<6); // led1 off
        PORTF &= ~(1<<0); // led2 off
        PORTB &= ~(1<<2); // led3 off
        // low_battery and display_connection_status when ble51_on
        if (ble51_boot_on && (low_battery || display_connection_status_check_times)) {
            rgblight_timer_enable();
            // 320ms on,320ms off. bt connected: 320ms*3 on, 320ms off.
            if (low_battery) {
                backlight_disable();
                rgblight_timer_disable(); 
                if (steps & 0b1000) PORTB |= (1<<2);
            } else {
                if (bt_connected) {
                    if (steps & 0b11000) { PORTB |= (1<<2); PORTF |= (1<<0); }
                } else {
                    if (steps & 0b1000)  { PORTB |= (1<<2); }
                }
            }
        } 
        // capslock
        if (host_keyboard_leds() & (1<<USB_LED_CAPS_LOCK)) {
            PORTE |= (1<<6);
        }
    }
}

void ble51_task_user(void)
{
    return;
}
