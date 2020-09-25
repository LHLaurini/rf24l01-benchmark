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
#include "../configpayload.h"

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
	rf24.setChannel(CONFIG_CHANNEL);
	rf24.setAutoAck(true);
	rf24.setCRCLength(RF24_CRC_8);

	configureTransmitter(0);

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

void Benchmark::configureReceiver(int testNum)
{
	log("Configuring receiver...");

	ConfigPayload payload = generateConfigPayload(configFromIndex(testNum));

	int i;
	for (i = 0; i < 10; i++)
	{
		if (rf24.write(&payload, sizeof payload))
		{
			log("Sent");
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

void Benchmark::configureTransmitter(int config)
{
	log("Configuring transmitter...");

	currentConfig = configFromIndex(config);
	rf24.setRetries(currentConfig.retryDelay, currentConfig.retryCount);
	rf24.setPayloadSize(currentConfig.payloadSize);
	rf24.setPALevel(static_cast<rf24_pa_dbm_e>(currentConfig.power));
	rf24.setDataRate(static_cast<rf24_datarate_e>(currentConfig.bitrate));

	rf24.openWritingPipe(CONFIG_ADDRESS);

	if (config != 0)
	{
		std::vector<char> whatever(currentConfig.payloadSize);

		log("Confirming receiver is accesssible...");

		int i;
		for (i = 0; i < 10; i++)
		{
			if (rf24.write(whatever.data(), whatever.size()))
			{
				log("OK.");
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
}

#define CONFIG_PAYLOAD(x)                                                                                              \
	config.delayms = CONFIG_##x##_DELAYMS;                                                                             \
	config.payloadSize = CONFIG_##x##_PAYLOAD_SIZE;                                                                    \
	config.bitrate = CONFIG_##x##_BITRATE;                                                                             \
	config.power = CONFIG_##x##_POWER;                                                                                 \
	config.retryDelay = CONFIG_##x##_RETRY_DELAY;                                                                      \
	config.retryCount = CONFIG_##x##_RETRY_COUNT

Config Benchmark::configFromIndex(int configIndex)
{
	Config config;

	switch (configIndex)
	{
	case 0:
		CONFIG_PAYLOAD(INITIAL);
		break;

	case 1:
		CONFIG_PAYLOAD(TEST1);
		break;

	case 2:
		CONFIG_PAYLOAD(TEST2);
		break;

	case 3:
		CONFIG_PAYLOAD(TEST3);
		break;

	case 4:
		CONFIG_PAYLOAD(TEST4);
		break;

	default:
		throw std::invalid_argument("config must be within [0;4]");
	}

	return config;
}

#undef CONFIG_PAYLOAD

ConfigPayload Benchmark::generateConfigPayload(const Config &config)
{
	ConfigPayload configPayload;
	configPayload.payloadSize = config.payloadSize - 1;
	configPayload.bitrate = config.bitrate;
	configPayload.power = config.power;
	configPayload.retryDelay = config.retryDelay;
	configPayload.retryCount = config.retryCount;
	return configPayload;
}

void Benchmark::log(const std::string &msg) const
{
	if (logFn)
	{
		logFn(msg);
	}
}

std::vector<PayloadDetails> Benchmark::run(int testNum)
{
	configureReceiver(testNum);
	configureTransmitter(testNum);

	log("Starting test...");
	std::this_thread::sleep_for(1s);

	std::vector<PayloadDetails> details;

	auto stopTime = std::chrono::steady_clock::now() + TARGET_TIME;

	auto notFinished = [&]() -> bool { return std::chrono::steady_clock::now() > stopTime; };

	auto randomBytes = std::as_bytes(std::span(randomData));
	details.clear();

	unsigned long i;

	if (currentConfig.delayms > 0)
	{
		for (i = 0; notFinished(); i += currentConfig.payloadSize)
		{
			unsigned retries = 0;
			auto data = randomBytes.subspan(i, currentConfig.payloadSize);

			while (notFinished())
			{
				if (rf24.write(data.data(), data.size()))
				{
					break;
				}
				else
				{
					retries += currentConfig.retryCount;
				}
			}

			details.emplace_back(true, std::chrono::high_resolution_clock::now(), retries + rf24.getARC());
			std::this_thread::sleep_for(std::chrono::milliseconds(currentConfig.delayms));
		}
	}
	else
	{
		for (i = 0; notFinished(); i += currentConfig.payloadSize * 3)
		{
			auto data = randomBytes.subspan(i, currentConfig.payloadSize * 3);

			auto data1 = data.subspan(0 * currentConfig.payloadSize, currentConfig.payloadSize);
			auto data2 = data.subspan(1 * currentConfig.payloadSize, currentConfig.payloadSize);
			auto data3 = data.subspan(2 * currentConfig.payloadSize, currentConfig.payloadSize);

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
				retries = currentConfig.retryCount;
			}

			// Info is about 3 payloads, in this case
			details.emplace_back(ok, std::chrono::high_resolution_clock::now(), retries);
		}
	}

	dataSent = i;

	return details;
}
