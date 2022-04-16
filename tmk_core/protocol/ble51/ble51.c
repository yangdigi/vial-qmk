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
#include <avr/wdt.h>
#include "host.h"
#include "host_driver.h"
#include "ble51.h"
#include "ble51_task.h"
#include "recore.h"
#include "debug.h"
#include "timer.h"
#include "wait.h"

#ifndef BLE51_CONSUMER_ON_DELAY
#define BLE51_CONSUMER_ON_DELAY 633
#endif 

/* Host driver */
static uint8_t keyboard_leds(void);
static void send_keyboard(report_keyboard_t *report);
static void send_mouse(report_mouse_t *report);
static void send_system(uint16_t data);
static void send_consumer(uint16_t data);

host_driver_t ble51_driver = {
    keyboard_leds,
    send_keyboard,
    send_mouse,
    send_system,
    send_consumer
};

uint8_t ble51_stop_sending = 0;
bool report_error = false;
static uint16_t consumer_ble51_on = 0;
static uint16_t consumer_ble51_data;
char ble51_buf[30];

static inline void serial_init_1st(void)
{
    cli();
    UBRR1L = ((F_CPU/(8.0*9600)-1+0.5));       /* baud rate 9600*/ 
    UBRR1H = 103>>8;  /* baud rate */ 
    UCSR1B |= (1<<RXCIE1) | (1<<RXEN1); /* RX interrupt, RX: enable */
    UCSR1B |= (0<<TXCIE1) | (1<<TXEN1); /* TX interrupt, TX: enable */ 
    UCSR1C |= (0<<UPM11) | (0<<UPM10);  /* parity: none(00), even(01), odd(11) */
    UCSR1A |= (1<<U2X1); /* 2x speed */
    sei(); 
}

static inline void serial_init_kbd(void)
{
    cli();
    UBRR1L = ((F_CPU/(8.0*76800)-1+0.5));       /* baud rate 76800*/ 
    sei(); 
}

const char *ble51_cmd(char *s) {
    ble51_puts(s);
    const char *result = ble51_gets(TIMEOUT);
    dprintf("cmd: %s R: %s\n", s, result);
    return result;
}

void ble51_init_cmd(char *s) {
    _delay_ms(300);
    ble51_cmd(s);
}

__attribute__ ((weak))
void ble51_init_blename(void){
#ifdef BLE_NAME
    ble51_set_blename(BLE_NAME);
#endif
}

void ble51_init(void)
{
    bt_power_init();

#if defined(BLE_NAME) || defined(BLE_NAME_VARIABLE) //ble51 init
    timer_init();
    serial_init_1st();
    ble51_init_cmd("AT\n");
    print("link test1\n");
    const char *result = ble51_cmd("AT\n"); 
    xprintf("%s\n", result);
    if ((result[0] == 'A' && result[1] == 'T') || (result[0] == 'O' && result[1] == 'K')) {
        print("51 init start\n");

        ble51_factory_reset();
        print("link\n");
        ble51_init_cmd("AT\n");
        ble51_init_cmd("ATE=0\n");
        ble51_init_blename();
        ble51_set_blehiden('1');
        ble51_init_cmd("AT+GAPSTARTADV\n");
        ble51_del_bonds();
#ifdef BLE_BATTERY_SERVICE
        print("Battery Service\n");
        ble51_init_cmd("AT+GATTADDSERVICE=UUID=6159\n");
        ble51_init_cmd("AT+GATTADDCHAR=UUID=0x2A19,PROPERTIES=16,MIN_LEN=1,VALUE=9\n");
#endif
        ble51_init_cmd("AT+BAUDRATE=76800\n");
        print("BAUDRATE\n");
        bootloader_jump(); 
    }
    serial_init_kbd();
#else
    serial_init();
#endif

    print("Serial Init\n");
#if defined(BLE_NAME) || defined(BLE_NAME_VARIABLE)
    ble51_init_cmd("AT\n");
    print("Link Start\n");
#endif
}

int16_t ble51_getc(void)
{
    return serial_recv2();
}

const char *ble51_gets(uint16_t timeout)
{
    static char s[24];
    uint16_t t = timer_read();
    uint8_t i = 0;
    int16_t c;
    while (i < 23 && timer_elapsed(t) < timeout) {
        if ((c = ble51_getc()) != -1) {
            if ((char)c == '\r') continue;
            if ((char)c == '\n') {
                if (i >= 2 && s[i-2] == 'O') break;
                else c = '_';
            }
            s[i++] = c;
        }
    }
    s[i] = '\0';
    return s;
}

void ble51_putc(uint8_t c)
{
    serial_send(c);
}

void ble51_puts(char *s)
{
    while (*s)
    serial_send(*s++);
}


static uint8_t leds = 0;
static uint8_t keyboard_leds(void) { return leds; }
void ble51_set_leds(uint8_t l) { leds = l; }

void ble51_clear_keys(void){
    send_keyboard(0);
}

