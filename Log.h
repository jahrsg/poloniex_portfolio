#pragma once

#include <string>


class Log
{
	std::string label_;
public:
	Log(const std::string& label);
	~Log();

	static void init();
	static void write(const std::string& msg);
};

