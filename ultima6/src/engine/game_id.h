#ifndef ULTIMA6_ENGINE_GAME_ID_H
#define ULTIMA6_ENGINE_GAME_ID_H

#include <string>

namespace Ultima6 {

/**
 * The three games that run on the Ultima 6 engine. They share one code base;
 * everything that differs (intro, main menu, character creation, ending, ...)
 * lives in a per-game sub-namespace: Ultima6::Ultima6, Ultima6::MartianDreams
 * and Ultima6::SavageEmpire.
 */
enum class GameId {
	Unknown,
	Ultima6,       // Ultima VI: The False Prophet
	MartianDreams, // Worlds of Ultima: Martian Dreams
	SavageEmpire   // Worlds of Ultima: The Savage Empire
};

struct GameInfo {
	GameId id;
	const char *title;
	const char *mainExe;   // executable that holds the intro/menu/character creation
	const char *gameExe;   // executable that holds the game proper
	const char *endExe;    // separate ending executable, or nullptr when the game has none
};

/** Static description of a game; GameId::Unknown yields an empty entry. */
const GameInfo &gameInfo(GameId id);

/**
 * Works out which game a data directory holds from the executables found in it
 * (ULTIMA6.EXE, MARTIAN.EXE or SAVAGE.EXE). The directory is the one that
 * contains the executables, e.g. ".../MARTIAN" for Martian Dreams.
 */
GameId detectGame(const std::string &dir);

} // namespace Ultima6

#endif
