/*
Copyright 2022 YANG <drk@live.com>

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
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include "keycode.h"
#include "host.h"
#include "action.h"
#include "action_util.h"
#include "action_layer.h"
#include "lufa.h"
#include "ble51_task.h"
#include "ble51.h"
#include "debug.h"
#include "timer.h"
#include "wait.h"
#include "command.h"


#ifndef BT_POWER_SAVE_TIME
#define BT_POWER_SAVE_TIME 15000
#endif 

#ifndef BT_POWER_OFF_TIME
#define BT_POWER_OFF_TIME 9000000
#endif 

static void update_battery(uint8_t mode);
static void update_battery_value(uint8_t value);
static void ble51_reset(uint8_t pressed_mods);
uint16_t ble_reset_key  __attribute__ ((section (".noinit")));

extern debug_config_t debug_config;
extern bool kb_started;

bool ble51_boot_on = 1;
extern bool report_error;
extern char ble51_buf[];
bool need_clear = false;

static uint8_t disconnect_s=0;
static uint16_t check_timer = 0;
uint16_t kb_idle_times = 0; 


uint8_t usb_connected = 4;

bool bt_connected = 0;

uint8_t display_connection_status_check_times = 1;

#define U8V(x) (x/16 - 1024/16) //convert voltage to uint8_t (0-255)
static const uint8_t battery_level_value[9] = {U8V(4050), U8V(3950), U8V(3900), U8V(3800), U8V(3750), U8V(3700), U8V(3650), U8V(3600), U8V(3550)};
uint8_t battery_calc_u8 = 0;
uint16_t battery_voltage = 3699;
uint8_t battery_check_timer = 0;
uint8_t battery_percentage = 12; // default 12 for ble51_boot_off.

uint8_t ble_set_code = 0;

uint8_t low_battery = 0;

#ifdef UPDATE_BATTERY_WHEN_CHARGING
bool is_charging = 0;
#endif

uint8_t BLE51_PowerState = 1; 


void ble51_task_init(void)
{
#ifdef CHARGING_STATE_INIT
    CHARGING_STATE_INIT();
#endif
    update_battery(1);
}

__attribute__ ((weak))
void ble51_task_user(void){}


static uint8_t usb_nkro;

void ble51_task(void)
{
    // update USB states
    usb_connected = (USB_DeviceState == DEVICE_STATE_Configured)? 1 : 0; 

    /* Bluetooth mode | USB mode */
    if (keyboard_protocol & (1<<7)) {
        if (host_get_driver() != &ble51_driver) {
            print("Bluetooth\n");
            usb_nkro = keymap_config.nkro;
            host_set_driver(&ble51_driver);
        } 
        keymap_config.nkro = 0; // always disable NKRO for BT mode
    } else {
        if (host_get_driver() != &lufa_driver) {
            print("USB\n");
            keymap_config.nkro = usb_nkro;
            host_set_driver(&lufa_driver);
        } else if (!usb_connected) {
            if (ble51_boot_on) keyboard_protocol |= (1<<7);
            else BLE51_PowerState = 12;
        }
    }
    
    if (!ble51_boot_on) return;

    if (BT_POWERED) {
        if (report_error) {
            print("\nerroring...");
            if (memcmp(ble51_cmd("AT\n"), "OK", 2) == 0) {
                report_error = 0;
                ble51_clear_keys(); //clear keyboard
                print("error end...\n");
            } else {
                need_clear = 1;
            }
        }
        if (need_clear) {
            print("clearing\n");
            const char *result = ble51_gets(TIMEOUT);
            if ((memcmp(result, "OK", 2) != 0) && (memcmp(result, "ERROR", 5) != 0)) {
                need_clear = 0;
            }
        }

        
        if (BLE51_PowerState == 2) {
            update_battery(1);
            BLE51_PowerState = 3;
        }

           
        /* check  every 1s.*/
        if (!need_clear && timer_elapsed(check_timer) > 1000) {
            check_timer = timer_read();
            static uint8_t check_connection_times = 0;
            /* if bt_connected, check every 4s */
            if (!bt_connected || (++check_connection_times & 0b11) == 0) {
                /* Get connect status*/
                //const char *result = ble51_cmd("AT+GAPGETCONN\n");
                if (1) {
                    uint8_t conn_status = ble51_get_connection_status();
                    if (conn_status == '1') {
                        if (!bt_connected) print("BT connected\n");
                        bt_connected = 1;
                        disconnect_s = 0;
                    } else if (conn_status == '0') {
                        if (bt_connected) print("BT disconnected\n");
                        bt_connected = 0;
                        disconnect_s++;
                        /* if disconnected for more than 90s, turn off bt */
                        if (!usb_connected && BLE51_PowerState && disconnect_s >= 90) {
                            print("90s now. BT is off. Sleeping.\n");
                            //if (USB_DeviceState != DEVICE_STATE_Configured) {
                                turn_off_bt();
                                BLE51_PowerState = 4;
                            //}
                            disconnect_s = 0;
                        }
                    } else {
                        //xprintf("%s\n", result);
                        need_clear = 1;
                    }

                }
                /* Display connect status*/
                if (display_connection_status_check_times) {
                    if (display_connection_status_check_times == 100) {
                        display_connection_status_check_times = 0;
                        led_wakeup();
                    } else if (++display_connection_status_check_times >= 20 || bt_connected == 1) {
                        display_connection_status_check_times = 100;
                    }
                }
            }

            static uint8_t battery_check_times = 0;
            //update battery every 8s
            if ((++battery_check_times & 0b111) == 0) {
#ifdef UPDATE_BATTERY_WHEN_CHARGING
                /* update bettery when charging */
                update_battery(is_charging);
#else
                /* check battery voltage while not dozing */
                if (BLE51_PowerState < 2) update_battery(0);
#endif
            }

            if (!usb_connected && !display_connection_status_check_times) {
                kb_idle_times++;
                dprintf("idle times: %d, PowerState: %d\n", kb_idle_times, BLE51_PowerState);

                if (BLE51_PowerState == 1 && (kb_idle_times >= (BT_POWER_SAVE_TIME/1000))) {
                    print("dozing\n");
                    ble51_clear_keys();
                    BLE51_PowerState = 2;
                }

                if (BLE51_PowerState == 3 && (kb_idle_times >= (BT_POWER_OFF_TIME/1000))) {
                    print("BT is idle for a long time. Turn off. \n");
                    BLE51_PowerState = 4;
                    turn_off_bt();
                }
            }
        }

       

        if (ble_set_code > 0) {
            if (ble_set_code == KC_I) {
                ble51_cmd("AT+GAPSETADVDATA=02-01-06\n");
            } else if (ble_set_code == KC_O) {
                ble51_cmd("AT+GAPSETADVDATA=02-01-04\n");
            } else if (ble_set_code == KC_R) {
                ble51_del_bonds();
                ble51_set_blehiden('0');
                ble51_set_blehiden('1');
                ble51_cmd("ATZ\n");
                ble51_cmd("AT+GAPSETADVDATA=02-01-06\n");
            }
            ble_set_code = 0;
        }

        /* hold consumer key */
        ble51_consumer_task();
    }



    if (BLE51_PowerState <= 1) ble51_task_user();
}

