#include "Log.h"
#include <fstream>
#include <cstdarg>
#include <boost/format.hpp>

const char* log_file = "polo.log";

Log::Log(const std::string& label):
	label_(label)
{
	std::ofstream f(log_file, std::ios_base::app | std::ofstream::out);
	f << label_ << " opened" << std::endl;
}


Log::~Log()
{
	std::ofstream f(log_file, std::ios_base::app | std::ofstream::out);
	f << label_ << " closed" << std::endl;
}

void Log::init()
{
	std::ofstream f(log_file, std::ios_base::trunc | std::ofstream::out);
}

void Log::write(const std::string& msg)
{
	std::ofstream f(log_file, std::ios_base::app | std::ofstream::out);
	f << '\t' << msg << std::endl;
}
