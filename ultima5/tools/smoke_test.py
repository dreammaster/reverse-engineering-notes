"""
Headless smoke tests for the MinGW build (build/u5.exe).

    python tools/smoke_test.py [--game-dir C:/games/ultima5] [scenario ...]

Each scenario runs the game headless (U5_HEADLESS=1, U5_FAST=1) with a scripted key feed (see src/key/key_win32.c),
takes screenshots, and checks the process exits cleanly and that the screenshots are not blank.  Scenarios run in
run/ (a working copy of the game data; the original game directory is never modified).
"""
import argparse
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.abspath(os.environ.get("U5_EXE") or os.path.join(ROOT, "build", "u5.exe"))
RUN = os.path.join(ROOT, "run")

# name -> (keys, [screenshots expected], timeout seconds)
SPELLS_TOWN = [("IL", ""), ("AZ", "1{enter}"), ("AN", "1{enter}"), ("M", "1{enter}"), ("AY", "{up}"), ("AS", "{up}"), ("IMX", ""), ("LV", ""),
               ("IS", ""), ("AEP", "{up}"), ("EIP", "{up}"), ("MV", "1{enter}"), ("RT", ""), ("AI", ""), ("AWY", ""), ("IQW", "")]

VEHICLE_CMDS = {"ship": ["f{left}", "f{up}", "h", "1{sleep:3000}", "k", "x", "b", "x"], "horse": ["h", "1{sleep:3000}", "k", "x", "b", "x"],
                "skiff": ["f{up}", "k", "x", "b"], "carpet": ["h", "1{sleep:3000}", "k", "x", "b"]}

# item list index -> keys for the prompt that follows ("On who:" takes a digit and Enter, "Direction-" an arrow); carpet (16) skipped
USE_ITEMS = [(n, "{up}" if n in (1, 17) else "1{enter}" if n in (6, 8, 9, 10, 11, 12, 13, 14, 15) else "") for n in range(26) if n != 16]

# dungeon: spells usable underground with the keys for their follow-up prompt; PU ("Up!") leaves the dungeon so it goes last
DUNGEON_PRE = ["i", "{up}", "h", "1{sleep:1500}n{sleep:3000}", "s{up}", "g{up}", "o{up}", "{up}", "{left}", "j{up}", "u{sleep:600}{esc}"]
SPELLS_DUNGEON = [("IL", ""), ("AZ", "1{enter}"), ("AN", "1{enter}"), ("M", "1{enter}"), ("AS", ""), ("IMX", ""), ("LV", ""), ("FGI", ""),
                  ("GIN", ""), ("GIZ", ""), ("AG", ""), ("IS", ""), ("GIS", ""), ("DP", ""), ("MV", "1{enter}"), ("RT", ""), ("AI", ""),
                  ("IQW", ""), ("CIM", "1{enter}"), ("AT", ""), ("PU", "")]

# combat: the abbey fight with Negate Time poked in (guards cannot "interfere"); each spell is followed by passes for the other party members
_D = "{left}"
_A = "{up}{up}{up}{enter}"
SPELLS_COMBAT = [("GP", _A), ("AZ", "1{enter}"), ("AN", "1{enter}"), ("M", "1{enter}"), ("AY", _D), ("AS", _D), ("ACX", ""), ("KX", ""), ("IMX", ""),
                 ("FV", _A), ("FGI", _A), ("GIN", _A), ("GIZ", _A), ("IP", ""), ("AG", _D), ("IS", ""), ("GIS", _A), ("QW", ""), ("BIX", ""),
                 ("AEP", _D), ("EIP", _D), ("MV", "1{enter}"), ("IZ", _D), ("RT", ""), ("IPVY", ""), ("AQW", ""), ("AI", ""), ("AEX", _A),
                 ("BRX", _A), ("LS", ""), ("CX", _A), ("IQX", _A), ("HIN", _D), ("CIQ", ""), ("CKX", ""), ("CGIV", _D),
                 ("FHI", _D), ("AT", "")]

SCENARIO_ENV = {}  # extra environment per scenario (U5_MERCHANT: see TALK_TestMerchant)

