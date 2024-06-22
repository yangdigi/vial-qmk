/*
Copyright 2020 YANG <drk@live.com>

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
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "backlight.h"
#include "timer.h"


void backlight_init_ports(void) {
    DDRC  &= ~(1<<6);
    PORTC |=  (1<<6);
}

void backlight_task(void) {
    return;
}

void backlight_user_enable(void)
{

    DDRC  |=  (1<<6);
    PORTC &= ~(1<<6);
}

void backlight_user_disable(void)
{
    DDRC  &= ~(1<<6);
    PORTC |=  (1<<6);
}

void backlight_set(uint8_t level)
{
    if (level) backlight_user_enable();
    else backlight_user_disable();
}

