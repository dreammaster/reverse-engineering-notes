// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vstables/visionaireGame.cpp - see manifest/source_layout.tsv.
// Heap-allocated and owned by TGameControl (confirmed from its ctor);
// exposed via GetVisionaire()/GetGameSystem().
#pragma once

class TVisionaireGame {
public:
	TVisionaireGame() = default;
};

// Confirmed a free function, not a member (TGameControl::Save, asm line
// 462914) - not reversed beyond that call shape.
void SaveGlobalScriptVariables(TVisionaireGame &game);
// The load-side counterpart (TGameControl::Load, Deponia_Linux.asm line
// 477015) - not reversed beyond that call shape. Notable: this call site's
// argument comes from the SAME field offset (TGameControl+0xC8) that every
// other call in TGameControl::Load/LoadGame reads as `_visionaire` (a
// TVisionaire*) via confirmed TVisionaire::GetGame()/GetList() calls - but a
// free function's parameter type is unambiguous from its own mangled name,
// so this one spot is modeled with the already-existing `_visionaireGame`
// member instead (matching TGameControl::Save's own already-shipped
// SaveGlobalScriptVariables(*_visionaireGame) call). Left as two distinct
// members rather than resolved into one, same as the standing TVisionaire/
// TVisionaireGame "not yet integrated" gap (see LoadDataGame/LoadSaveGame's
// own comments) - but this is a second, independent data point suggesting
// they may really be the same underlying object in the original binary.
void LoadGlobalScriptVariables(TVisionaireGame &game);