SCENARIOS = {
    "boot": ("{sleep:500}{shot:boot.png}{quit}", ["boot.png"], 120),
    # 'J' at the very first key poll is the original "Journey Onward" shortcut; the pristine SAVED.GAM has no
    # character yet, so the game prints "No active game. Please create a character..."
    "play": ("j{sleep:6000}{shot:play1.png}{up}{sleep:700}{left}{sleep:700}{shot:play2.png}{quit}", ["play1.png", "play2.png"], 180),
    "stats": ("j{sleep:6000}z{sleep:1500}{shot:stats_select.png}1{sleep:1500}{shot:stats1.png}{esc}{sleep:800}{quit}",
              ["stats_select.png", "stats1.png"], 180),
    "look": ("j{sleep:6000}{right}{sleep:300}{down}{sleep:300}{down}{sleep:300}l{sleep:300}{down}{sleep:1500}{shot:look.png}"
             "k{sleep:300}{down}{sleep:1500}{shot:klimb.png}{quit}", ["look.png", "klimb.png"], 180),
    # "paula-outside" fixture: standing outside the start room; walking south to the map edge asks to leave
    "overworld": ("j{sleep:6000}" + "{down}{sleep:200}" * 14 + "{sleep:1000}{shot:leave.png}y{sleep:3000}{down}{sleep:1500}{sleep:5000}"
                  "{shot:overworld1.png}{down}{sleep:300}{sleep:5000}{shot:overworld2.png}z{sleep:800}1{sleep:1500}{shot:overworld_stats.png}{quit}",
                  ["leave.png", "overworld1.png", "overworld2.png", "overworld_stats.png"], 240),
    # "paula-abbey" fixture: one square east of Empath Abbey on the overworld
    "abbey": ("j{sleep:6000}{left}{sleep:2000}e{sleep:8000}{shot:abbey_gate.png}" + "{up}{sleep:700}" * 4 + "{left}{sleep:700}"
              "{shot:abbey_hall.png}t{sleep:500}{left}{sleep:2500}{shot:abbey_talk.png}{quit}",
              ["abbey_gate.png", "abbey_hall.png", "abbey_talk.png"], 240),
    # "paula-jester" fixture: Hardluck the abbey jester one square north (an empty line ends a conversation)
    "talk": ("j{sleep:6000}t{sleep:500}{up}{sleep:2500}{shot:talk_greet.png}name{enter}{sleep:3000}{shot:talk_name.png}"
             "job{enter}{sleep:3000}{shot:talk_job.png}bye{enter}{sleep:3000}{shot:talk_bye.png}{quit}",
             ["talk_greet.png", "talk_name.png", "talk_job.png", "talk_bye.png"], 240),
    # "paula-abbey-strong" fixture = paula-abbey with str/dex/int 40 and 999 HP: attack an abbey guard, which starts combat
    "combat": ("j{sleep:6000}{left}{sleep:2000}e{sleep:8000}" + "{up}{sleep:700}" * 4 + "{left}{sleep:700}a{sleep:500}{left}{sleep:6000}"
               "{shot:combat_start.png}a{sleep:500}{up}{sleep:5000}{shot:combat_attack.png}p{sleep:3000}{shot:combat_pass.png}"
               "a{sleep:500}{up}{sleep:5000}{shot:combat_round2.png}{quit}",
               ["combat_start.png", "combat_attack.png", "combat_pass.png", "combat_round2.png"], 300),
    # location-patched fixtures (tools/make_fixture.py): gate-compassion = standing on the Shrine of Compassion tile,
    # gate-britain = on Britain's town tile, dungeon-deceit = inside Deceit (level 1, facing east), all with a strong party
    "shrine": ("j{sleep:6000}e{sleep:8000}{shot:shrine_enter.png}compassion{enter}{sleep:4000}mu{enter}{sleep:2000}mu{enter}{sleep:2000}"
               "mu{enter}{sleep:3000}{shot:shrine_mantra.png}3{enter}{sleep:30000}{shot:shrine_quest.png}{quit}",
               ["shrine_enter.png", "shrine_mantra.png", "shrine_quest.png"], 300),
    "dungeon": ("j{sleep:8000}i{sleep:2000}{shot:dungeon_dark.png}{up}{sleep:1500}{shot:dungeon_walk.png}{left}{sleep:1500}{up}{sleep:1500}"
                "{shot:dungeon_turn.png}{right}{sleep:1500}{right}{sleep:1500}{shot:dungeon_turn2.png}k{sleep:1500}{shot:dungeon_klimb.png}{quit}",
                ["dungeon_dark.png", "dungeon_walk.png", "dungeon_turn.png", "dungeon_turn2.png", "dungeon_klimb.png"], 240),
    "britain": ("j{sleep:6000}e{sleep:8000}{shot:britain_enter.png}{up}{sleep:700}{up}{sleep:700}{shot:britain_gate.png}"
                "t{sleep:500}{left}{sleep:2500}{shot:britain_talk.png}{down}{sleep:700}{down}{sleep:700}{down}{sleep:700}{shot:britain_street.png}{quit}",
                ["britain_enter.png", "britain_gate.png", "britain_talk.png", "britain_street.png"], 240),
    # a tour of the one-key commands in the start room (prompts are dismissed with Esc); checks for crashes/hangs
    "commands": ("j{sleep:6000}i{sleep:800}s{sleep:300}{up}{sleep:800}g{sleep:300}{up}{sleep:800}p{sleep:300}{up}{sleep:800}{shot:cmd1.png}"
                 "u{sleep:600}{esc}{sleep:600}r{sleep:600}{esc}{sleep:600}w{sleep:600}{esc}{sleep:600}v{sleep:800}{shot:cmd2.png}"
                 "m{sleep:800}{esc}{sleep:600}c{sleep:800}{esc}{sleep:600}y{sleep:800}{enter}{sleep:600}n{sleep:800}{esc}{sleep:600}{shot:cmd3.png}"
                 "h{sleep:800}{esc}{sleep:600}{shot:cmd4.png}{quit}", ["cmd1.png", "cmd2.png", "cmd3.png", "cmd4.png"], 240),
    # "gate-britain-magic" = gate-britain plus 9 of every spell mixture and equipment item: cast several spells in Britain
    # (spell words are typed by initial letter: il = In Lor (light), an = An Nox (cure), im = In Mani (heal), ...)
    # spell letters per spell (index into D_1c30) that work in a town; each is cast by player 1 ('1' then Enter confirms the player)
    "spells": ("j{sleep:6000}e{sleep:8000}" + "".join("c{sleep:700}1{enter}{sleep:700}%s{enter}{sleep:1500}%s{sleep:1800}{shot:spell%d.png}" % (sp, extra, i)
               for i, (sp, extra) in enumerate(SPELLS_TOWN)) + "{quit}",
               ["spell%d.png" % i for i in range(len(SPELLS_TOWN))], 400),
    # pristine = the shipped INIT.GAM (no character): the intro menu's Create New Character flow, name "zed", male, answering
    # the gypsy's seven virtue questions a/b alternately; then Ultima V Introduction ('u') and Acknowledgements ('a')
    "create": ("j{sleep:3000}{space}{sleep:3000}c{sleep:3000}zed{enter}{sleep:3000}{shot:create_name.png}m{sleep:3000}{enter}{sleep:3000}"
               "{shot:create_q0.png}" + "".join("%s{sleep:3000}{shot:create_q%d.png}{enter}{sleep:2000}" % ("ab"[i % 2], i + 1) for i in range(7)) +
               "{sleep:3000}{shot:create_done.png}{enter}{sleep:3000}{shot:create_game.png}{quit}",
               ["create_name.png", "create_q0.png", "create_q1.png", "create_q7.png", "create_done.png", "create_game.png"], 400),
    "intro-menus": ("j{sleep:3000}{space}{sleep:3000}u{sleep:4000}{shot:intro_u1.png}{space}{sleep:3000}{shot:intro_u2.png}{space}{sleep:3000}"
                    "{shot:intro_u3.png}{esc}{sleep:3000}{shot:intro_menu.png}a{sleep:4000}{shot:intro_ack1.png}{space}{sleep:3000}"
                    "{shot:intro_ack2.png}{quit}", ["intro_u1.png", "intro_u2.png", "intro_u3.png", "intro_menu.png", "intro_ack1.png",
                                                    "intro_ack2.png"], 300),
    # gate-britain-shop = gate-britain-magic with INT 30 (an INT above 33 makes the unsigned price formula overflow, as in the
    # original): walk to Iolo's Bows (path from the TOWNE.DAT map), buy a Bow, then sell a Leather Helm
    "shop": ("j{sleep:6000}e{sleep:8000}" + "{up}{sleep:400}" * 9 + "{left}{sleep:400}" * 10 + "{sleep:1000}{shot:shop_door.png}"
             "t{sleep:500}{up}{sleep:2500}{shot:shop_greet.png}{space}{sleep:2500}{shot:shop_menu.png}b{sleep:3000}{shot:shop_list.png}"
             "c{sleep:3000}{shot:shop_price.png}y{sleep:3000}{shot:shop_bought.png}{space}{sleep:3000}{shot:shop_leave.png}"
             "t{sleep:500}{up}{sleep:2500}{space}{sleep:2500}s{sleep:3000}{shot:sell_pick.png}{enter}{sleep:3000}{shot:sell_offer.png}"
             "y{sleep:3000}{shot:sell_done.png}{quit}",
             ["shop_door.png", "shop_greet.png", "shop_menu.png", "shop_list.png", "shop_price.png", "shop_bought.png", "shop_leave.png",
              "sell_pick.png", "sell_offer.png", "sell_done.png"], 400),
    # merchants by dialog number (129 weapons, 130 tavern, 131 horses, 133 herbalist, 136 inn): {tm:N} walks up to the merchant
    # (they wander with the schedule) and presses T + direction.  gate-moonglow / gate-trinsic are gate-britain-shop moved to
    # those towns' tiles.
    "inn": ("j{sleep:6000}e{sleep:8000}{tm:136}{sleep:3000}{shot:inn_greet.png}y{sleep:3000}{shot:inn_menu.png}r{sleep:3000}{shot:inn_price.png}"
            "y{sleep:6000}{shot:inn_room.png}{sleep:30000}{shot:inn_morning.png}{quit}",
            ["inn_greet.png", "inn_menu.png", "inn_price.png", "inn_room.png", "inn_morning.png"], 400),
    "tavern": ("j{sleep:6000}e{sleep:8000}{tm:130}{sleep:3000}{shot:tavern_greet.png}y{sleep:3000}{shot:tavern_menu.png}a{sleep:3000}"
               "{shot:tavern_ale.png}{enter}{sleep:3000}{shot:tavern_leave.png}{quit}",
               ["tavern_greet.png", "tavern_menu.png", "tavern_ale.png", "tavern_leave.png"], 300),
    "herbalist": ("j{sleep:6000}e{sleep:8000}{tm:133}{sleep:3000}{shot:herb_greet.png}y{sleep:3000}{shot:herb_list.png}a{sleep:3000}"
                  "{shot:herb_ask.png}y{sleep:3000}{shot:herb_bought.png}{enter}{sleep:3000}{shot:herb_leave.png}{quit}",
                  ["herb_greet.png", "herb_list.png", "herb_ask.png", "herb_bought.png", "herb_leave.png"], 300),
    "horses": ("j{sleep:6000}e{sleep:8000}{tm:131}{sleep:3000}{shot:horse_greet.png}y{sleep:3000}{shot:horse_price.png}y{sleep:3000}"
               "{shot:horse_bought.png}{quit}", ["horse_greet.png", "horse_price.png", "horse_bought.png"], 300),
    "journey": ("j{sleep:1500}{shot:noactive.png}{quit}", ["noactive.png"], 120),
}
# shop conversations started directly with the U5_MERCHANT test hook (the merchants of Minoc/Skara Brae are awkward to walk to):
# fixtures put the party in the town (map 5 = Minoc, 8 = Skara Brae); gate-minoc-hurt has a poisoned, a dead and a wounded member and 999 gold,
# gate-minoc-rich has 5000 gold, INT 20 (INT 40 makes the original price formula overflow)
def _merchant(name, dlg, fixture, body, shots):
    SCENARIOS[name] = ("j{sleep:6000}" + body + "{quit}", ["%s%d.png" % (name, i) for i in range(1, shots + 1)], 400)
    SCENARIO_ENV[name] = {"U5_MERCHANT": str(dlg)}
    MERCHANT_FIXTURES[name] = fixture

