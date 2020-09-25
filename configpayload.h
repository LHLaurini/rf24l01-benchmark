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

// Hope this doesn't explode
struct __attribute__((packed)) ConfigPayload
{
	unsigned delayms : 15;
	unsigned payloadSize : 5;
	rf24_datarate_e bitrate : 2;
	rf24_pa_dbm_e power : 2;
	unsigned retryDelay : 4;
	unsigned retryCount : 4;
};

static_assert(sizeof(ConfigPayload) == 4, "unexpected ConfigPayload size");
