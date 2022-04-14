#ifndef BLE51_TASK_H
#define BLE51_TASK_H

#include <stdbool.h>
#include "ble51.h"

extern uint8_t BLE51_PowerState; 
extern bool ble51_boot_on;
extern uint16_t kb_idle_times;
extern uint8_t display_connection_status_check_times;
extern bool bt_connected;
extern uint8_t usb_connected;
extern uint8_t low_battery;
extern uint16_t battery_voltage;

void ble51_task_init(void);
void ble51_task(void);
void ble51_task_user(void);
extern uint16_t ble_reset_key;

#endif