MERCHANT_FIXTURES = {}
_merchant("healer", 135, "gate-minoc-hurt", "y{sleep:2500}c{sleep:2500}1{enter}{sleep:2500}{shot:healer1.png}y{sleep:2500}h{sleep:2500}3{enter}{sleep:2500}{shot:healer2.png}y{sleep:2500}r{sleep:2500}2{enter}{sleep:2500}{shot:healer3.png}y{sleep:2500}{shot:healer4.png}n{sleep:2500}{shot:healer5.png}", 5)
_merchant("shipwright", 132, "gate-minoc-rich", "y{sleep:2500}f{sleep:2500}{shot:shipwright1.png}y{sleep:3000}{shot:shipwright2.png}", 2)
_merchant("shipwright-skiff", 132, "gate-minoc-rich", "y{sleep:2500}s{sleep:2500}{shot:shipwright-skiff1.png}y{sleep:3000}{shot:shipwright-skiff2.png}", 2)
_merchant("shipwright-poor", 132, "gate-minoc-hurt2", "y{sleep:2500}f{sleep:2500}y{sleep:3000}{shot:shipwright-poor1.png}", 1)
_merchant("guild", 134, "gate-town8-in2", "y{sleep:2500}a{sleep:2500}{shot:guild1.png}y{sleep:2500}{shot:guild2.png}b{sleep:2500}{shot:guild3.png}y{sleep:2500}c{sleep:2500}y{sleep:2500}{shot:guild4.png}n{sleep:2500}{shot:guild5.png}", 5)
_merchant("reagents", 133, "gate-britain-in", "y{sleep:2500}a{sleep:2500}y{sleep:2500}{shot:reagents1.png}b{sleep:2500}y{sleep:2500}c{sleep:2500}n{sleep:2500}{shot:reagents2.png}{enter}{sleep:2500}{shot:reagents3.png}", 3)
_merchant("tavern2", 130, "gate-britain-in", "y{sleep:2500}a{sleep:2500}y{sleep:2500}r{sleep:2500}2{enter}{sleep:2500}{shot:tavern21.png}y{sleep:2500}c{sleep:2500}job{enter}{sleep:2500}{shot:tavern22.png}n{sleep:2500}{shot:tavern23.png}", 3)

