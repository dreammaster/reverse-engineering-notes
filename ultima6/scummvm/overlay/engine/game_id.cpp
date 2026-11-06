#include "engine/game_id.h"

#include "martian_dreams/game.h"
#include "savage_empire/game.h"
#include "ultima6/game.h"

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

// ScummVM's detection says which game it is; there is nothing to look for in the folder.
GameId detectGame(const std::string &) {
	return GameId::Unknown;
}

} // namespace Ultima6
