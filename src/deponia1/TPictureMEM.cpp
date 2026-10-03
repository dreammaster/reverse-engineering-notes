#include "TPictureMEM.h"

unsigned int TPictureMEM::GetPixel(const wxPoint &/*pos*/, float /*brightness*/) const {
	return 0xFFFFFFFF;
}

void TPictureMEM::SetMemoryBlock(TPictureMemBlock */*block*/) {
}

void TPictureMEM::ClearMemData() {
	_width = 0;
	_height = 0;
}
