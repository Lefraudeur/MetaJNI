#include "logger.hpp"

static std::ofstream logfile;

bool logger::init()
{
	logfile = std::ofstream("mujina_logs.txt");
	if (!logfile)
		return false;
	return true;
}

void logger::shutdown()
{
	logfile = std::ofstream();
}

void logger::log(std::string_view msg)
{
	logfile << "info: " << msg << std::endl;
}

void logger::error(std::string_view msg)
{
	logfile << "error: " << msg << std::endl;
}
