#include "mem.hpp"

#ifdef __linux__
	#include <sys/mman.h>
#elif _WIN32
	#include <Windows.h>
#endif

static int get_protection_id(mem::protection prot)
{
#ifdef __linux__
	switch (prot)
	{
	case mem::protection::EXEC_READ:
		return PROT_EXEC | PROT_READ;
	case mem::protection::EXEC_READ_WRITE:
		return PROT_EXEC | PROT_READ | PROT_WRITE;
	case mem::protection::READONLY:
		return PROT_READ;
	case mem::protection::READ_WRITE:
		return PROT_READ | PROT_WRITE;
	};
#elif _WIN32
	switch (prot)
	{
	case mem::protection::EXEC_READ:
		return PAGE_EXECUTE_READ;
	case mem::protection::EXEC_READ_WRITE:
		return PAGE_EXECUTE_READWRITE;
	case mem::protection::READONLY:
		return PAGE_READONLY;
	case mem::protection::READ_WRITE:
		return PAGE_READWRITE;
	};
#endif

	return mem::protection::OTHER;
}

void mem::protect(void* address, size_t size, protection prot)
{
	int id = get_protection_id(prot);
	if (!id)
		return;
#ifdef __linux__
	mprotect((void*)address, size, id);
#elif _WIN32
	DWORD buff = 0;
	VirtualProtect((void*)address, size, id, &buff);
#endif
}

void* mem::alloc(size_t size)
{
#ifdef __linux__
	void* allocated = mmap(nullptr, size + sizeof(size_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	*(size_t*)allocated = size + sizeof(size_t);
	return (size_t*)allocated + 1;
#elif _WIN32
	return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#endif
}

void mem::free(void* address)
{
#ifdef __linux__
	size_t* real_address = (size_t*)address - 1;
	size_t size = *real_address;
	munmap(real_address, size);
	return;
#elif _WIN32
	VirtualFree(address, 0, MEM_RELEASE);
	return;
#endif
}