# gate-bt-weak = at the Palace of Blackthorn on the overworld with a 1 HP party: {go} into the palace gets the party caught by the guards
# and Blackthorn asks for the mantra of Honesty (the BLCKTHRN capture sequence); AHM is the right answer
SCENARIOS["blackthorn"] = ("j{sleep:6000}e{sleep:9000}{shot:bt_gate.png}{go:21,25}{sleep:3000}{shot:bt_guards.png}{space}{sleep:2500}{space}{sleep:2500}{shot:bt_question.png}"
                           "ahm{enter}{sleep:3000}{shot:bt_answer.png}{space}{sleep:3000}{shot:bt_after.png}{quit}",
                           ["bt_gate.png", "bt_guards.png", "bt_question.png", "bt_answer.png", "bt_after.png"], 600)

# U5_ENDGAME=1 (test hook in ultima.c) plays the ending right after the intro; the fixture is any strong party
SCENARIOS["endgame"] = ("j{sleep:4000}{shot:end0.png}{space}{sleep:2000}{shot:end1.png}y{sleep:6000}{shot:end2.png}" + "".join("{space}{sleep:3000}{shot:end%d.png}" % n for n in range(3, 14)) + "{quit}", ["end%d.png" % n for n in range(14)], 400)

# gate-lb = at the door of the Castle of Lord British; attacking a guard inside gets the party captured by Blackthorn (BLCKTHRN Capture)
_CAP_RND = "a{sleep:500}{left}{sleep:2500}" + "{space}{sleep:1200}" * 3
SCENARIOS["capture"] = ("j{sleep:6000}e{sleep:9000}{up}{sleep:800}{up}{sleep:800}{left}{sleep:800}a{sleep:600}{left}{sleep:3000}" + _CAP_RND * 10 + "{shot:capture.png}{quit}", ["capture.png"], 900)
# gate-dead = whole party dead on the overworld: one move triggers the Death sequence and the rescue
SCENARIOS["death"] = ("j{sleep:6000}{left}{sleep:3000}{shot:death1.png}{sleep:8000}{shot:death2.png}{quit}", ["death1.png", "death2.png"], 300)

