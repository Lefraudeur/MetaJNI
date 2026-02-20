#pragma once
#include "../mappings.hpp"

class cache
{
public:
	cache();
	~cache();

	bool is_valid() const;
	bool update();

	maps::MinecraftClient instance{jni::GLOBAL};
	maps::ClientWorld world{ jni::GLOBAL };
	maps::ClientPlayerEntity player{ jni::GLOBAL };
	maps::List players{ jni::GLOBAL };
	maps::GameOptions options{ jni::GLOBAL };
	maps::Mouse mouse{ jni::GLOBAL };
private:
	bool _is_valid = false;
};