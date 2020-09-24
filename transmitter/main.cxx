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

#include "benchmark.hxx"

#include <iostream>

int pmain()
{
	Benchmark bench([](const std::string &msg) { std::clog << msg << std::endl; });
	std::vector<PayloadDetails> details = bench.run();

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