_COMBAT_PRE = "j{sleep:6000}{left}{sleep:2000}e{sleep:8000}" + "{up}{sleep:700}" * 4 + "{left}{sleep:700}a{sleep:500}{left}{sleep:6000}"
SCENARIOS["cspells"] = (_COMBAT_PRE + "".join("c{sleep:800}%s{enter}{sleep:1500}%s{sleep:1500}{space}{sleep:600}{space}{sleep:600}{space}{sleep:600}%s" % (sp, x, "{shot:csp%d.png}" % i if i % 4 == 0 else "")
                                              for i, (sp, x) in enumerate(SPELLS_COMBAT)) + "{shot:csp_end.png}{quit}",
                        ["csp%d.png" % i for i in range(0, len(SPELLS_COMBAT), 4)] + ["csp_end.png"], 1200)

SCENARIOS["dspells"] = ("j{sleep:7000}" + "".join("%s{sleep:1500}{shot:dpre%d.png}" % (c, i) for i, c in enumerate(DUNGEON_PRE)) +
                       "".join("c{sleep:700}1{enter}{sleep:700}%s{enter}{sleep:1500}%s{sleep:1500}{shot:dsp%d.png}" % (sp, x, i)
                               for i, (sp, x) in enumerate(SPELLS_DUNGEON)) + "{quit}",
                       ["dpre%d.png" % i for i in range(len(DUNGEON_PRE))] + ["dsp%d.png" % i for i in range(len(SPELLS_DUNGEON))], 900)

