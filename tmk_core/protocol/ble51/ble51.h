#ifndef BLE51_H
#define BLE51_H

#include <stdbool.h>
#include "host_driver.h"
#include "ble51_task.h"
#include "recore.h"

host_driver_t ble51_driver;

void ble51_init(void);
int16_t ble51_getc(void);
const char *ble51_gets(void);
void ble51_putc(uint8_t c);
void ble51_puts(char *s);
void ble51_consumer_task(void);
void ble51_led_set(uint8_t key_led);
const char *ble51_cmd(char *s);
void ble51_init_cmd(char *s);

void ble51_clear_recv2(void);
void ble51_clear_keys(void);
void ble51_stop_sending_end_action(void);

void ble51_init_blename(void);

void ble51_factory_reset(void);
//void ble51_set_connectable(uint8_t mode);
void ble51_set_blehiden(uint8_t mode);
void ble51_set_blename(char *s);
uint8_t ble51_get_connection_status(void);
void ble51_del_bonds(void);

extern uint8_t ble51_stop_sending;
//Confirm the result when all keys pop up
extern uint8_t g_u8_kbd_report_clear_failed_count;

#endif
