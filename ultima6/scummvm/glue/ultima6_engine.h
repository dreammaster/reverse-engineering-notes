#ifndef ULTIMA_ULTIMA6_ULTIMA6_ENGINE_H
#define ULTIMA_ULTIMA6_ULTIMA6_ENGINE_H

#include "engines/engine.h"
#include "ultima/detection.h"

namespace Ultima {
namespace Ultima6 {

/**
 * Ultima VI, Martian Dreams and Savage Empire, run by the engine written from the original programs
 * (engine/, files/, ultima6/, martian_dreams/, savage_empire/); this class is only what ScummVM needs around it:
 * the screen, the keyboard and mouse, the AdLib sound and the saved games.
 */
class Ultima6Engine : public Engine {
private:
	const UltimaGameDescription *_gameDescription;

protected:
	Common::Error run() override;

public:
	Ultima6Engine(OSystem *syst, const UltimaGameDescription *gameDesc);
	~Ultima6Engine() override;

	bool hasFeature(EngineFeature f) const override;
};

} // namespace Ultima6
} // namespace Ultima

#endif
