#include "engine/game_id.h"

#include "martian_dreams/game.h"
#include "savage_empire/game.h"
#include "ultima6/game.h"

#include <cstdio>

namespace Ultima6 {

const GameInfo &gameInfo(GameId id) {
	static const GameInfo UNKNOWN = {GameId::Unknown, "Unknown", nullptr, nullptr, nullptr};

	switch (id) {
	case GameId::Ultima6:
		return Ultima6::info();
	case GameId::MartianDreams:
		return MartianDreams::info();
	case GameId::SavageEmpire:
		return SavageEmpire::info();
	default:
		return UNKNOWN;
	}
}

static bool fileExists(const std::string &dir, const char *name) {
	std::string path = dir;
	if (!path.empty() && path.back() != '/' && path.back() != '\\')
		path += '/';
	path += name;
	if (FILE *f = std::fopen(path.c_str(), "rb")) {
		std::fclose(f);
		return true;
	}
	return false;
}

GameId detectGame(const std::string &dir) {
	if (fileExists(dir, "MARTIAN.EXE"))
		return GameId::MartianDreams;
	if (fileExists(dir, "SAVAGE.EXE"))
		return GameId::SavageEmpire;
	if (fileExists(dir, "ULTIMA6.EXE") && fileExists(dir, "GAME.EXE"))
		return GameId::Ultima6;
	return GameId::Unknown;
}

} // namespace Ultima6
