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

#include "../config.h"
#include "../configpayload.h"
#include <RF24/RF24.h>

constexpr uint16_t CE_PIN = CONFIG_RECEIVER_CE_PIN;
constexpr uint16_t CSN_PIN = CONFIG_RECEIVER_CSN_PIN;

int main()
{
	RF24 rf24(CE_PIN, CSN_PIN);
	rf24.begin();
	rf24.setAddressWidth(CONFIG_ADDRESS_WIDTH);
	rf24.setChannel(CONFIG_CHANNEL);
	rf24.setAutoAck(true);
	rf24.setCRCLength(RF24_CRC_8);

	rf24.setRetries(CONFIG_INITIAL_RETRY_DELAY, CONFIG_INITIAL_RETRY_COUNT);
	rf24.setPayloadSize(CONFIG_INITIAL_PAYLOAD_SIZE);
	rf24.setPALevel(CONFIG_INITIAL_POWER);
	rf24.setDataRate(CONFIG_INITIAL_BITRATE);

	rf24.openReadingPipe(0, CONFIG_ADDRESS);
	rf24.startListening();

	uint8_t payloadSize;

	while (true)
	{
		if (rf24.available())
		{
			ConfigPayload config;
			rf24.read(&config, sizeof config);

			payloadSize = config.payloadSize + 1;

			rf24.setRetries(config.retryDelay, config.retryCount);
			rf24.setPayloadSize(payloadSize);
			rf24.setPALevel(config.power);
			rf24.setDataRate(config.bitrate);
		}
	}

	while (true)
	{
		if (rf24.available())
		{
			static uint8_t buffer[32];
			rf24.read(buffer, payloadSize);
		}
	}
}
