#pragma once
// Access to the cells of the current map (WL.EXE 0x17C20..0x17DF0).
//
// The map block holds, one row after the other, the tile class of every cell as a 4 bit value (two cells per byte, the
// even column in the high nibble) followed by one action byte per cell. `mapWidth` is a power of two. A cell may point at
// a "location" record (the action data of the map) through its action byte.
#include "wasteland1/machine.h"

namespace wl {

// 17C20: tile class (0..15) of cell (x, y). Keeps the pointer of the last row used in the cache fields of the data segment.
uint8_t tileClassAt(Machine &m, uint8_t x, uint8_t y);
// 17C72: makes `actionRowPtr` point at the action bytes of row y.
void selectActionRow(Machine &m, uint8_t y);
// 17CEB: action byte of cell (x, y).
uint8_t actionAt(Machine &m, uint8_t x, uint8_t y);
// 17CD2: tile class and action byte of cell (x, y).
struct Cell { uint8_t tileClass, action; };
Cell cellAt(Machine &m, uint8_t x, uint8_t y);
// 17D7A: sets the tile class of cell (x, y).
void setTileClass(Machine &m, uint8_t x, uint8_t y, uint8_t tileClass);
// 17D50: writes the pending cell change (newTileClass/newTileAction) into cell (x, y); true when it changed the cell (the
// class has the high bit clear).
bool applyCellChange(Machine &m, uint8_t x, uint8_t y);
// 17CFF: loads the cell change from byte `index` of the current location record (FEh: the previously remembered cell, FDh:
// remembered cell without writing anything), remembers the old content of (x, y) and applies the change.
bool changeCellFromLocation(Machine &m, uint8_t index, uint8_t x, uint8_t y);
// 17CB1: locationDetailsPtr = the `index`-th record pointer of table `table` of the map's location block.
void selectLocationRecord(Machine &m, uint8_t table, uint8_t index);
// 17DC7 / 17DE0: conversion between the party position and map coordinates (the view offset of the map window).
struct XY { uint8_t x, y; };
XY partyMapPosition(Machine &m);
XY toViewPosition(Machine &m, uint8_t x, uint8_t y);
void storeViewPosition(Machine &m);
// 18024: draws the map cell (x, y) into the view window (terrain tile, or the object tile of the cell's action class);
// the registers it leaves are the text cursor (dl = rectTextX, bl = rectTextY)
void drawMapCell(Machine &m, uint8_t x, uint8_t y);
// 16619 / 16646: redraws the column (nine cells) at the screen offset ds:4685 / the row (46A3 cells) at the offset ds:4686
void redrawViewColumn(Machine &m);
void redrawViewRow(Machine &m);
// 16584 / 165AC / 165D2 / 165FB: scroll the view one cell up / left / right / down and draw the cells that appear
void scrollViewUp(Machine &m);
void scrollViewLeft(Machine &m);
void scrollViewRight(Machine &m);
void scrollViewDown(Machine &m);
// 167CE: redraws the cells of the view window whose class carries an object (classes 4, 5 and 9)
void redrawViewObjects(Machine &m);
// 17DC7: the party position in map cells (view origin + 46A8/46A9); 17DBB stores it in 46A6/46A7
void partyCell(Machine &m, uint8_t &x, uint8_t &y);
// 169EB: selects the location record of the cell (x, y) and returns its tile class
uint8_t selectCellLocation(Machine &m, uint8_t x, uint8_t y);
// 16428: draws the markers of the other party groups that are inside the view
void drawOtherParties(Machine &m);
// 16716: draws the party marker; 163A0: redraws objects, other parties and the party marker
void drawPartyMarker(Machine &m);
void redrawViewAll(Machine &m);
// 17DF1: the clock, compass and light bar beside the view
void drawStatusBar(Machine &m);
// 1675C: draws the cell of the party position
void drawPartyCell(Machine &m);
// 14330: line of sight from the party to cell (x, y): walks the line with an error term (the original patches its own opcodes for
// the step directions) and stops at a cell of class 0Bh. True (CF) when blocked
bool lineBlocked(Machine &m, uint8_t x, uint8_t y);   // 17DBB: viewX/viewY = partyMapPosition()

} // namespace wl
