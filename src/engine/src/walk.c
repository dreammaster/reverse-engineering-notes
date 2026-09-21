/* ags/walk.h's own implementation. See that header's file-level
 * comment for the full set of scoping decisions this module makes.
 */
#include "ags/walk.h"

#include <stdlib.h>

/* line_callback/can_see_from (Engine/routefnd.cpp:72-133) -- a
 * direct, unmodified port (both are already confirmed verbatim
 * matches in this project's own reversing notes). `line_failed`/
 * `lastcx`/`lastcy` are file-scope statics here instead of the real
 * engine's own globals, since this module's own can_see_from is the
 * only caller of do_line -- no other code needs to observe them. */
static int s_line_failed;
static int s_lastcx, s_lastcy;

static void line_callback(BITMAP *bmpp, int x, int y, int d)
{
    (void)d;
    if (getpixel(bmpp, x, y) < 1) {
        s_line_failed = 1;
    } else if (s_line_failed == 0) {
        s_lastcx = x;
        s_lastcy = y;
    }
}

static int can_see_from(block wallscreen, int x1, int y1, int x2, int y2)
{
    s_line_failed = 0;
    s_lastcx = x1;
    s_lastcy = y1;

    if (x1 == x2 && y1 == y2) {
        return 1;
    }

    do_line(wallscreen, x1, y1, x2, y2, 0, line_callback);
    return s_line_failed == 0;
}

/* MAKE_INTCOORD (Engine/routefnd.cpp:764) -- exact port. */
static int make_intcoord(int x, int y)
{
    return (int)(((unsigned int)(unsigned short)x << 16) | (unsigned int)(unsigned short)y);
}

/* calculate_move_stage (Engine/routefnd.cpp:682-761), adapted to take
 * the fixed-point move speeds as parameters instead of reading
 * source's own move_speed_x/move_speed_y globals -- otherwise a
 * direct, unmodified port of the real fixed-point trig. */
static void calculate_move_stage(struct MoveList *mlsp, int aaa, fixed move_speed_x, fixed move_speed_y)
{
    short ourx, oury, destx, desty;
    fixed xdist, ydist;
    fixed use_move_speed;
    fixed angl;
    fixed newxmove, newymove;

    if (mlsp->pos[aaa] == mlsp->pos[aaa + 1]) {
        mlsp->xpermove[aaa] = 0;
        mlsp->ypermove[aaa] = 0;
        return;
    }

    ourx = (short)((mlsp->pos[aaa] >> 16) & 0x0000ffff);
    oury = (short)(mlsp->pos[aaa] & 0x0000ffff);
    destx = (short)((mlsp->pos[aaa + 1] >> 16) & 0x0000ffff);
    desty = (short)(mlsp->pos[aaa + 1] & 0x0000ffff);

    if (ourx == destx) {
        mlsp->xpermove[aaa] = 0;
        mlsp->ypermove[aaa] = move_speed_y;
        if (desty < oury) {
            mlsp->ypermove[aaa] = -mlsp->ypermove[aaa];
        }
        return;
    }

    if (oury == desty) {
        mlsp->xpermove[aaa] = move_speed_x;
        mlsp->ypermove[aaa] = 0;
        if (destx < ourx) {
            mlsp->xpermove[aaa] = -mlsp->xpermove[aaa];
        }
        return;
    }

    xdist = itofix(abs(ourx - destx));
    ydist = itofix(abs(oury - desty));

    if (move_speed_x == move_speed_y) {
        use_move_speed = move_speed_x;
    } else {
        fixed xproportion = fixdiv(xdist, xdist + ydist);
        if (move_speed_x > move_speed_y) {
            use_move_speed = move_speed_y + fixmul(xproportion, move_speed_x - move_speed_y);
        } else {
            use_move_speed = move_speed_x + fixmul(itofix(1) - xproportion, move_speed_y - move_speed_x);
        }
    }

    angl = fixatan(fixdiv(ydist, xdist));
    newymove = fixmul(use_move_speed, fixsin(angl));
    newxmove = fixmul(use_move_speed, fixcos(angl));

    if (destx < ourx) newxmove = -newxmove;
    if (desty < oury) newymove = -newymove;

    mlsp->xpermove[aaa] = newxmove;
    mlsp->ypermove[aaa] = newymove;
}

