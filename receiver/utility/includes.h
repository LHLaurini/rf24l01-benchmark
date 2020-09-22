/*
	Declare functions used by RF24
	Copyright (C) 2020  Luiz Henrique Laurini

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "../spi.hxx"

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MINIMAL 1
#define IF_SERIAL_DEBUG(x)

constexpr bool LOW = false;
constexpr bool HIGH = true;
constexpr bool OUTPUT = true;

void delayMicroseconds(int micro);
void delay(int milli);
void digitalWrite(int pin, bool value);
void pinMode(int pin, bool output);
int millis();
