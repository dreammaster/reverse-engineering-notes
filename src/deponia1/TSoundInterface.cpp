#include "TSoundInterface.h"

void TSoundInterface::Play(const wxFileName &/*file*/) {
}

int TSoundInterface::Play(const wxFileName &/*file*/, int /*volume*/, int /*balance*/, bool /*loop*/,
                          TSoundTypeEnum /*type*/, bool /*flag*/, int /*value*/) {
	return -1;
}

void TSoundInterface::ContinueAll() {
}

void TSoundInterface::CleanUp() {
}

void TSoundInterface::Stop(const wxFileName &/*file*/) {
}

void TSoundInterface::SetStats(const wxFileName &/*file*/, int /*volume*/, int /*balance*/, TSoundTypeEnum /*type*/,
                               bool /*flag*/, int /*value*/) {
}

void TSoundInterface::FinishSoundFade() {
}

void TSoundInterface::StartSoundFade(TFadeEnum /*fade*/, int /*milliseconds*/, bool /*flag*/) {
}

bool TSoundInterface::IsPlaying(const wxFileName &/*file*/) const {
	return false;
}

void TSoundInterface::Keep(const wxFileName &/*file*/) {
}