int ags_walk_character_straight(struct CharacterInfo *chin, struct AgsWalkState *ws,
                                 block walkable_mask, const struct ViewStruct272 *views,
                                 int tox, int toy)
{
    int charX = chin->x, charY = chin->y;
    fixed move_speed;

    ws->active = 0;
    chin->walking = 0;

    if (tox == charX && toy == charY) {
        return 0;
    }

    /* MoveCharacterStraight's own line-of-sight resolution: if the
     * target isn't directly visible, fall back to the last reachable
     * point along the line. */
    if (!can_see_from(walkable_mask, charX, charY, tox, toy)) {
        tox = s_lastcx;
        toy = s_lastcy;
    }

    if (tox == charX && toy == charY) {
        return 0;
    }

    /* set_route_move_speed (routefnd.cpp:662) -- this build's own
     * CharacterInfo has no separate walkspeed_y field (confirmed
     * absent, see ags/character.h), so both axes always share
     * chin->walkspeed. Negative speeds mean "1 pixel every N frames"
     * (source's own reciprocal convention), matched here too. */
    if (chin->walkspeed < 0) {
        move_speed = fixdiv(itofix(1), itofix(-chin->walkspeed));
    } else {
        move_speed = itofix(chin->walkspeed);
    }

    ws->mls.numstage = 2;
    ws->mls.pos[0] = make_intcoord(charX, charY);
    ws->mls.pos[1] = make_intcoord(tox, toy);
    calculate_move_stage(&ws->mls, 0, move_speed, move_speed);

    ws->mls.fromx = charX;
    ws->mls.fromy = charY;
    ws->mls.onstage = 0;
    ws->mls.onpart = 0;
    ws->mls.doneflag = 0;
    ws->mls.lastx = -1;
    ws->mls.lasty = -1;

    ws->active = 1;
    chin->walking = 1;
    chin->wait = 0;

    /* mls.pos[0]!=mls.pos[1] is already guaranteed by the two
     * "already there" early-returns above, matching source's own
     * guard before calling fix_player_sprite. */
    ags_fix_player_sprite(&ws->mls, chin, views);

    return 1;
}

int ags_do_movelist_move(struct MoveList *cmls, int *inout_x, int *inout_y)
{
    int need_to_fix_sprite = 0;
    fixed xpermove = cmls->xpermove[cmls->onstage];
    fixed ypermove = cmls->ypermove[cmls->onstage];
    short targetx = (short)((cmls->pos[cmls->onstage + 1] >> 16) & 0x00ffff);
    short targety = (short)(cmls->pos[cmls->onstage + 1] & 0x00ffff);
    int xps = *inout_x, yps = *inout_y;

    if (cmls->doneflag & 1) {
        int adjAmnt = 3;
        if (((xpermove & 0xffff0000L) == 0xffff0000L) || ((xpermove & 0xffff0000L) == 0x00000000L)) {
            adjAmnt = 2;
        }
        if (ypermove == 0) {
            /* nothing */
        } else if ((ypermove & 0xffff0000L) == 0) {
            targety -= adjAmnt;
        } else if (ypermove == (fixed)0xffff0000L) {
            /* nothing */
        } else if ((ypermove & 0xffff0000L) == 0xffff0000L) {
            targety += adjAmnt;
        }
    } else {
        xps = cmls->fromx + (int)(fixtof(xpermove) * (float)cmls->onpart);
    }

    if (cmls->doneflag & 2) {
        int adjAmnt = 3;
        if (((ypermove & 0xffff0000L) == 0xffff0000L) || ((ypermove & 0xffff0000L) == 0x00000000L)) {
            adjAmnt = 2;
        }
        if (xpermove == 0) {
            /* nothing */
        } else if ((xpermove & 0xffff0000L) == 0) {
            targetx -= adjAmnt;
        } else if (xpermove == (fixed)0xffff0000L) {
            /* nothing */
        } else if ((xpermove & 0xffff0000L) == 0xffff0000L) {
            targetx += adjAmnt;
        }
    } else {
        yps = cmls->fromy + (int)(fixtof(ypermove) * (float)cmls->onpart);
    }

    if ((xpermove > 0 && xps >= targetx) || (xpermove < 0 && xps <= targetx)) {
        cmls->doneflag |= 1;
        xps = targetx;
    } else if (xpermove == 0) {
        cmls->doneflag |= 1;
    }

    if (ypermove > 0 && yps >= targety) {
        cmls->doneflag |= 2;
        yps = targety;
    } else if (ypermove < 0 && yps <= targety) {
        cmls->doneflag |= 2;
        yps = targety;
    } else if (ypermove == 0) {
        cmls->doneflag |= 2;
    }

    if ((cmls->doneflag & 0x03) == 3) {
        cmls->fromx = (signed short)((cmls->pos[cmls->onstage + 1] >> 16) & 0x0000ffff);
        cmls->fromy = (signed short)(cmls->pos[cmls->onstage + 1] & 0x0000ffff);

        cmls->onstage++;
        cmls->onpart = -1;
        cmls->doneflag &= 0xf0;
        cmls->lastx = -1;

        if (cmls->onstage < cmls->numstage) {
            xps = cmls->fromx;
            yps = cmls->fromy;
        }
        if (cmls->onstage >= cmls->numstage - 1) {
            /* last stage is just the destination position -- this
             * module's own model never has more than 2 waypoints, so
             * this is always the branch taken. */
            cmls->numstage = 0;
            need_to_fix_sprite = 1;
        } else {
            need_to_fix_sprite = 2;
        }
    }
    cmls->onpart++;

    *inout_x = xps;
    *inout_y = yps;
    return need_to_fix_sprite;
}

