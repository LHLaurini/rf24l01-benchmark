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
	PayloadDetails(auto time, auto arc) : time(time), arc(arc)
	{
	}
	std::chrono::high_resolution_clock::time_point time;
	std::uint8_t arc;
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

	std::atomic_flag running;
	std::atomic_flag noError;
	running.test_and_set();
	noError.test_and_set();

	std::thread helper([&]() {
		auto stopTime = std::chrono::steady_clock::now() + TARGET_TIME;
		while (noError.test_and_set())
		{
			auto now = std::chrono::steady_clock::now();

			if (now > stopTime)
			{
				running.clear();
				break;
			}

			std::this_thread::sleep_for(1s);
		}
	});

	auto randomBytes = std::as_bytes(std::span(randomData));
	bool failed = false;
	std::vector<PayloadDetails> details;

	for (auto i = 0ULL; running.test_and_set(); i += CONFIG_PAYLOAD_SIZE)
	{
		if (!rf24.writeBlocking(randomBytes.subspan(i).data(), CONFIG_PAYLOAD_SIZE, 10000))
		{
			failed = true;
			noError.clear();
			break;
		}

		details.emplace_back(std::chrono::high_resolution_clock::now(), rf24.getARC());
	}

	std::cout << "failed = " << std::boolalpha << failed << "\n";
	std::cout << "sent ~" << details.size() << "/" << randomBytes.size() << " bytes\n";

	auto lastTime = details.at(0).time;

	auto i = 0ULL;
	for (auto &detail : details)
	{
		std::cout << i++ << " " << std::chrono::duration_cast<std::chrono::nanoseconds>(detail.time - lastTime).count()
				  << " " << static_cast<unsigned int>(detail.arc) << "\n";
		lastTime = detail.time;
	}

	helper.join();

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
