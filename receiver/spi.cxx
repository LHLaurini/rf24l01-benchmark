/*
    Spi class
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

#include "spi.hxx"

#include <avr/io.h>

Spi _SPI;

namespace {
constexpr auto DI_DD = DDB0;
constexpr auto DO_DD = DDB1;
constexpr auto USCK_DD = DDB2;
}

void Spi::begin()
{
    DDRB = (DDRB & ~(1 << DI_DD)) | (1 << DO_DD) | (1 << USCK_DD);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

uint8_t Spi::transfer(uint8_t byte)
{

#pragma GCC diagnostic pop

    USIDR = byte;

    for (auto i = 0U; i < 8U; i++) {
        auto common = (1 << USIWM0) | (0 << USICS1) | (0 << USICS0);
        USICR = common | (1 << USITC);
        USICR = common | (1 << USITC) | (1 << USICLK);
    }

    return USIDR;
}
