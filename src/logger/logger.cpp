#include "logger.hpp"

#ifndef NDEBUG
static std::ofstream logfile;
#endif

bool logger::init()
{
#ifndef NDEBUG
	logfile = std::ofstream("mujina_logs.txt");
	if (!logfile)
		return false;
#endif
	return true;
}

void logger::shutdown()
{
#ifndef NDEBUG
	logfile = std::ofstream();
#endif
}

void logger::log(std::string_view msg)
{
#ifndef NDEBUG
	logfile << "info: " << msg << std::endl;
#endif NDEBUG
}

void logger::error(std::string_view msg)
{
#ifndef NDEBUG
	logfile << "error: " << msg << std::endl;
#endif
}
