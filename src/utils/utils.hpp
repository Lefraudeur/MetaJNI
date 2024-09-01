#pragma once
#include <cstring>

namespace utils
{
	// freeed heap regions can still be read, null before free
	class null_string
	{
	public:
		null_string(const char* nulled_string = nullptr);
		null_string(const null_string& other);
		~null_string();

		null_string operator+(const null_string& other);
		null_string& operator=(const null_string& other);

		char* data; //null terminated
		size_t size;
	};
}
