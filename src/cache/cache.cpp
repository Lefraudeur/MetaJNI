#include "cache.hpp"

cache::cache()
{
}

cache::~cache()
{
}

bool cache::is_valid() const
{
	return _is_valid;
}

bool cache::update()
{
	jni::frame frame{};

	_is_valid = false;

	instance = maps::MinecraftClient{}.instance.get();
	if (!instance) return false;

	world = instance.world.get();
	if (!world) return false;

	player = instance.player.get();
	if (!player) return false;

	options = instance.options.get();
	if (!options) return false;

	mouse = instance.mouse.get();
	if (!mouse) return false;

	players = world.players.get();
	if (!players) return false;

	_is_valid = true;

	return true;
}
