/* ags/roomobj.h -- M11+ ("the long tail", room objects, see
 * src/PLAN.md): the RoomObject/RoomStatus tracking that ags/
 * interaction.h's own respond[]==6/7/13 (ObjectOff/ObjectOn) and ags/
 * native_api.h's own ObjectOn dispatch have been stubbed for since M9
 * ("no RoomObject tracking yet"). Picked as the next real slice per
 * PLAN.md's own data-driven priority rule: test_interpreter.c's real
 * printed import list for Rob Blanc 1's own compiled global script
 * names AnimateObject/ObjectOn/MoveObject directly -- this is exactly
 * "whatever the real game actually calls" that this project hasn't
 * built yet.
 *
 * ags_init_room_status is a real, statement-by-statement port of
 * load_new_room's own "first visit" (croom->beenhere==0) block
 * (Engine/AC.CPP:4278-4325 -- this exact block is already cited in
 * matches.json's own `load_new_room` entry for RoomObject/RoomStatus
 * field recovery, e.g. `croom->obj[cc].x/y/num/on` and the hotspot_
 * enabled/flagstates resets). Real, cited scope decisions vs. that
 * source:
 *   - RoomObject.moving is initialized to 0, NOT source's -1: this
 *     build's own confirmed "not moving" sentinel is 0, not -1 (see
 *     struct-layout-drift.md's EndSkippingUntilCharStops entry:
 *     "objs[ff].moving=0" -- a real, disassembly-confirmed reset this
 *     build performs, matching ags/room.h's own RoomObject.moving
 *     field comment).
 *   - RoomObject.flags is left at 0 (no restrictions). 2011's
 *     `croom->obj[cc].flags = thisroom.objectFlags[cc]` has NO
 *     matches.json evidence of a Rob Blanc-era on-disk objectFlags[]
 *     array at all (RoomStruct, ags/room.h, has no such field) --
 *     rather than guess a source, this is left as an honestly
 *     documented gap, not a silent wrong value.
 *   - RoomObject.baseline: source's own `if (thisroom.objbaseline[cc]
 *     >=0) croom->obj[cc].baseline=thisroom.objbaseline[cc];` (else
 *     leave the -1 just set) is ported directly -- -1 is this
 *     project's own already-confirmed "use obj.y as the baseline
 *     instead" sentinel (matches.json's own GetObjectAt entry).
 *   - Region-related resets (2011's `region_enabled[]`) are skipped:
 *     the Region subsystem is confirmed entirely absent from this
 *     build (CLAUDE.md's own "Fresh AGS-side subsystem: Regions,
 *     confirmed entirely absent" entry, dated to AGS 2.55 -- after
 *     this build's own <2.5 pin).
 *   - This build's own hscond[]/objcond[]/misccond copy from
 *     RoomStruct to RoomStatus (2011 keeps this dead/commented-out at
 *     this exact point in load_new_room, per source's own comment
 *     right above it -- but struct-layout-drift.md's own RoomStatus
 *     recovery rounds found THIS build still performs three `rep
 *     movsd` block copies matching this exact shape) IS ported here,
 *     since both structs already carry real, confirmed hscond/objcond/
 *     misccond fields (ags/room.h) for exactly this purpose.
 *   - The `beenhere>0` branch (copying EventBlock timesrun counters
 *     back for the Score column) is NOT ported -- this build has no
 *     save/room-revisit flow exercising it yet; calling this function
 *     on an already-visited RoomStatus is a safe no-op (matches
 *     source's own `if(croom->beenhere==0)` guard).
 */
#ifndef AGS_ROOMOBJ_H
#define AGS_ROOMOBJ_H

#include "ags/room.h"
#include "ags/sprite_loader.h"

/* load_new_room's own real "first visit" RoomStatus init -- see this
 * header's own file-level comment for the complete evidence and scope
 * decisions. Idempotent past the first call (source's own
 * beenhere==0 guard; this port sets beenhere=1 at the end, matching
 * source exactly). */
void ags_init_room_status(struct RoomStatus *croom, const struct RoomStruct *rst);

/* ObjectOn(obj)/ObjectOff(obj) (Engine/AC.CPP, matches.json's own
 * ObjectOn/ObjectOff entries -- exact matches minus 2011's own
 * confirmed-absent StopObjectMoving/invalidate_screen calls, the
 * usual "predates that later machinery" pattern already established
 * elsewhere in this project). Out-of-range `obj` is ignored, matching
 * is_valid_object's own real validation role. */
void ags_object_on(struct RoomStatus *croom, int obj);
void ags_object_off(struct RoomStatus *croom, int obj);

/* A scoped real per-object renderer: draws every croom->obj[] with
 * on!=0, in array order (this build's own confirmed "array order IS
 * z-order" convention, per GUIMain's own established finding, applied
 * here the same way), at its own real x/y using its own real sprite
 * slot number (`num`). Static pose only -- the same documented
 * simplification M7 already made for the player character's own
 * rendering (no view/loop/frame animation-frame stepping here;
 * AnimateObject stays stubbed, ags/interaction.h's respond==4/
 * ags/native_api.h's AnimateObject entry, now with real RoomObject
 * data to build on next). `transparent` (per-object opacity,
 * SetObjectTransparency's own real field) is not applied -- draws
 * fully opaque regardless of its value; every object's own confirmed
 * default is 0 (opaque) per ags_init_room_status above, so real room
 * data's own default appearance renders correctly regardless of this
 * scope decision. */
void ags_draw_room_objects(const struct RoomStatus *croom, struct AgsSpriteSet *sprites, BITMAP *target);

#endif /* AGS_ROOMOBJ_H */
