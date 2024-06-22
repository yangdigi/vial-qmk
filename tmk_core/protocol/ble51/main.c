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

#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include "lufa.h"
#include "print.h"
#include "sendchar.h"
#include "keyboard.h"
#include "keycode.h"
#include "action_util.h"
#include "wait.h"
#include "timer.h"
#include "suspend.h"
#include "ble51.h"

void protocol_post_task(void);

/* hooks */
__attribute__((weak))
void hook_early_init(void) {}

static int8_t sendchar_func(uint8_t c)
{
    //xmit(c);        // SUART
    sendchar(c);    // LUFA
    return 0;
}

static void SetupHardware(void)
{
    /* Disable clock division */
    clock_prescale_set(clock_div_1);

    hook_early_init();

    USB_Init();

#ifdef CONSOLE_ENABLE
    // for Console_Task
    USB_Device_EnableSOFEvents();
    print_set_sendchar(sendchar_func);
#endif
}

int main(void)  __attribute__ ((weak));

#ifndef NOT_BLE // BLE Ver

int main(void)
{
    SetupHardware();
    sei();
#if !defined(HARDWARE_BT_SWITCH) && !defined(NO_BT_SWITCH)
    if (eeprom_read_byte(EECONFIG_USER) == 0xBD) ble51_boot_on = 0;
#endif

    // BLE Power on
    if (ble51_boot_on) {
        ble51_init();
        ble51_task_init();
    }

#if 1 //def CONSOLE_ENABLE
    /* wait for USB startup to get ready for debug output */
    uint8_t timeout = 255;
    while (timeout-- && USB_DeviceState != DEVICE_STATE_Configured) {
         WAIT_MS(12); // wait for USB about 3s
#ifdef PS2_USE_INT  // BLE980M may need more time.
         WAIT_MS(16);
#endif
#if !defined(INTERRUPT_CONTROL_ENDPOINT)
        USB_USBTask();
#endif
    }
    print("\nUSB init\n");
#endif
    /* init modules */
    keyboard_init();

    print("Keyboard start\n");

#ifdef DEFAULT_6KRO
    keymap_config.nkro = 0;
#else
    keymap_config.nkro = 1; // default force nkro on for ble series
#endif

    watchdog_on();
    while (1) {
        //if (BT_POWERED){
        usb_bt_state_task();
        if (BLE51_PowerState < 10){
            wdt_reset();
            if (BLE51_PowerState <= 1) {
                keyboard_task();
                protocol_post_task();
                if (ble51_boot_on) ble51_task_user();
            }
            ble51_task(); 
        }
        if (BLE51_PowerState > 1 || (!ble51_boot_on && (USB_DeviceState != DEVICE_STATE_Configured))) { //power down
            suspend_power_down();
            watchdog_on();
            if (suspend_wakeup_condition()) {
                kb_idle_times = 0; 
                if (ble51_boot_on) {
                    BLE51_PowerState = 1;
                    if (BT_POWERED) {
                        update_battery(1);
                    } else {
                        display_connection_status_check_times = 1;
                        turn_on_bt();
                    }
                }
                suspend_wakeup_init_action();
                if (usb_connected_once && USB_DeviceState == DEVICE_STATE_Suspended) {
                    USB_Device_SendRemoteWakeup();
                }
            }
        }
    }
}

#else // Not BLE
int main(void)
{
    SetupHardware();
    sei();

    /* wait for USB startup & debug output */
    while (USB_DeviceState != DEVICE_STATE_Configured) {
#if !defined(INTERRUPT_CONTROL_ENDPOINT)
        USB_USBTask();
#endif
    }
    
    print("\nUSB init\n");
    /* init modules */
    keyboard_init();
    host_set_driver(&lufa_driver);
    usb_connected = 1;
    print("Keyboard start\n");
    watchdog_on();
    while (1) {
        // usb suspend
        while (USB_DeviceState == DEVICE_STATE_Suspended) {
            suspend_power_down();
            watchdog_on();
            if (USB_Device_RemoteWakeupEnabled && suspend_wakeup_condition()) {
                USB_Device_SendRemoteWakeup();
                //suspend_wakeup_init_action();
                for (;;); //restart keyboard
            }
        }
        wdt_reset();
        keyboard_task();
        protocol_post_task(); // for via?
    }
}

#endif