SCENARIOS["use"] = ("j{sleep:6000}" + "".join("u{sleep:1000}" + "{down}{sleep:150}" * n + "{enter}{sleep:1800}%s{sleep:1200}{shot:use%d.png}" % (x, n)
                    for n, x in USE_ITEMS) + "{quit}", ["use%d.png" % n for n, _ in USE_ITEMS], 600)

for _v, _c in VEHICLE_CMDS.items():
    SCENARIOS[_v] = ("j{sleep:6000}" + "".join("%s{sleep:1500}{shot:v_%s%d.png}" % (c, _v, i) for i, c in enumerate(_c)) + "{quit}",
                     ["v_%s%d.png" % (_v, i) for i in range(len(_c))], 300)



def load_png_rgb(path):
    """Decode the PNGs written by GRAP_WIN32_SaveScreenshot (8-bit RGB, filter 0)."""
    data = open(path, "rb").read()
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    pos, idat, w, h = 8, b"", 0, 0
    while pos < len(data):
        n, typ = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        if typ == b"IHDR":
            w, h = struct.unpack(">II", body[:8])
        elif typ == b"IDAT":
            idat += body
        pos += 12 + n
    raw = zlib.decompress(idat)
    return w, h, raw


def distinct_colors(path):
    w, h, raw = load_png_rgb(path)
    stride = 1 + w * 3
    colors = set()
    for y in range(h):
        row = raw[y * stride + 1:(y + 1) * stride]
        for x in range(0, len(row), 3):
            colors.add(row[x:x + 3])
    return len(colors)


GOLDEN = os.path.join(ROOT, "tests", "golden.json")
# regions that legitimately vary between runs: wind banner, text cursor line
MASKS = [(96, 362, 300, 388), (384, 340, 640, 392)]


def screen_hash(path):
    w, h, raw = load_png_rgb(path)
    stride = 1 + w * 3
    rows = []
    for y in range(h):
        row = bytearray(raw[y * stride + 1:(y + 1) * stride])
        for x0, y0, x1, y1 in MASKS:
            if y0 <= y < y1:
                row[x0 * 3:x1 * 3] = bytes((x1 - x0) * 3)
        rows.append(bytes(row))
    return hashlib.sha1(b"".join(rows)).hexdigest()


# Real-game references (DOSBox window captures supplied by the project owner); the 640x400 game area starts at (1,23).
# Map viewport differences are animated tiles (water, torches), so the allowance is a small fraction; the panels are static.
REFERENCES = [("abbey_hall.png", "abbey_hall_ega.png", (1, 23),
               [("map viewport", (16, 16, 372, 372), 0.025), ("party list", (384, 0, 640, 120), 0.0), ("stats", (384, 120, 640, 170), 0.0)])]


def check_references():
    import refdiff
    bad = []
    for ours, ref, (ox, oy), boxes in REFERENCES:
        po = os.path.join(RUN, ours)
        if not os.path.exists(po):
            continue
        a, b = refdiff.dec(os.path.join(ROOT, "tests", "reference", ref)), refdiff.dec(po)
        for name, box, allow in boxes:
            n = refdiff.region_diff(a, b, ox, oy, box)
            area = (box[2] - box[0]) * (box[3] - box[1])
            if n > allow * area:
                bad.append("%s vs %s: %s differs in %d of %d pixels" % (ours, ref, name, n, area))
    return bad


