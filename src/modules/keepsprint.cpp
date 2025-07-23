#include "modules.hpp"

void modules::keepsprint::run(::cache& cache)
{
	if (!(GetKeyState(0x5A) & 0x8000))
		return;

	if (!cache.player.isSprinting())
		cache.player.setSprinting(JNI_TRUE);
}