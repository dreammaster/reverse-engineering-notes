#include "graphicslib/shader.h"

std::list<TShader *> shader_list;
std::vector<std::vector<TRenderPass> > shader_renderpasses;
int shader_buffers = 0;
int shader_transition = -1;

TShader *CreateShader() {
	return nullptr;
}

// Confirmed (asm lines 404590-404612, and the table `CSWTCH_1542` at 3141883): the blend modes of the engine for
// the numbers 0 to 10 of the scripts.
int CompositeEnums(int value) {
	static const int kModes[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 11};

	if (value < 0 || value > 10)
		return 2;

	return kModes[value];
}
