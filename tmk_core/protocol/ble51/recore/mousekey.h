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

#ifndef MOUSEKEY_H
#define  MOUSEKEY_H

#include <stdbool.h>
#include "host.h"


#define MOUSEKEY_MOVE_REPEAT_MAX 20 // 4的倍数
#ifndef MOUSEKEY_REPEAT_UPDATE_INTERVAL
#define MOUSEKEY_REPEAT_UPDATE_INTERVAL 12 // for 8M
#endif


#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t mk_delay;
extern uint8_t mk_interval;
extern uint8_t mk_max_speed;
extern uint8_t mk_time_to_max;
extern uint8_t mk_wheel_max_speed;
extern uint8_t mk_wheel_time_to_max;

extern bool rapidfire_key[];
extern bool rapidfire_mode;

void mousekey_task(void);
void mousekey_on(uint8_t code);
void mousekey_off(uint8_t code);
void mousekey_clear(void);
void mousekey_send(void);

#ifdef __cplusplus
}
#endif

#endif