/* find_looporder_index/useDiagonal/hasUpDownLoops/fix_player_sprite's
 * own core (Engine/acchars.cpp:19,124-267) -- direct ports of the
 * pure direction-selection logic; see ags/walk.h's file-level comment
 * for why the gradual-turning tail is not ported. */
static const int turnlooporder[8] = { 0, 6, 1, 7, 3, 5, 2, 4 };

static int use_diagonal(const struct CharacterInfo *chinf, const struct ViewStruct272 *views)
{
    const struct ViewStruct272 *v = &views[chinf->view];
    if (v->numloops < 8 || (chinf->flags & 0x08) != 0) { /* CHF_NODIAGONAL */
        return 1;
    }
    if (v->numframes[4] < 2) {
        return 2;
    }
    return 0;
}

static int has_updown_loops(const struct CharacterInfo *chinf, const struct ViewStruct272 *views)
{
    const struct ViewStruct272 *v = &views[chinf->view];
    if (v->numframes[0] < 1 || v->numloops < 4 || v->numframes[3] < 1) {
        return 0;
    }
    return 1;
}

void ags_fix_player_sprite(const struct MoveList *cmls, struct CharacterInfo *chin,
                            const struct ViewStruct272 *views)
{
    int want_horiz = 1;
    int useloop = 1;
    int no_diagonal;
    fixed xpmove = cmls->xpermove[cmls->onstage];
    fixed ypmove = cmls->ypermove[cmls->onstage];

    (void)turnlooporder; /* only needed by the gradual-turning tail this module doesn't implement */

    if (xpmove == 0 && ypmove == 0) {
        return;
    }

    if (has_updown_loops(chin, views) == 0) {
        want_horiz = 1;
    } else if ((ypmove < 0 ? -ypmove : ypmove) > (xpmove < 0 ? -xpmove : xpmove)) {
        want_horiz = 0;
    }

    no_diagonal = use_diagonal(chin, views);

#define CHECK_DIAGONAL(maindir, othdir, codea, codeb) \
    do { \
        if (no_diagonal) { \
            /* no diagonal frames available */ \
        } else if ((maindir < 0 ? -(maindir) : (maindir)) > (othdir < 0 ? -(othdir) : (othdir)) / 2) { \
            useloop = (maindir < 0) ? (codea) : (codeb); \
        } \
    } while (0)

    if (want_horiz == 1 && xpmove > 0) {
        useloop = 2;
        CHECK_DIAGONAL(ypmove, xpmove, 5, 4);
    } else if (want_horiz == 1 && xpmove <= 0) {
        useloop = 1;
        CHECK_DIAGONAL(ypmove, xpmove, 7, 6);
    } else if (ypmove < 0) {
        useloop = 3;
        CHECK_DIAGONAL(xpmove, ypmove, 7, 5);
    } else {
        useloop = 0;
        CHECK_DIAGONAL(xpmove, ypmove, 6, 4);
    }

#undef CHECK_DIAGONAL

    /* Always the "OPT_ROTATECHARS disabled"/CHF_NOTURNING path -- see
     * ags/walk.h's own file-level comment. */
    chin->loop = useloop;
}

void ags_advance_walk_animation(struct CharacterInfo *chin, struct AgsWalkState *ws,
                                 const struct ViewStruct272 *views)
{
    int x, y;
    int result;
    int numFrames;

    if (!ws->active) {
        return;
    }

    if (chin->wait > 0) {
        chin->wait--;
        return;
    }

    x = chin->x;
    y = chin->y;
    result = ags_do_movelist_move(&ws->mls, &x, &y);
    chin->x = x;
    chin->y = y;

    if (result != 0) {
        ws->active = 0;
        chin->walking = 0;
        chin->frame = 0;
        chin->wait = 0;
        return;
    }

    if (chin->loop >= 8) {
        chin->loop = 0;
    }
    numFrames = views[chin->view].numframes[chin->loop];
    chin->frame++;
    if (chin->frame >= numFrames) {
        chin->frame = (numFrames < 2) ? 0 : 1;
    }
    chin->wait = views[chin->view].frames[chin->loop][chin->frame].speed + chin->animspeed;
}
