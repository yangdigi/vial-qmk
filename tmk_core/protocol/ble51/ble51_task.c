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

//static void update_battery(uint8_t mode);
static void update_battery_value(void);
static void ble51_reset(uint8_t pressed_mods);
//static void ble51_device_switching(uint8_t mode); //device_switching costs about 212B
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
bool usb_connected_once = 0;

bool bt_connected = 0;

//bool display_connection_status = true;
uint8_t display_connection_status_check_times = 1;

//#define U8V(x) (x/16 - 1024/16) //convert voltage to uint8_t (0-255)
#define U8V(x) (x/8 - (330-25)) //adc_u8
static const uint8_t battery_level_value[9] = {U8V(4070), U8V(3970), U8V(3900), U8V(3800), U8V(3750), U8V(3700), U8V(3650), U8V(3600), U8V(3550)};
uint8_t battery_calc_u8 = 0;
uint16_t battery_voltage = 3699;
uint8_t battery_check_timer = 0;

#ifdef HARDWARE_BT_SWITCH
uint8_t battery_percentage = 120; // default 120 for ble51_boot_off.
#else
uint8_t battery_percentage = 12; // default 12 for ble51_boot_off.
#endif

uint8_t ble_set_code = 0;

uint8_t low_battery = 0;

#ifdef UPDATE_BATTERY_WHEN_CHARGING
bool is_charging = 0;
#endif

// BLE51_PowerState: work 1 | dozing 2 | update battery 3(delete) | sleeping 4 | lock mode 10
// BLE51_PowerDozing_EN: default 1 | no_sleep 0
// ble51_off and no usb, 12
uint8_t BLE51_PowerState = 1; 
uint8_t BLE51_PowerDozing_EN = 1; 


void ble51_task_init(void)
{
#ifdef CHARGING_STATE_INIT
    CHARGING_STATE_INIT();
#endif
    update_battery(1); //update battery value when keyboard boots.
}

__attribute__ ((weak))
void ble51_task_user(void){}


void usb_bt_state_task(void)
{
    // update USB states
    usb_connected = (USB_DeviceState == DEVICE_STATE_Configured)? 1 : 0; 

    /* Bluetooth mode | USB mode */
    if (keyboard_protocol & (1<<7)) {
        if (host_get_driver() != &ble51_driver) {
            //usb_connected = 0; //set it to 0 to reconfirm USB_DeviceState
            print("Bluetooth\n");
            //ble51_usb_nkro = keymap_config.nkro;
            host_set_driver(&ble51_driver);
            //function_led_state &= ~(1<<1); //Bluetooth Mode, LED off
            //keyboard_function_led_set();
        }
        if (!usb_connected_once && usb_connected) {
            keyboard_protocol &= ~(1<<7);
        }
    } else {
        if (!usb_connected) {
            // no usb, switch to bt if ble51_boot_on
            if (ble51_boot_on) keyboard_protocol |= (1<<7);
            else BLE51_PowerState = 12;
        } else if (host_get_driver() != &lufa_driver) {
            print("USB\n");
            host_set_driver(&lufa_driver);
            //function_led_state |= (1<<1); //USB Mode, LED On
            //keyboard_function_led_set();
            usb_connected_once = 1;
        }
    }
}

