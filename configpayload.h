/*
	Config payload definition
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

#include <RF24/RF24.h>

// Not worth the hassle to make a bit field (3 vs 5 bytes)
// Also, bit field seemed inconsistent between ARM Clang and AVR GCC
struct ConfigPayload
{
	uint8_t payloadSize;
	uint8_t bitrate;
	uint8_t power;
	uint8_t retryDelay;
	uint8_t retryCount;
};