static void update_battery(uint8_t mode) {
    /* mode 0: not update bat_value, only get voltage and calc low_battery. */

    /*only update when light is off.*/
#ifdef BLE_LIGHT_ON
    if (BLE_LIGHT_ON && mode == 0) return;
#endif

#ifndef NO_DEBUG
    if (debug_config.enable == 0 && mode == 0) return;
#endif

    const char *result = ble51_cmd("AT+HWADC=6\n");
    if (result[1] < '0' || result[1] > '9') need_clear = 1;
    else if(result[2] > '9') print("No battery or wrong!\n"); 
    else {
        uint16_t battery_voltage_get = ((result[0]-48)*100 + (result[1]-48)*10 + result[2]- 48 -32) * 8 + 54;
        battery_voltage = (battery_voltage_get + ((battery_voltage == 3699)?battery_voltage_get:battery_voltage)) / 2;
        dprintf(" Battery Voltage: %dmv, ADC Value:%s\n", battery_voltage, result);
        
#ifdef UPDATE_BATTERY_WHEN_CHARGING
        if (BATTERY_CHARGING) {
            is_charging = 1;
            battery_voltage -= CHARGING_FIX_VALUE;
        } else { 
            is_charging = 0;
        }
#endif
        battery_calc_u8 = U8V(battery_voltage);
        if (battery_calc_u8 > U8V(3600)) low_battery = 0;
        else if (battery_calc_u8 < U8V(3420)) low_battery = 1;
        else if (battery_calc_u8 < U8V(3550)) low_battery = 5; 
    }    


#ifdef BLE_BATTERY_SERVICE
    if (mode == 1) {
        uint8_t battery_level = 44;
        uint8_t i = 0;
        for (uint8_t j = 90; j > 0; j -= 10) {
            if (battery_level_value[i++] <= battery_calc_u8) { 
                battery_level = j; 
                break;
            }
        }
        if (low_battery) battery_level = low_battery;

#ifdef UPDATE_BATTERY_WHEN_CHARGING
        battery_level += is_charging; //+1 when is_charging
    #ifdef CHARGING_STATE_INIT
        if (!is_charging && battery_calc_u8 >= U8V(4140)) battery_level = 100;
    #else //USB VBUS
        if (battery_calc_u8 >= U8V(4140)) battery_level = 100;
    #endif
#else
        if (battery_calc_u8 >= U8V(4140)) battery_level = 100;
#endif 
        update_battery_value(battery_level);
    }
#endif
}