void ble51_task(void)
{
    //if (!ble51_boot_on) return;

    if (BT_POWERED) {
#ifndef DEBUG_BLE51 
        //get the result(OK or ERROR) of keyboard_cmd ble51_puts() with no waiting. 
        ble51_clear_recv2();
        // to confirm report_clear(key up action) is executed
        if (g_u8_kbd_report_clear_failed_count && bt_connected) {
            ble51_clear_keys();
            // After many failures, try to restart.
            xprintf("clear failed times: %d\n", g_u8_kbd_report_clear_failed_count);
            if (g_u8_kbd_report_clear_failed_count == 16) {
              #ifdef bt_power_reset()
                //run only once when the count is specified.
                bt_connected = 0;
                bt_power_reset();
                print("bt power reset\n");
              #else
                for (;;);
              #endif
            }
        }
#else 
        /* if error */
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
#endif

        /* check  every 1s.*/
        if (!need_clear && timer_elapsed(check_timer) >= 1000) {
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
                        /* if usb_connected == 0 and disconnected for more than 90s, turn off bt */
                        //if (!usb_connected && BLE51_PowerState && disconnect_s >= 90) {
                        if (!usb_connected && disconnect_s >= 90) {
                            print("90s now. BT is off. Sleeping.\n");
                            turn_off_bt();
                            //BLE51_PowerState = 4;
                            disconnect_s = 0;
                        }
                    } 
                    #ifdef DEBUG_BLE51 
                    else {
                        xprintf("%s\n", result);
                        need_clear = 1;
                    }
                    #endif

                }
                /* Display connect status*/
                if (display_connection_status_check_times) {
                    if (++display_connection_status_check_times > 20) {
                        display_connection_status_check_times = 0;
                        // qmk 
                        led_wakeup();
                    } else {
                        //display_connection_status_check_times++;
                        if (bt_connected == 1) {
                            // After a 20s timeout or bt_connected, the next loop ends.
                            // So there is 1s to indicate that it is connected.
                            display_connection_status_check_times = 100;
                            // 重置check_connection_times，确保已连接指示有4s左右(闪三次)
                            check_connection_times = 0;
                        }
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

            if (!usb_connected && !display_connection_status_check_times && BLE51_PowerDozing_EN) {
                // count idle time
                kb_idle_times++;
                dprintf("idle times: %d, PowerState: %d\n", kb_idle_times, BLE51_PowerState);
                if (kb_idle_times >= (BT_POWER_OFF_TIME/1000)) {
                    // 暂时保留 BLE51_PowerState 4，HHKB BLE和BLE660C等有使用到
                    BLE51_PowerState = 4;
                    turn_off_bt();
                } else 
                // always set State to 2, when BT_POWERED and kb_idle_times for POWER_SAVE_TIME.
                if (kb_idle_times >= (BT_POWER_SAVE_TIME/1000)) {
                    BLE51_PowerState = 2;
                }
            }
        }

       
        /* ble set once
         *AT+GAPSETADVDATA=02-01-04/n  Discoverable OFF   KC_O
         *AT+GAPSETADVDATA=02-01-06/n  Discoverable ON    KC_I
         *AT+GAPDELBONDS\n             Pair Clear        KC_R
         */
        if (ble_set_code > 0) {
            //watchdog_off();
            if (ble_set_code == KC_I) {
                ble51_puts("AT+GAPSETADVDATA=02-01-0");
                ble51_cmd("6\n");
            } else if (ble_set_code == KC_O) {
                ble51_puts("AT+GAPSETADVDATA=02-01-0");
                ble51_cmd("4\n");
                //watchdog_on();
            } else if (ble_set_code == KC_R) {
                //ble51_set_connectable('0');
                //ble51_cmd("AT+GAPDISCONNECT\n");
                //ble51_cmd("AT+GATTCLEAR\n"); //clear battery service.
                ble51_del_bonds();
                ble51_set_blehiden('0');
                ble51_set_blehiden('1');
                //ble51_set_connectable('1');
                ble51_cmd("ATZ\n");
                ble51_puts("AT+GAPSETADVDATA=02-01-0");
                ble51_cmd("6\n");
            }
            ble_set_code = 0;
        }

        /* hold consumer key */
        ble51_consumer_task();
    }
}

