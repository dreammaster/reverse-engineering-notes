#ifndef YENDOR23_VIEWRENDER_H
#define YENDOR23_VIEWRENDER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dungeongrid.h"
#include "game.h"
#include "viewport.h"

/*
 * The first-person dungeon view: RedrawDungeonScreen's drawing passes (yendor2.asm:29571) -- DrawDungeonFloorAndCeiling,
 * ExtendDungeonCeilingPass, RenderDungeonViewport and the table-driven blitter DrawViewportSprite (:46553) -- applied to
 * the 51-cell buffer and occlusion flags of viewport.h. The picture-geometry tables are one 0x499C-byte block of WORLD.DAT
 * (PreloadMonsterStatsTable, misnamed; ida_scripts/dump_view_tables_offset.py) that is byte-identical in both games.
 *
 * Nothing is scaled: every depth has its own cut of the same pictures. The viewport is 224 x 136 pixels at (8, 8) of the
 * 320 x 200 screen: the ceiling (category 5, 224 x 62) at y = 8, the floor (category 4, 224 x 74) at y = 70, then the
 * cells' pictures on top. Colours are 8-bit palette indices; shading adds a signed delta to the colour's low nibble inside
 * its 16-colour block (ShiftPaletteShadeClamped; colours >= 0xD0 and delta 0 are left alone), and 0xFF is transparent.
 *
 * The tables, per sprite "layer" (word_32918) and per cell index (the 51-cell buffer index, g_viewportRowDepth), 6-byte
 * entries {x, y, ptr}: x = 0 means "not drawn at this cell":
 *   val11 @0x0000  front-facing wall faces (layers 0 and 8)        ptr1 @0x3B76  layer 7        ptr2..ptr7 @0x3CA8, 0x3DF2,
 *   val12 @0x0386  floor patches (layer 1)                         0x41D2, 0x4316, 0x4460, 0x4858  layers 9..14
 *   val13 @0x0BF2  ceiling patches (layer 2)                       val14 @0x13B6  side walls (3 left, 4 right), vanishing strips (6)
 * Layers 0, 7, 8, 9-14 ("row-mask sprites"): ptr -> {u16 group list, run records...}. A run record is {count, run, skip}
 * (three words, count 0 ends the list): `count` times copy `run` source pixels to the destination, then skip `skip` source
 * pixels; applied to every scanline. The group list at the first word is {repeat, rows, skip} records (repeat 0 ends): `repeat`
 * times {draw `rows` scanlines, then skip `skip` source scanlines}. So horizontal and vertical decimation are both encoded; the
 * destination scanline is 320 bytes on.
 * Layers 1 and 2 (polygon patches): ptr -> records {run, shift, _}; the source starts at (x, y) of the picture and lands at
 * (x + 8, y + 70) for the floor / (x + 8, y + 8) for the ceiling; each scanline copies `run` pixels, then both pointers move
 * by `shift` after stepping a row. Run 0 ends.
 * Layers 3 and 4 (side walls): ptr -> {columns, srcAdvance, {count, run, skip} records, 0}* : each column is a vertical
 * decimation pattern (same record semantics down the source column, drawn down the screen); after a set of columns the
 * destination steps one row down (left walls) / up (right walls) and the next set starts.
 * Layer 6 (the far strips beside the party): 113 rows x 7 pixels from column `frame` of a 56-wide category 6 picture at (x, y).
 */
enum {
    ViewTablesSize = 0x499C,
    ViewScreenWidth = 320,
    ViewScreenHeight = 200,
    ViewGradientSize = 7
};

/* File offset of the table block inside WORLD.DAT. */
uint32_t viewTablesOffset(GameKind game);

/* Pixels (width * height bytes, or NULL) of a picture; see pictures.h. */
typedef const uint8_t *(*ViewPictureFn)(void *ctx, unsigned category, unsigned id);

typedef struct {
    GameKind game;
    const uint8_t *tables; /* ViewTablesSize bytes */
    ViewPictureFn picture;
    void *pictureCtx;
    uint8_t *screen; /* 320 x 200 palette indices */
} ViewRenderer;

typedef struct {
    const DungeonGridCell *cells; /* ViewportCellCount cells with ViewportCellHidden set by viewportComputeVisibility */
    uint16_t facing;              /* SaveFacing */
    int16_t gradient[ViewGradientSize]; /* lightingComputeGradient: [0] farthest row ... [6] the party's row */
} ViewScene;

/* ShiftPaletteShadeClamped: `colour` shifted by `delta` (an 8-bit signed delta). */
uint8_t viewShadeColour(uint8_t colour, int8_t delta);

/*
 * DrawViewportSprite: picture `id` of `category` at sprite layer `layer` (word_32918) for buffer cell `depth`, shaded by
 * `shade`, with 0xFF transparent when `transparent`; `frame` is the source column offset of layer 6.
 */
void viewDrawSprite(const ViewRenderer *r, unsigned layer, unsigned category, unsigned id, unsigned depth, int8_t shade, bool transparent,
                    unsigned frame);

/*
 * DrawPicture (yendor2.asm:46133) for the plain modes: picture `id` of `category` copied with its top-left at (x, y); `transparent`
 * skips colour 0xFF; `shade` is the ShiftPaletteShadeClamped delta (0 = none). Pixels outside the 320 x 200 screen are clipped.
 */
void viewDrawPicture(const ViewRenderer *r, unsigned category, unsigned id, int x, int y, bool transparent, int8_t shade);

/* The ceiling, floor and every cell's patches, walls, features (monsters are not drawn). */
void viewRender(const ViewRenderer *r, const ViewScene *scene);

#endif