static void send_keyboard(report_keyboard_t *report)
{
    if (ble51_stop_sending) return;
    if (bt_connected && !report_error) {
        ble51_puts("AT+BLEKEYBOARDCODE=");
        if (report == 0) {
            ble51_cmd("00-00\n");
        } else {
            sprintf(ble51_buf, "%02X-00-%02X-%02X-%02X-%02X-%02X-%02X\n",
                report->mods,
                report->keys[0],
                report->keys[1],
                report->keys[2],
                report->keys[3],
                report->keys[4],
                report->keys[5]);
            dprint("sending keyboard...");
            if (memcmp(ble51_cmd(ble51_buf), "ERROR", 5) == 0) {
                report_error = 1;
            } else {
                report_error = 0;
                dprint("end\n");
            }
#ifndef NO_BLE51_LED
            uint8_t key = report->keys[0];
            if (report->mods == 0 ) {
                uint8_t key_led = 0;
                if (key == KC_NLCK) key_led = 16;
                else if (key == KC_CAPS) key_led = 17;
                else if (key == KC_SLCK) key_led = 18;
                if (key_led) ble51_led_set(key_led);
            }
#endif
        }
    } 
}

__attribute__((weak))
void ble51_led_set(uint8_t key_led){}

static void send_mouse(report_mouse_t *report)
{
    if (!report_error) {
        dprint("sending mouse move...");
        sprintf(ble51_buf, "AT+BLEHIDMOUSEMOVE=%d,%d,%d,%d\n",
            report->x,
            report->y,
            report->v,
            report->h);
        if (memcmp(ble51_cmd(ble51_buf), "OK", 2) == 0) {
            report_error = 0;
            dprint("end\n");
        }
        else {
            report_error = 1;
        }
    } 

#ifndef MOUSEBUTTON_NO_CHECK
    static uint8_t mouse_button_prev = 0;  //send report->buttons only if it changes. Cost 13B.
    if (!report_error && (report->buttons != mouse_button_prev)) {
        mouse_button_prev = report->buttons;
#else
    if (!report_error) {
#endif
        dprint("sending mouse button...");
        //sprintf(ble51_buf, "AT+BLEHIDMOUSEBUTTON=%d\n", report->buttons);
        sprintf(ble51_buf+strlen("AT+BLEHIDMOUSE"), "BUTTON=%d\n", report->buttons);
        if (memcmp(ble51_cmd(ble51_buf), "OK", 2) == 0) {
            report_error = 0;
            dprint("end\n");
        }
        else {
            report_error = 1;
        }
    } 
}

static void send_system(uint16_t data)
{
    // system data: 0x81 power 0x82 sleep 0x83 wake
}

static void send_consumer(uint16_t data)
{
    if (data == BRIGHTNESS_UP || data == BRIGHTNESS_DOWN || data == AUDIO_VOL_UP || data == AUDIO_VOL_DOWN) {  //0x6F, 0x70, 0xE9, 0xEA
        if (!consumer_ble51_on) consumer_ble51_on = BLE51_CONSUMER_ON_DELAY;
        consumer_ble51_data = data;
    }
    else consumer_ble51_on = 0;
    if (!report_error) {
        dprintf("sending consumer...%X...", data);
        sprintf(ble51_buf, "AT+BLEHIDCONTROLKEY=%d\n",data);
        if (memcmp(ble51_cmd(ble51_buf), "OK", 2) == 0) {
            report_error = 0;
            dprint("end\n");
        } else {
            report_error = 1;
        }
    }    
}

void ble51_consumer_task(void)
{
    if (consumer_ble51_on) {
        if (consumer_ble51_on > 1 ) {
            consumer_ble51_on--;
        } else {
            static uint8_t sending_consumer_step = 0;  //controll repeat speed to be similar to USB 
            if (++sending_consumer_step >= (0xF3 - consumer_ble51_data)) {
                sending_consumer_step = 0;
                send_consumer(consumer_ble51_data);
            }
        }
    }
}

void ble51_factory_reset(void)
{
    ble51_init_cmd("AT+FACTORYRESET\n");
}

#if 0 //not used yet
void ble51_set_connectable(uint8_t mode)
{
    ble51_puts("AT+GAPCONNECTABLE=");
    ble51_putc(mode);
    ble51_cmd("\n");
}
#endif

void ble51_set_blehiden(uint8_t mode)
{
    if (mode == '0') ble51_init_cmd("AT+BLEHIDEN=0\n");
    else ble51_init_cmd("AT+BLEHIDEN=1\n");
}

void ble51_set_blename(char *s)
{
    ble51_puts("AT+GAPDEVNAME=");
    ble51_puts(s);
    ble51_init_cmd("\n");
}

uint8_t ble51_get_connection_status(void)
{
    const char *result = ble51_cmd("AT+GAPGETCONN\n");
    return result[0];
}

void ble51_del_bonds(void)
{
    ble51_init_cmd("AT+GAPDELBONDS\n");
}