void update_battery(uint8_t mode) {
    /* mode 0: not update bat_value, only get voltage and calc low_battery. */

    /*only update when light is off.*/
#ifdef BLE_LIGHT_ON
    if (BLE_LIGHT_ON && mode == 0) return;
#endif

#ifndef NO_DEBUG
    if (debug_config.enable == 0 && mode == 0) return;
#endif

    const char *result = ble51_cmd("AT+HWADC=6\n");
    /* simple calculate batt level 
     * Char "0"->"9" = 0x30->0x39 Char "O"=0x4F. atol() needs more space.
     * When no battery, ADC value is 32 and voltage measured by multimeter is 54mv.
     * So result[2] will be "_" not 0.
     */
#ifdef DEBUG_BLE51 
    if (result[1] < '0' || result[1] > '9') need_clear = 1;
    else if(result[2] > '9') print("No battery or wrong!\n"); 
    else {
#else
    // confirm the result is voltage
    if (result[0] >= '3' && result[2] >= '0' && result[2] <= '9') {
#endif
        //uint16_t battery_voltage_get = ((result[0]-48)*100 + (result[1]-48)*10 + result[2]- 48 -32) * 8 + 54;
        // adc - 330,  to unit8_t, max voltage = 4480
        uint8_t bat_adc_u8 = result[0]*100 + result[1]*10 + result[2]- 48*100 - 48*10 - 48 - 330;

        // Control the effective range of adc_u8, and update only within this range.
        if (bat_adc_u8 > U8V(3350)) {
            #ifndef BLE51_NO_BATTERY_VOLTAGE //save space
            uint16_t battery_voltage_get = bat_adc_u8 * 8 + (330-25)*8;
            battery_voltage = (battery_voltage_get + ((battery_voltage == 3699)?battery_voltage_get:battery_voltage)) / 2;
            dprintf(" BAT Volt: %dmv, ADC:%s\n", battery_voltage, result);
            battery_calc_u8 = U8V(battery_voltage);
            #else 
            battery_calc_u8 = bat_adc_u8;
            #endif
        #ifdef UPDATE_BATTERY_WHEN_CHARGING
            if (BATTERY_CHARGING) {
                is_charging = 1;
                //battery_voltage -= CHARGING_FIX_VALUE;
                //Directly testing the battery voltage during charging is not accurate.
                if (battery_calc_u8 < U8V(4208)) battery_calc_u8 -= ((CHARGING_FIX_VALUE+2) / 4);
                else battery_calc_u8 -= ((CHARGING_FIX_VALUE+4) / 8);
                dprintf(" CHG Calc: %dmv\n", battery_calc_u8*8+(330-25)*8);
            } else {
                is_charging = 0;
            }
        #endif
        #ifdef BLE51_NO_ULTRA_LOW_BATTERY
            low_battery = (battery_calc_u8 < U8V(3550))? 5:0;
        #else
            if (battery_calc_u8 > U8V(3600)) low_battery = 0;
            else if (battery_calc_u8 < U8V(3420)) low_battery = 1;  //Ultra low battery
            else if (battery_calc_u8 < U8V(3550)) low_battery = 5;
        #endif
        }
    }    


#ifdef BLE_BATTERY_SERVICE
    if (mode == 1) {
        /* Calculate battery level */
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
        battery_percentage = battery_level;
        update_battery_value();
    }
#endif
}


static void update_battery_value(void) {
    #ifdef BLE51_USE_BLEBATT // Two ways for battery service. Only use one of them.
    sprintf(ble51_buf, "AT+BLEBATTVAL=%d\n", battery_percentage);
    #else
    sprintf(ble51_buf, "AT+GATTCHAR=1,%d\n", battery_percentage);
    #endif
    ble51_cmd(ble51_buf);
    ble51_cmd(ble51_buf); // run it one more time to properly refresh on macOS.
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
#ifndef NOT_BLE
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
            // 1:   start to display
            // 100: end display and restore ledmapu. 
            display_connection_status_check_times = display_connection_status_check_times? 100:1;
            return true;    
        case KC_P:
            BLE51_PowerDozing_EN ^= 1;
            type_num(BLE51_PowerDozing_EN);
            return true;
        case KC_W:
#if !defined(HARDWARE_BT_SWITCH) && !defined(NO_BT_SWITCH)
            // EECONFIG_USER (uint32_t *)19， (uint8_t)0xBD mean off
            eeprom_update_byte(EECONFIG_USER, (pressed_mods&MOD_BIT(KC_LCTRL))?0xBD:1);
            for (;;);
#endif
        case KC_V:  // output battery and bt connection
            type_numbers(battery_percentage);
            #ifndef BLE51_NO_BATTERY_VOLTAGE
            //display voltage with LCtrl pressed. 30Byte
            if (pressed_mods & MOD_BIT(KC_LCTRL)) {
                tap_code(KC_MINS);
                type_numbers(battery_voltage);
            }
            #endif
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
#endif // USB only, less
        case KC_B:  // decide what (LCtrl+)LRShift+B by user action_function
            ble51_reset(pressed_mods);
            return true;
        case KC_N: // for hook;
            hook_nkro_change();
            //ble51_usb_nkro = !ble51_usb_nkro;
            #ifdef NO_DEFAULT_COMMAND
            keymap_config.nkro = !keymap_config.nkro;
            #endif
            return true;
        default:
            return false;   // yield to default command
    }
}
