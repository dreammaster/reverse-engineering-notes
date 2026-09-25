// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vstables/visionaireGame.cpp - see manifest/source_layout.tsv.
// Heap-allocated and owned by TGameControl (confirmed from its ctor);
// exposed via GetVisionaire()/GetGameSystem().
#pragma once

class TVisionaireGame {
public:
    TVisionaireGame() = default;
};
