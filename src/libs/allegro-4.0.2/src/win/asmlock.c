/*
 *      C port of asmlock.s (originally i386 GAS assembly by Isaac Cruz,
 *      dirty-rectangles mechanism by Eric Botcazou).
 *
 *      Rewritten in portable C for the ags/src reimplementation build
 *      (the original targeted 32-bit x86 only; this build targets a
 *      modern toolchain and has no assembler dependency).
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "wddraw.h"

extern char *wd_dirty_lines;
extern void (*update_window)(RECT *rect);
extern char *gdi_dirty_lines;
extern void (*ptr_gfx_gdi_autolock)(BITMAP *bmp);
extern void (*ptr_gfx_gdi_unlock)(BITMAP *bmp);


static void update_dirty_lines(BITMAP *pseudo, char *dirty_lines)
{
   int line = 0;

   while (line < pseudo->h) {
      if (dirty_lines[line]) {
	 RECT rect;
	 rect.left = 0;
	 rect.right = pseudo->w;
	 rect.top = line;

	 do {
	    dirty_lines[line] = 0;
	    line++;
	 } while (line < pseudo->h && dirty_lines[line]);

	 rect.bottom = line;
	 update_window(&rect);
      }
      else {
	 line++;
      }
   }
}


unsigned long gfx_directx_write_bank(BITMAP *bmp, int line)
{
   if (!(bmp->id & BMP_ID_LOCKED))
      ptr_gfx_directx_autolock(bmp);

   return (unsigned long) bmp->line[line];
}


void gfx_directx_unwrite_bank(BITMAP *bmp)
{
   if (bmp->id & BMP_ID_AUTOLOCK) {
      ptr_gfx_directx_unlock(bmp);
      bmp->id &= ~BMP_ID_AUTOLOCK;
   }
}


unsigned long gfx_directx_write_bank_win(BITMAP *bmp, int line)
{
   wd_dirty_lines[bmp->y_ofs + line] = 1;

   if (!(bmp->id & BMP_ID_LOCKED))
      ptr_gfx_directx_autolock(bmp);

   return (unsigned long) bmp->line[line];
}


void gfx_directx_unwrite_bank_win(BITMAP *bmp)
{
   if (bmp->id & BMP_ID_AUTOLOCK) {
      ptr_gfx_directx_unlock(bmp);
      bmp->id &= ~BMP_ID_AUTOLOCK;
   }

   if (!(pseudo_screen->id & BMP_ID_LOCKED))
      update_dirty_lines(pseudo_screen, wd_dirty_lines);
}


void gfx_directx_unlock_win(BITMAP *bmp)
{
   ptr_gfx_directx_unlock(bmp);

   if (!(pseudo_screen->id & BMP_ID_LOCKED))
      update_dirty_lines(pseudo_screen, wd_dirty_lines);
}


unsigned long gfx_gdi_write_bank(BITMAP *bmp, int line)
{
   gdi_dirty_lines[bmp->y_ofs + line] = 1;

   if (!(bmp->id & BMP_ID_LOCKED))
      ptr_gfx_gdi_autolock(bmp);

   return (unsigned long) bmp->line[line];
}


void gfx_gdi_unwrite_bank(BITMAP *bmp)
{
   if (bmp->id & BMP_ID_AUTOLOCK) {
      ptr_gfx_gdi_unlock(bmp);
      bmp->id &= ~BMP_ID_AUTOLOCK;
   }
}
