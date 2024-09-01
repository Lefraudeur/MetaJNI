#include "modules.hpp"

void modules::fastbreak::run(::cache& cache)
{
	if (cache.theMinecraft.playerController.get().curBlockDamageMP.get() > 0.3f)
		cache.theMinecraft.playerController.get().curBlockDamageMP = 1.0f;
}