static void update_battery_value(uint8_t value) {
    battery_percentage = value;
    ble51_puts("AT+GATTCHAR=1,");
    sprintf(ble51_buf, "%d\n", value);
    ble51_cmd(ble51_buf);
}

#define MODS_SHIFT_MASK (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_RSHIFT))
static void ble51_reset(uint8_t pressed_mods) {
    if (pressed_mods & (MOD_BIT(KC_LCTRL) | MODS_SHIFT_MASK)) {
        if ((pressed_mods & (~MODS_SHIFT_MASK)) == MOD_BIT(KC_LCTRL)) {
            *(uint16_t *)0x0800 = 0xFC2B;
        }
        for (;;);
    } else if (pressed_mods == (MOD_BIT(KC_LALT) | MOD_BIT(KC_LGUI))) {
        ble_reset_key = 0xBBAA;
        for (;;);
    }
}


bool command_extra(uint8_t code)
{
    uint8_t pressed_mods = get_mods();
    clear_keyboard();
    switch (code) {
        case KC_U:
            if (ble51_boot_on) keyboard_protocol ^= (1<<7);
            return true;
        case KC_I:
        case KC_O:
            ble_set_code = code;
            return true;
        case KC_R:
            if  (pressed_mods&MOD_BIT(KC_LCTRL)) {
                ble_set_code = code;
                display_connection_status_check_times = 1;
                return true;
            } else {
                return false;
            }
        case KC_S:
            display_connection_status_check_times = display_connection_status_check_times? 100:1;
            return true;    
        case KC_P:
            BLE51_PowerState ^= 1;
            type_num(BLE51_PowerState);
            return true;
        case KC_B:
            ble51_reset(pressed_mods);
            return true;
        case KC_V:
            type_numbers(battery_percentage);
            if (pressed_mods & MOD_BIT(KC_LCTRL)) {
                tap_code(KC_MINS);
                type_numbers(battery_voltage);
            }
            tap_code(KC_MINS);
            type_num(bt_connected);
            return true;
        case KC_L:
            if (!usb_connected) {
                turn_off_bt();
                BLE51_PowerState = 10;
                bt_connected = 0;
                layer_clear();
            }
            return true;
        default:
            return false;
    }
    return true;
}

