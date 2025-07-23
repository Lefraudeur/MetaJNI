#pragma once
#include "../mappings.hpp"

class cache
{
public:
	cache();
	~cache();

	bool is_valid() const;
	bool update();

	maps::MinecraftClient instance{ nullptr, true };
	maps::ClientWorld world{ nullptr, true };
	maps::ClientPlayerEntity player{ nullptr, true };
	maps::List players{ nullptr, true };
	maps::GameOptions options{ nullptr, true };
	maps::Mouse mouse{ nullptr, true };
private:
	bool _is_valid = false;
};