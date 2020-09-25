/*
	Benchmark class declaration
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

#include "../configpayload.h"

#include <RF24/RF24.h>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

struct PayloadDetails
{
	PayloadDetails(auto ok, auto time, auto retries) : ok(ok), time(time), retries(retries)
	{
	}
	bool ok;
	std::chrono::high_resolution_clock::time_point time;
	unsigned retries;
};

class Benchmark
{
  public:
	typedef std::function<void(const std::string &)> LogFn;

	Benchmark();
	Benchmark(LogFn logFn);
	std::vector<PayloadDetails> run(int testNum);
	unsigned getDataSent() const;

	LogFn logFn;

  private:
	struct Config;

	void initializeRF24();
	std::vector<std::uint64_t> generateRandomData() const;
	void configureReceiver(int config);
	void configureTransmitter(int config);
	static ConfigPayload generateConfigPayload(const Config &config);
	static Config configFromIndex(int config);
	void log(const std::string &msg) const;

	struct Config
	{
		unsigned delayms;
		unsigned payloadSize;
		rf24_datarate_e bitrate;
		rf24_pa_dbm_e power;
		unsigned retryDelay;
		unsigned retryCount;
	};

	RF24 rf24;
	std::vector<std::uint64_t> randomData;
	unsigned dataSent;
	Config currentConfig;
};
