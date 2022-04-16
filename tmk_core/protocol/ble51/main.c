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
#include "ble51_task.h"


void protocol_post_task(void);

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
int main(void)
{
    SetupHardware();
    sei();

    /* wait for USB startup to get ready for debug output */
    uint8_t timeout = 255;
    while (timeout-- && USB_DeviceState != DEVICE_STATE_Configured) {
        _delay_ms(6);
#if !defined(INTERRUPT_CONTROL_ENDPOINT)
        USB_USBTask();
#endif
    }
    print("\nUSB init\n");

    if (ble51_boot_on) {
        ble51_init();
        ble51_task_init();
    } 
    /* init modules */
    keyboard_init();

    print("Keyboard start\n");


    watchdog_on();
    while (1) {
        if (BLE51_PowerState < 4){
            wdt_reset();
            if (BLE51_PowerState <= 1) {
                keyboard_task();
                protocol_post_task();
            }
            ble51_task(); 
        }
        if (BLE51_PowerState > 1 || (!ble51_boot_on && (USB_DeviceState != DEVICE_STATE_Configured))) { //power down
            suspend_power_down();
            watchdog_on();
            if (suspend_wakeup_condition()) {
                kb_idle_times = 0; 
                suspend_wakeup_init_action();
                BLE51_PowerState = 1;
                if (ble51_boot_on) { 
                    turn_on_bt();
                }
                else if (USB_DeviceState == DEVICE_STATE_Suspended) USB_Device_SendRemoteWakeup();
            }
        }
    }
}

