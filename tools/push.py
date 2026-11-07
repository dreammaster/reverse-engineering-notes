#!/usr/bin/env python3
"""
Pushing objects, as the player's move does it in CAD.EXE (sub_16570 + sub_167DF, docs/formats.md 6.12).  A model validated against
the running game: 8 random key sequences (hundreds of steps) in level 1 room 0 reproduce every position of every object exactly
(tools/pushmodel check in the commit history; one unexplained event: a pushed sign disappeared).

  * the player's candidate position is tested with collision_test; blocked / exit / solid hit -> no move
  * every other overlapped object must be pushed by the same (dx, dy): its cost is its header byte 13 (weight), the sum may not
    exceed the player's strength (ds:59C = 50); the pushed object is tested the same way (self skipped), its overlaps are pushed
    recursively (depth < 32); any failure (room edge, solid object, budget) cancels the whole move.
"""
import collision


def push_move(ds, ss, ents, pl, dx, dy, strength):
    """ents: list of dict(rec, pos=[x1, y1, z0], w, sz=(xs, ys, zs)); the entry boxes are in ds.  pl: same for the player (not in ents).
    -> (status, moved indices) with status 'moved' | 'free' | 'blocked' | 'solid' | 'pushfail'."""
    byrec = {e['rec']: i for i, e in enumerate(ents)}

    def test(e, ddx, ddy):
        return collision.collision_test(ds, (e['pos'][0] + ddx) & 255, (e['pos'][1] + ddy) & 255, e['pos'][2] & 255,
                                        e['sz'][0], e['sz'][1], e['sz'][2], e['rec'], ss)

    def push(i, budget, depth):
        e = ents[i]
        budget -= e['w']
        if budget < 0 or depth >= 32:
            return False, budget, []
        r = test(e, dx, dy)
        if r['count'] in (0xFF, 0xFE) or r['flag'] == 0xFD:
            return False, budget, []
        moved = [i]
        for rec in r['ptrs']:
            j = byrec.get(rec)
            if j is None:
                continue
            ok, budget, m = push(j, budget, depth + 1)
            if not ok:
                return False, budget, []
            moved += m
        return True, budget, moved

    r = test(pl, dx, dy)
    if r['count'] in (0xFF, 0xFE):
        return 'blocked', []
    if r['flag'] == 0xFD:
        return 'solid', []
    budget, moved = strength, []
    for rec in r['ptrs']:
        j = byrec.get(rec)
        if j is None:
            continue
        ok, budget, m = push(j, budget, 0)
        if not ok:
            return 'pushfail', []
        moved += m
    return ('moved' if moved else 'free'), moved
