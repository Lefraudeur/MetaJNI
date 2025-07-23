#include "utils.hpp"

utils::null_string::null_string(const char* nulled_string) :
	data(nulled_string ? new char[std::strlen(nulled_string) + 1]{0} : nullptr),
	size(nulled_string ? std::strlen(nulled_string) : 0ULL)
{
	if (!nulled_string) return;
	memcpy(data, nulled_string, size);
}

utils::null_string::null_string(const null_string& other) :
	data(other.data ? new char[other.size + 1]{0} : nullptr),
	size(other.size)
{
	if (!other.data) return;
	memcpy(data, other.data, size);
}

utils::null_string::~null_string()
{
	if (!data) return;
	memset(data, 0, size + 1);
	delete[] data;
}

utils::null_string utils::null_string::operator+(const null_string& other)
{
	null_string result{};
	result.size = size + other.size;
	result.data = new char[result.size + 1] {0};
	memcpy(result.data, data, size);
	memcpy(result.data + size, other.data, other.size);
	return result;
}
