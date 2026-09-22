/* ags/roomobj.h's own implementation. See that header for the
 * complete evidence and scope decisions.
 */
#include "ags/roomobj.h"

#include <string.h>

void ags_init_room_status(struct RoomStatus *croom, const struct RoomStruct *rst)
{
    int cc;

    if (!croom || !rst || croom->beenhere != 0) {
        return;
    }

    croom->numobj = rst->numsprs;
    croom->tsdatasize = 0;

    for (cc = 0; cc < croom->numobj && cc < 10; cc++) {
        struct RoomObject *o = &croom->obj[cc];

        o->x = rst->sprs[cc].x;
        o->y = rst->sprs[cc].y;
        o->num = rst->sprs[cc].sprnum;
        o->on = (char)rst->sprs[cc].on;
        o->view = -1;
        o->loop = 0;
        o->frame = 0;
        o->wait = 0;
        o->transparent = 0;
        o->moving = 0; /* this build's own "not moving" sentinel -- see ags/roomobj.h's own scope note, not source's -1 */
        o->cycling = 0;
        o->overall_speed = 0;
        o->flags = 0; /* no confirmed on-disk source array for this build -- see ags/roomobj.h */
        o->baseline = -1;
        if (rst->objbaseline[cc] >= 0) {
            o->baseline = (short)rst->objbaseline[cc];
        }
    }

    memcpy(croom->walkbehind_base, rst->objyval, sizeof(croom->walkbehind_base));
    for (cc = 0; cc < 15; cc++) {
        croom->flagstates[cc] = 0;
    }

    /* This build's own live hscond/objcond/misccond copy -- see ags/
     * roomobj.h's own file-level comment (struct-layout-drift.md's
     * own RoomStatus recovery evidence for these three copies). */
    memcpy(croom->hscond, rst->hscond, sizeof(croom->hscond));
    memcpy(croom->objcond, rst->objcond, sizeof(croom->objcond));
    croom->misccond = rst->misccond;

    for (cc = 0; cc < 20; cc++) {
        croom->hotspot_enabled[cc] = 1;
    }

    croom->beenhere = 1;
}

void ags_object_on(struct RoomStatus *croom, int obj)
{
    if (!croom || obj < 0 || obj >= croom->numobj || obj >= 10) {
        return;
    }
    croom->obj[obj].on = 1;
}

void ags_object_off(struct RoomStatus *croom, int obj)
{
    if (!croom || obj < 0 || obj >= croom->numobj || obj >= 10) {
        return;
    }
    croom->obj[obj].on = 0;
}

void ags_draw_room_objects(const struct RoomStatus *croom, struct AgsSpriteSet *sprites, BITMAP *target)
{
    int i;

    if (!croom || !sprites || !target) {
        return;
    }

    for (i = 0; i < croom->numobj && i < 10; i++) {
        const struct RoomObject *o = &croom->obj[i];
        BITMAP *spr;

        if (!o->on) {
            continue;
        }
        spr = ags_spriteset_load(sprites, o->num);
        if (!spr) {
            continue;
        }
        draw_sprite(target, spr, o->x, o->y);
        destroy_bitmap(spr);
    }
}
