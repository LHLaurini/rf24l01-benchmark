/*
	Benchmark class definition
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

#include "benchmark.hxx"
#include "../config.h"

#include <random>
#include <span>
#include <thread>

using namespace std::chrono_literals;

constexpr std::uint16_t CE_PIN = CONFIG_TRANSMITTER_CE_PIN;
constexpr auto TARGET_TIME = CONFIG_TARGET_TIME;
constexpr auto TARGET_TIME_SECS = std::chrono::duration_cast<std::chrono::seconds>(TARGET_TIME).count();
constexpr auto RANDOM_BUFFER_SIZE = 2 * TARGET_TIME_SECS * 250000 / sizeof(std::uint64_t);

Benchmark::Benchmark() : Benchmark(nullptr)
{
}

Benchmark::Benchmark(LogFn logFn) : logFn(logFn), rf24(CE_PIN, 0)
{
	log("Generating random data...");
	randomData = generateRandomData();
	log("Initializing transmitter...");
	initializeRF24();
}

void Benchmark::initializeRF24()
{
	if (!rf24.begin())
	{
		throw std::runtime_error("bad response from nRF24L01+");
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
}

std::vector<std::uint64_t> Benchmark::generateRandomData() const
{
	std::random_device device;
	std::mt19937_64 engine(device());
	std::uniform_int_distribution<std::uint64_t> distribution;

	std::vector<std::uint64_t> randomData(RANDOM_BUFFER_SIZE);
	std::generate(randomData.begin(), randomData.end(), std::bind(distribution, engine));
	return randomData;
}

// Will make sense in the next commit
void Benchmark::configureReceiver()
{
	log("Configuring receiver...");
	std::array<char, CONFIG_PAYLOAD_SIZE> whatever;

	int i;
	for (i = 0; i < 10; i++)
	{
		if (rf24.write(whatever.data(), whatever.size()))
		{
			log("OK! Starting test.");
			break;
		}
		else
		{
			log("Failed. Trying again...");
			std::this_thread::sleep_for(1s);
		}
	}

	if (i == 10)
	{
		throw std::runtime_error("failed to reach receiver");
	}
}

void Benchmark::log(const std::string &msg) const
{
	if (logFn)
	{
		logFn(msg);
	}
}

std::vector<PayloadDetails> Benchmark::run()
{
	configureReceiver();

	std::vector<PayloadDetails> details;

	auto stopTime = std::chrono::steady_clock::now() + TARGET_TIME;

	auto notFinished = [&]() -> bool { return std::chrono::steady_clock::now() > stopTime; };

	auto randomBytes = std::as_bytes(std::span(randomData));
	details.clear();

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

	dataSent = i;

	return details;
}
