#include "graphicslib/preloadedPicManager.h"

void TPreloadedPicManager::Pause() {
}

void TPreloadedPicManager::Continue() {
}

void TPreloadedPicManager::StopPreloading(TPictureIO */*picture*/) {
}

void TPreloadedPicManager::ReleasePicture(TPictureIO */*picture*/) {
}

std::vector<TPictureIO *> TPreloadedPicManager::GetPreloadedPictures() {
	return {};
}

void TPreloadedPicManager::PreloadPicture(TPictureIO */*picture*/) {
}

void TPreloadedPicManager::StopPreloading(std::vector<TPictureIO *> &/*pictures*/) {
}

void TPreloadedPicManager::PreloadPictures(std::vector<TPictureIO *> /*pictures*/) {
}

bool TPreloadedPicManager::HasQueuedPictures() const {
	return false;
}
