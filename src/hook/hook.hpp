#pragma once
#include "hde/hde64.h"
#include "mem/mem.hpp"
#include <type_traits>
#include <tuple>
#include <iostream>

template<typename T> concept function_t = std::is_function_v<std::remove_pointer_t<T>>;

template<function_t T>
class hook
{
public:

	template<function_t F>
	struct callback_gen;

	template<typename R, typename... Args>
	struct callback_gen<R(*)(Args...)>
	{
		using type = R(*)(hook* hk, void* return_address, Args...);
		static constexpr size_t args_count = sizeof...(Args);
	};

	template<typename R, typename... Args>
	struct callback_gen<R(*)(Args..., ...)>
	{
		using type = R(*)(hook* hk, void* return_address, Args..., ...);
		static constexpr size_t args_count = sizeof...(Args) + 16; // we don't know args_count, so let's assume we didn't put more than 16 varargs
	};

	typedef typename callback_gen<T>::type callback_t;



	inline static hook* create(T target, callback_t callback)
	{
		return new hook(target, callback);
	}

	// ask the detour function to destroy hook at the end of next call, useful when hooked function runs frequently in a different thread, prevent race condition
	inline void request_destroy() // warning, hook* is now invalid
	{
		_request = 1ULL;
	}
	// basically like request_destroy, except the hook* pointer remains valid, useful if you wanna know when the free happened with freeed()
	inline void request_free()
	{
		_request = 2ULL;
	}
	inline void destroy() // warning, hook* is now invalid
	{
		delete this;
	}
	bool destroy_requested() const
	{
		return _request == 1ULL;
	}
	bool free_requested() const
	{
		return _request == 2ULL;
	}
	bool freeed() const
	{
		return _freeed;
	}

	// called in destructor
	// disable hook
	// free allocated memory
	void free()
	{
		if (_freeed) return;

		if (target && memcmp(target, jmp_rip_plus_0, sizeof(jmp_rip_plus_0)) == 0)
		{
			mem::protect(target, bytes_to_replace, mem::protection::EXEC_READ_WRITE);
			memcpy(target, trampoline, bytes_to_replace);
			mem::protect(target, bytes_to_replace, mem::protection::EXEC_READ);
		}
		if (trampoline)
			mem::free(trampoline);
		if (pre_call)
			mem::free(pre_call);

		_freeed = true;
	}


	bool is_valid() const
	{
		return _is_valid;
	}

	T get_original() const
	{
		return trampoline;
	}

private:

