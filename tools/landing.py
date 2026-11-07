#!/usr/bin/env python3
"""
Where the player lands after leaving a room through a door (derived from CAD.EXE room_enter/main loop and checked against the
original, tools/verify_landing.py).  The player appears at the *same door in the new room*, just inside it:
    door on the north wall (box y0 == 0):      x1 = box.x1 - 7,        y1 = rec[20] + 7      (wall thickness + player size)
    door on the south wall (box y0 >= 8*h):    x1 = box.x1 - 7,        y1 = 8*h - 1
    door on the west wall  (box x0 == 0):      x1 = rec[21] + 7,       y1 = box.y1 - 7
    door on the east wall  (box x0 >= 8*w):    x1 = 8*w - 1,           y1 = box.y1 - 7
(7 = the player's box size; z = 0)  The new room is the other room that lists the door id.
"""


def landing(exit_box, rec):
    """exit_box = (x1, y1, x0, y0, zt, zb, id) of the door in the NEW room, rec = its compact room record -> (x1, y1)."""
    x1, y1, x0, y0 = exit_box[:4]
    w8, h8 = rec[4] * 8, rec[5] * 8
    if y0 == 0:
        return x1 - 7, rec[20] + 7
    if y0 >= h8:
        return x1 - 7, h8 - 1
    if x0 == 0:
        return rec[21] + 7, y1 - 7
    return w8 - 1, y1 - 7
