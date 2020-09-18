/*
	Entry point
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

#include <RF24/RF24.h>

constexpr uint16_t CE_PIN = 0;
constexpr uint16_t CSN_PIN = 0;

__attribute__((externally_visible)) int main()
{
	RF24 rf24(CE_PIN, CSN_PIN);
	rf24.begin();
	rf24.openReadingPipe(0, 0x123456);

	while (true)
	{
		if (rf24.available())
		{
			static uint8_t buffer[32];
			rf24.read(buffer, sizeof(buffer));
		}
	}
}