def install_fixture(name):
    """Copy tests/fixtures/<name>/* over the saves in run/SAVEGAME (scenario starts from that character)."""
    src = os.path.join(ROOT, "tests", "fixtures", name)
    for f in os.listdir(src):
        shutil.copy2(os.path.join(src, f), os.path.join(RUN, "SAVEGAME", f))


def run_scenario(name, keys, shots, timeout, golden=None, update=None):
    env = dict(os.environ, U5_HEADLESS="1", U5_FAST="1", U5_SEED="7", U5_VCLOCK="1", U5_KEYS=keys)
    if name == "endgame":
        env["U5_ENDGAME"] = "1"
    env.update(SCENARIO_ENV.get(name, {}))
    for s in shots:
        p = os.path.join(RUN, s)
        if os.path.exists(p):
            os.remove(p)
    try:
        r = subprocess.run([EXE], cwd=RUN, env=env, timeout=timeout, capture_output=True)
    except subprocess.TimeoutExpired:
        return "TIMEOUT after %ds" % timeout
    if r.returncode != 0:
        return "exit code %d" % r.returncode
    for s in shots:
        p = os.path.join(RUN, s)
        if not os.path.exists(p):
            return "missing screenshot " + s
        if distinct_colors(p) < 2:
            return "blank screenshot " + s
        if golden is not None and s in golden:
            if screen_hash(p) != golden[s]:
                return "screenshot %s differs from golden" % s
        elif update is not None:
            update[s] = screen_hash(p)
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game-dir", default="C:/games/ultima5")
    ap.add_argument("--update-golden", action="store_true", help="record screenshot hashes in tests/golden.json")
    ap.add_argument("scenarios", nargs="*")
    args = ap.parse_args()

    if not os.path.exists(EXE):
        sys.exit("build/u5.exe not found; run mingw32-make first")
    subprocess.run([sys.executable, os.path.join(ROOT, "tools", "u5tool.py"), "rundir", args.game_dir, RUN], check=True)

    golden = json.load(open(GOLDEN)) if os.path.exists(GOLDEN) else {}
    update = {} if args.update_golden else None
    failed = 0
    for name in args.scenarios or SCENARIOS:
        keys, shots, timeout = SCENARIOS[name]
        if name in ("play", "stats", "look"):
            install_fixture("paula")
        elif name in ("create", "intro-menus"):
            install_fixture("pristine")
        elif name in ("inn", "tavern"):
            install_fixture("gate-britain-shop")
        elif name == "herbalist":
            install_fixture("gate-moonglow")
        elif name == "horses":
            install_fixture("gate-trinsic")
        elif name == "shop":
            install_fixture("gate-britain-shop")
        elif name == "spells":
            install_fixture("gate-britain-caster")
        elif name in VEHICLE_CMDS:
            install_fixture("gate-vehicle-" + name)
        elif name == "use":
            install_fixture("gate-items")
        elif name == "dspells":
            install_fixture("dungeon-caster")
        elif name == "cspells":
            install_fixture("abbey-caster")
        elif name == "capture":
            install_fixture("gate-lb")
        elif name == "death":
            install_fixture("gate-dead")
        elif name == "endgame":
            install_fixture("gate-items")
        elif name == "blackthorn":
            install_fixture("gate-bt-weak")
        elif name in MERCHANT_FIXTURES:
            install_fixture(MERCHANT_FIXTURES[name])
        elif name == "shrine":
            install_fixture("gate-compassion")
        elif name == "dungeon":
            install_fixture("dungeon-deceit")
        elif name == "britain":
            install_fixture("gate-britain")
        elif name == "commands":
            install_fixture("paula")
        elif name == "combat":
            install_fixture("paula-abbey-strong")
        elif name == "talk":
            install_fixture("paula-jester")
        elif name == "abbey":
            install_fixture("paula-abbey")
        elif name == "overworld":
            install_fixture("paula-outside")
        err = run_scenario(name, keys, shots, timeout, None if args.update_golden else golden, update)
        print("%-8s %s" % (name, "ok" if err is None else "FAIL: " + err))
        failed += err is not None
    if not args.update_golden:
        for msg in check_references():
            print("REFERENCE MISMATCH: " + msg)
            failed += 1
    if update:
        golden.update(update)
        json.dump(golden, open(GOLDEN, "w"), indent=1, sort_keys=True)
        print("recorded %d golden hashes" % len(update))
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
