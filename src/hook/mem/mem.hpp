#pragma once
#include <cstdint>
#include <cstddef>

namespace mem
{
	enum protection
	{
		OTHER,
		READONLY,
		READ_WRITE,
		EXEC_READ,
		EXEC_READ_WRITE
	};
	void protect(void* address, size_t size, protection prot);
	void* alloc(size_t size);
	void free(void* address);
}