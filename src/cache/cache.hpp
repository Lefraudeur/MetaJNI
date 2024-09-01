#pragma once
#include "../mappings_auto.hpp"

class cache
{
public:
	cache();
	~cache();

	bool is_valid() const;
	bool update();

	net::minecraft::client::Minecraft theMinecraft{nullptr, true};
	net::minecraft::client::entity::EntityClientPlayerMP thePlayer{ nullptr, true };
	net::minecraft::client::multiplayer::WorldClient theWorld{ nullptr, true };
	java::io::FileDescriptor fd{ nullptr, true };

private:
	bool _is_valid = false;

	bool update_all();
};