	hook(T target, callback_t callback) :
		_request(0LL),
		target(target),
		callback(callback),
		trampoline(nullptr),
		bytes_to_replace(0LL),
		pre_call(nullptr)
	{
		if (!target || !callback) return;
		bytes_to_replace = find_bytes_to_replace();
		if (!bytes_to_replace) return;

		// Warning does not handle rip relative addressing when creating trampoline
		// Warning at least 14 bytes must be availabe
		const size_t trampoline_size = bytes_to_replace + ABSOLUTE_JMP_SIZE;
		trampoline = (T)mem::alloc(trampoline_size);
		if (!trampoline) return;
		mem::protect(trampoline, trampoline_size, mem::protection::EXEC_READ_WRITE);
		memcpy(trampoline, target, bytes_to_replace);
		//patch_rip_relative((uint8_t*)trampoline, bytes_to_replace, (uint8_t*)target, (uint8_t*)trampoline);
		memcpy((uint8_t*)trampoline + bytes_to_replace, jmp_rip_plus_0, sizeof(jmp_rip_plus_0));
		*(T*)((uint8_t*)trampoline + bytes_to_replace + sizeof(jmp_rip_plus_0)) = T((uint8_t*)target + bytes_to_replace);
		mem::protect(trampoline, trampoline_size, mem::protection::EXEC_READ);


		mem::protect(target, bytes_to_replace, mem::protection::EXEC_READ_WRITE);
		{
			/*
push rbp
push rsi
push rcx
push rdx
push r8
push r9
mov rbp, rsp

mov rcx, [rip + data]
mov rdx, [rsp + 48]
mov r8, [rsp + 24]
mov r9, [rsp + 16]

xor r10, r10
mov r11, [rip + data+ 16]

mov rsi, r11
and rsi, 1
cmp rsi, 1
je continue
push 0

continue:
lea rsi, [rbp + 48 + 32]
for_start:
cmp r10, r11
je for_end

mov rax, r11
sub rax, r10
push [rsi + rax * 8]

inc r10
jmp for_start
for_end:

push [rbp]
push [rbp + 8]

sub rsp, 32
call [rip + data + 24]
mov rsp, rbp
add rsp, 32
pop rsi
pop rbp

mov rcx, [rip + data]
mov rcx, [rcx]
cmp rcx, 0
je ex
cmp rcx, 2
je free
mov rcx, [rip +data]
mov rdx, rax
jmp [rip + data + 8]
free:
mov rcx, [rip +data]
mov rdx, rax
jmp [rip + data + 32]

ex:
ret

data:
#hook_ptr
#hook_delete
#parameter_count
#detour
#hook_free
			*/
			const uint8_t pre_call_shellcode[] = { 0x55, 0x56, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x48, 0x89, 0xE5, 0x48, 0x8B, 0x0D, 0x8F, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x54, 0x24, 0x30, 0x4C, 0x8B, 0x44, 0x24, 0x18, 0x4C, 0x8B, 0x4C, 0x24, 0x10, 0x4D, 0x31, 0xD2, 0x4C, 0x8B, 0x1D, 0x86, 0x00, 0x00, 0x00, 0x4C, 0x89, 0xDE, 0x48, 0x83, 0xE6, 0x01, 0x48, 0x83, 0xFE, 0x01, 0x74, 0x02, 0x6A, 0x00, 0x48, 0x8D, 0x75, 0x50, 0x4D, 0x39, 0xDA, 0x74, 0x0E, 0x4C, 0x89, 0xD8, 0x4C, 0x29, 0xD0, 0xFF, 0x34, 0xC6, 0x49, 0xFF, 0xC2, 0xEB, 0xED, 0xFF, 0x75, 0x00, 0xFF, 0x75, 0x08, 0x48, 0x83, 0xEC, 0x20, 0xFF, 0x15, 0x58, 0x00, 0x00, 0x00, 0x48, 0x89, 0xEC, 0x48, 0x83, 0xC4, 0x20, 0x5E, 0x5D, 0x48, 0x8B, 0x0D, 0x30, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x09, 0x48, 0x83, 0xF9, 0x00, 0x74, 0x26, 0x48, 0x83, 0xF9, 0x02, 0x74, 0x10, 0x48, 0x8B, 0x0D, 0x1A, 0x00, 0x00, 0x00, 0x48, 0x89, 0xC2, 0xFF, 0x25, 0x19, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x0D, 0x0A, 0x00, 0x00, 0x00, 0x48, 0x89, 0xC2, 0xFF, 0x25, 0x21, 0x00, 0x00, 0x00, 0xC3 };

			pre_call = mem::alloc(sizeof(pre_call_shellcode) + 40);
			mem::protect(pre_call, sizeof(pre_call_shellcode) + 40, mem::EXEC_READ_WRITE);
			memcpy(pre_call, pre_call_shellcode, sizeof(pre_call_shellcode));
			*(hook**)((uint8_t*)pre_call + sizeof(pre_call_shellcode)) = this;
			*(decltype(_destroy)**)((uint8_t*)pre_call + sizeof(pre_call_shellcode) + 8) = _destroy;
			*(int64_t*)((uint8_t*)pre_call + sizeof(pre_call_shellcode) + 16) = (int64_t)(callback_gen<T>::args_count > 4 ? (callback_gen<T>::args_count - 4) : 0);
			*(callback_t*)((uint8_t*)pre_call + sizeof(pre_call_shellcode) + 24) = callback;
			*(decltype(_free)**)((uint8_t*)pre_call + sizeof(pre_call_shellcode) + 32) = _free;
			mem::protect(pre_call, sizeof(pre_call_shellcode) + 40, mem::EXEC_READ);

			memcpy(target, jmp_rip_plus_0, sizeof(jmp_rip_plus_0));
			*(void**)((uint8_t*)target + sizeof(jmp_rip_plus_0)) = pre_call;
		}
		mem::protect(target, bytes_to_replace, mem::protection::EXEC_READ);
		_is_valid = true;
	}

	~hook()
	{
		free();
	}

	size_t find_bytes_to_replace()
	{
		T code = target;
		uint8_t replace = 0;
		hde64s hde{};
		while (replace < ABSOLUTE_JMP_SIZE)
		{
			hde64_disasm(code, &hde);
			replace += hde.len;
			code = (T)((uint8_t*)code + hde.len);
		}
		return replace;
	}

	void patch_rip_relative(uint8_t* to_patch, size_t to_patch_size, const uint8_t* original_code_address, const uint8_t* new_code_address)
	{
		// not done
		hde64s hde{};

		uint8_t* begin = to_patch;
		uint8_t* end = begin + to_patch_size;
		size_t index = 0;
		

		for (hde64s hde{}; begin < end; begin += hde.len, index += hde.len)
		{
			hde64_disasm(begin, &hde);

			if (begin[0] == 0x8B && begin[1] == 0x0D)
			{
				const uint8_t* new_rip = new_code_address + index + hde.len;
				const uint8_t* old_rip = original_code_address + index + hde.len;

				std::cout << ((old_rip + hde.disp.disp32) - new_rip) << '\n';
				int32_t new_rip_relative = (int32_t)((old_rip + hde.disp.disp32) - new_rip);
				int32_t* p_rip_relative = (int32_t*)(begin + 2);

				*p_rip_relative = new_rip_relative;
			}
		}
	}

	// interface so it's callable from assembly
	inline static void* _destroy(hook* hk, void* return_value)
	{
		hk->destroy();
		return return_value;
	}

	inline static void* _free(hook* hk, void* return_value)
	{
		hk->free();
		return return_value;
	}

	uint64_t _request; // will be read from assembly
	T target;
	callback_t callback;
	T trampoline;
	size_t bytes_to_replace;
	void* pre_call; // used to modify return address



	bool _is_valid = false;
	bool _freeed = false;

	inline static const uint8_t jmp_rip_plus_0[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
	inline static const size_t ABSOLUTE_JMP_SIZE = sizeof(jmp_rip_plus_0) + 8;
};
