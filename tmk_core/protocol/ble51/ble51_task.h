#ifndef BLE51_TASK_H
#define BLE51_TASK_H

#include <stdbool.h>
#include "ble51.h"

extern uint8_t BLE51_PowerState;  //no_sleep 0, work 1,dozing 2, check battery once when dozing 3, sleeping 4,  lock mode 10
extern uint8_t BLE51_PowerDozing_EN;  //
extern bool ble51_boot_on;
extern uint16_t kb_idle_times;
extern uint8_t display_connection_status_check_times;
extern bool bt_connected;
extern uint8_t usb_connected;
extern bool usb_connected_once;
extern uint8_t low_battery;
extern uint16_t battery_voltage;
extern uint8_t ble51_usb_nkro;

void usb_bt_state_task(void);
void ble51_task_init(void);
void ble51_task(void);
void ble51_task_user(void);
void update_battery(uint8_t mode);
extern uint16_t ble_reset_key;

#endif
