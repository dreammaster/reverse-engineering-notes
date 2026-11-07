// Small enums shared by the type (TTypeData/TTypeGroup) and visionaire headers,
// which would otherwise include each other.
#pragma once

// Confirmed one value, 0x22 (TGameControl::SaveGame, asm line 463095) - real
// meaning/other values not resolved (named for its raw value like TypeOrder,
// rather than guessing a table name).
enum class eVisionaireTable { kValue34 = 0x22 };

// Confirmed 3 values (TGameControl::PreLoad/LoadAndInitGame, asm lines 464227,
// 467804; TVisionaire::Load/BinaryLoad): 0 reads the file into the loader's
// buffer but only loads the Loading table (so a loading screen can be shown
// first), 1 loads everything from the buffer a pass of type 0 left behind, 2
// reads the file and loads everything in one go.
enum class TLoadingTypeEnum { kValue0 = 0, kValue1 = 1, kValue2 = 2 };

// Confirmed 3 values, 0-2 (TGameControl::PreLoad, Deponia_Linux.asm line
// 464228; TTypeData/TTypeGroup::IsFittingSaveGameType(), asm lines 668776/
// 585021) - whether a data field/type group exists in game data only (0), in
// savegames only (1, "t_SAVEGAME" in the original's assert text), or in both
// (2). Named by raw value like TypeOrder.
enum class eSaveGame { kValue0 = 0, kValue1 = 1, kValue2 = 2 };
