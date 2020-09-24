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
#include <RF24/RF24.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <span>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

constexpr std::uint16_t CE_PIN = CONFIG_TRANSMITTER_CE_PIN;
constexpr auto TARGET_TIME = CONFIG_TARGET_TIME;
constexpr auto TARGET_TIME_SECS = std::chrono::duration_cast<std::chrono::seconds>(TARGET_TIME).count();
constexpr auto RANDOM_BUFFER_SIZE = 2 * TARGET_TIME_SECS * 250000 / sizeof(std::uint64_t);

struct PayloadDetails
{
	PayloadDetails(auto ok, auto time, auto retries) : ok(ok), time(time), retries(retries)
	{
	}
	bool ok;
	std::chrono::high_resolution_clock::time_point time;
	unsigned retries;
};

int pmain()
{
	std::random_device device;
	std::mt19937_64 engine(device());
	std::uniform_int_distribution<std::uint64_t> distribution;

	std::vector<std::uint64_t> randomData(RANDOM_BUFFER_SIZE);
	std::generate(randomData.begin(), randomData.end(), std::bind(distribution, engine));

	RF24 rf24(CE_PIN, 0);
	if (!rf24.begin())
	{
		std::cerr << "Error: bad response from nRF24L01+." << std::endl;
		return 1;
	}

	rf24.setAddressWidth(CONFIG_ADDRESS_WIDTH);
	rf24.setRetries(CONFIG_RETRY_DELAY, CONFIG_RETRY_COUNT);
	rf24.setChannel(CONFIG_CHANNEL);
	rf24.setPayloadSize(CONFIG_PAYLOAD_SIZE);
	rf24.setAutoAck(true);
	rf24.setPALevel(CONFIG_POWER);
	rf24.setDataRate(CONFIG_BITRATE);
	rf24.setCRCLength(RF24_CRC_8);
	rf24.openWritingPipe(CONFIG_ADDRESS);
	rf24.stopListening();

	{
		std::clog << "Checking if receiver is reachable..." << std::endl;
		std::array<char, CONFIG_PAYLOAD_SIZE> whatever;

		int i;
		for (i = 0; i < 10; i++)
		{
			if (rf24.write(whatever.data(), whatever.size()))
			{
				std::clog << "OK! Starting test." << std::endl;
				break;
			}
			else
			{
				std::clog << "Failed. Trying again..." << std::endl;
				std::this_thread::sleep_for(1s);
			}
		}

		if (i == 10)
		{
			return 1;
		}
	}

	auto stopTime = std::chrono::steady_clock::now() + TARGET_TIME;

	auto notFinished = [&]() -> bool { return std::chrono::steady_clock::now() > stopTime; };

	auto randomBytes = std::as_bytes(std::span(randomData));
	std::vector<PayloadDetails> details;

	{
		unsigned long i;

		if constexpr (CONFIG_DELAYMS > 0)
		{
			for (i = 0; notFinished(); i += CONFIG_PAYLOAD_SIZE)
			{
				unsigned retries = 0;
				auto data = randomBytes.subspan(i, CONFIG_PAYLOAD_SIZE);

				while (notFinished())
				{
					if (rf24.write(data.data(), data.size()))
					{
						break;
					}
					else
					{
						retries += CONFIG_RETRY_COUNT;
					}
				}

				details.emplace_back(true, std::chrono::high_resolution_clock::now(), retries + rf24.getARC());
				std::this_thread::sleep_for(std::chrono::milliseconds(CONFIG_DELAYMS));
			}
		}
		else
		{
			for (i = 0; notFinished(); i += CONFIG_PAYLOAD_SIZE * 3)
			{
				auto data = randomBytes.subspan(i, CONFIG_PAYLOAD_SIZE * 3);

				auto data1 = data.subspan(0 * CONFIG_PAYLOAD_SIZE, CONFIG_PAYLOAD_SIZE);
				auto data2 = data.subspan(1 * CONFIG_PAYLOAD_SIZE, CONFIG_PAYLOAD_SIZE);
				auto data3 = data.subspan(2 * CONFIG_PAYLOAD_SIZE, CONFIG_PAYLOAD_SIZE);

				rf24.writeFast(data1.data(), data1.size());
				rf24.writeFast(data2.data(), data2.size());
				rf24.writeFast(data3.data(), data3.size());

				unsigned retries;
				bool ok = rf24.txStandBy();
				if (ok)
				{
					retries = rf24.getARC();
				}
				else
				{
					retries = CONFIG_RETRY_COUNT;
				}

				// Info is about 3 payloads, in this case
				details.emplace_back(ok, std::chrono::high_resolution_clock::now(), retries);
			}
		}

		std::cout << "sent ~" << i << "/" << randomBytes.size() << " bytes\n";
	}

	auto lastTime = details.at(0).time;

	auto i = 0ULL;
	for (auto &detail : details)
	{
		std::cout << i++ << " " << std::boolalpha << detail.ok << " "
				  << std::chrono::duration_cast<std::chrono::nanoseconds>(detail.time - lastTime).count() << " "
				  << detail.retries << "\n";
		lastTime = detail.time;
	}

	return 0;
}

int main()
{
	try
	{
		return pmain();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}
	catch (...)
	{
		std::cerr << "Exception: unknown" << std::endl;
		return 1;
	}
}
