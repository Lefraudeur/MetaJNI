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

	net::minecraft::client::Minecraft new_theMinecraft = net::minecraft::client::Minecraft{}.theMinecraft.get();
	if (!new_theMinecraft) return false;

	net::minecraft::client::entity::EntityClientPlayerMP new_thePlayer = new_theMinecraft.thePlayer.get();
	if (!new_thePlayer) return false;

	net::minecraft::client::multiplayer::WorldClient new_theWorld = new_theMinecraft.theWorld.get();
	if (!new_theWorld) return false;

	if (!new_theMinecraft.is_same_object(theMinecraft)
		|| !new_thePlayer.is_same_object(thePlayer)
		|| !new_theWorld.is_same_object(theWorld))
	{
		theMinecraft = new_theMinecraft;
		thePlayer = new_thePlayer;
		theWorld = new_theWorld;

		if (!update_all()) return false;
	}

	_is_valid = true;

	return true;
}

bool cache::update_all()
{

	return true;
}
