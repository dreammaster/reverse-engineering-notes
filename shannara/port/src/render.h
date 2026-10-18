// Room renderer: draws the current room the way the original builds it, driven by the recompiled game logic.
//   background  = picture get_room_pic_id(room) (honours game flags: room 88 swaps its picture once flag 22 is set)
//   overlays    = the tables the room handler passes to setup_pic_regions? in its draw phase (0x16); an entry is drawn when the game's own overlay_visible? says so
//                 (condition callback, container, required attribute); flag-0x40 sprites are XORed onto the background, an overlay carrying a palette replaces the active one.
// Needs the extracted picture files (SHANnnn.PIC from PICS.UHA). Animations (load_pic_sequence?) are not drawn.
#pragma once
#include "engine.h"

namespace shannara {

struct Image {
	int w = 0, h = 0;
	Bytes rgb; // w*h*3, 8 bits per component
};

struct RenderInfo {
	int room = 0, picId = 0;
	int tables = 0, entries = 0, drawn = 0, skippedPlain = 0, skippedBounds = 0;
};

// Draws the current room (the engine's current-room word) into an image. Runs the room's draw phase, which has no effect on the saved state.
Image renderRoom(Engine &e, const std::string &picDir, RenderInfo *info = nullptr);

// Minimal PNG writer (stored deflate blocks, no compression).
void writePng(const std::string &path, const Image &img);

} // namespace shannara
