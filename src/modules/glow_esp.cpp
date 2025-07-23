#include "modules.hpp"

void modules::glow_esp::run(::cache& cache)
{
	for (const maps::AbstractClientPlayerEntity& playerEntity : jni::array<maps::AbstractClientPlayerEntity>(cache.players.toArray()).to_vector())
	{
		playerEntity.setFlag(6L, JNI_TRUE);
	}
}

void modules::glow_esp::on_disable(::cache& cache)
{
	for (const maps::AbstractClientPlayerEntity& playerEntity : jni::array<maps::AbstractClientPlayerEntity>(cache.players.toArray()).to_vector())
	{
		playerEntity.setFlag(6L, JNI_FALSE);
	}
}