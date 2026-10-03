#include "datastruct/vedfile.h"

bool TVedFile::IsSaveGame() const {
	return false;
}

bool TVedFile::GetVersionOk(int /*versionIn*/, int /*versionOut*/) const {
	return true;
}
