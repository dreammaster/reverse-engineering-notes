"""
KEH.EXE: define game structs in the IDB and apply them to `[reg+disp]` operands.

Phase 1 (always): create/refresh structs and rename PartyMember/ItemSlot fields.
Phase 2: for every function, track which registers were loaded from which pointer source
(stack variable, known global, party table, array element computed with imul+add) and infer
a struct for each source from the displacements used through it. Only unambiguous matches
(or ones forced by HINTS / FUNC_HINTS) are applied with op_stroff.

    run_ida_script.ps1 -Idb KEH -ScriptName apply_structs_keh.py            (apply + export)
    set REPORT_ONLY = False below to only write structs_report_KEH.txt (no operand changes)
"""
import os
import re
import idc
import idautils
import ida_bytes
import ida_struct
import ida_funcs

REPORT_ONLY = False
HERE = os.path.dirname(os.path.abspath(__file__))

B, W, D = "b", "w", "d"

# name -> (size, [(offset, member, kind, count)])    kind b/w/d, count>1 = array of that kind
STRUCTS = {
    "MapHeader": (0x414, [
        (0x00, "width", W, 1), (0x02, "height", W, 1), (0x04, "flagsWord", W, 1),
        (0x06, "minutesPerStep", B, 1), (0x07, "encounterPct", B, 1), (0x08, "encounterGroups", B, 1),
        (0x09, "monsterCount", B, 1), (0x0B, "groupCount", B, 1), (0x0D, "offMapMsg", B, 1),
        (0x1E, "storedMembersOfs", W, 1),
        (0x404, "shopsOfs", W, 1), (0x406, "monstersOfs", W, 1), (0x408, "monstersEndOfs", W, 1),
        (0x40A, "groupsOfs", W, 1), (0x40C, "groundItemsOfs", W, 1), (0x40E, "groundItemCount", W, 1),
        (0x410, "groundItemLimit", W, 1), (0x412, "groundItemGap", W, 1),
    ]),
    "MapCell": (8, [
        (0, "tile", W, 1), (2, "flags", B, 1), (3, "b3", B, 1), (4, "w4", W, 1), (6, "script", W, 1),
    ]),
    "Monster": (0x68, [
        (0x00, "name", B, 24), (0x18, "attr", B, 7), (0x44, "hp", W, 1), (0x46, "hpMax", W, 1),
        (0x48, "profession", B, 1), (0x49, "id", B, 1), (0x4A, "readied", B, 1), (0x4B, "armor", B, 3),
        (0x4E, "rank", B, 1), (0x50, "flags", B, 1), (0x52, "itemCount", B, 1), (0x53, "itemIds", B, 8),
        (0x5B, "x", B, 1), (0x5C, "y", B, 1), (0x5D, "state", B, 1), (0x5E, "b5E", B, 1),
        (0x5F, "radius", B, 1), (0x60, "targetX", B, 1), (0x61, "targetY", B, 1), (0x62, "b62", B, 1),
        (0x63, "portrait", B, 1), (0x64, "moveAccum", B, 1), (0x65, "overlayId", B, 1), (0x66, "script", W, 1),
    ]),
    "GroupSlot": (0x16, [
        (0x00, "enemyId", B, 1), (0x02, "hp", W, 6), (0x0E, "initCount", B, 1), (0x0F, "count", B, 1),
        (0x10, "targetX", B, 1), (0x11, "targetY", B, 1), (0x12, "spreadPct", B, 1), (0x13, "portrait", B, 1),
    ]),
    "EnemyGroup": (0x66, [
        (0x00, "x0", B, 1), (0x01, "y0", B, 1), (0x02, "x", B, 1), (0x03, "y", B, 1),
        (0x04, "reach", B, 1), (0x05, "b5", B, 1), (0x06, "flags", B, 1), (0x07, "b7", B, 1),
        (0x08, "slotCount", B, 1), (0x09, "activeMask", B, 1), (0x0A, "moveAccum", B, 1),
        (0x0B, "speed", B, 1), (0x0C, "overlayId", B, 1), (0x0D, "spawnDelay", B, 1),
        (0x0E, "slots", B, 0x58),
    ]),
    "GroundItem": (6, [
        (0, "qty", W, 1), (2, "x", B, 1), (3, "yAndHidden", B, 1), (4, "item", B, 1),
    ]),
    "Combatant": (12, [
        (0, "type", B, 1), (1, "index", B, 1), (2, "sub", B, 1), (3, "targetType", B, 1),
        (4, "targetIndex", B, 1), (5, "targetSub", B, 1), (6, "command", B, 1), (7, "speed", B, 1),
        (8, "distance", B, 1), (9, "flags", B, 1), (10, "b10", B, 1), (11, "b11", B, 1),
    ]),
    "Shop": (0x2C, [(0x14, "title", B, 20), (0x28, "portrait", B, 1), (0x2A, "count", W, 1)]),
    "ShopRow": (0x36, [(0x2C, "item", B, 1), (0x2D, "stock", B, 1), (0x2E, "sellPrice", D, 1), (0x32, "buyPrice", D, 1)]),
    "ServiceRow": (0x4C, [(0x40, "effectKind", B, 1), (0x41, "effectParam", B, 1), (0x42, "cost", D, 1),
                          (0x46, "reqKind", B, 1), (0x47, "reqIdx", B, 1), (0x48, "reqValue", D, 1)]),
    "EnemyDef": (0x40, [
        (0x00, "name", B, 0x2C), (0x2C, "dmgMin", W, 1), (0x2E, "dmgMax", W, 1),
        (0x35, "b35", B, 1), (0x36, "b36", B, 1), (0x37, "b37", B, 1),
        (0x39, "armor", B, 1), (0x3A, "hitPct", B, 1), (0x3B, "b3B", B, 1), (0x3C, "b3C", B, 1),
        (0x3D, "xp", B, 1),
    ]),
    "ItemDef": (0x18, [
        (0x00, "name", B, 0x14), (0x14, "flags", B, 1), (0x15, "weaponDef", B, 1),
        (0x16, "b16", B, 1), (0x17, "uniqueBit", B, 1),
    ]),
    "WeaponDef": (0x3C, [
        (0x14, "b14", B, 1), (0x16, "b16", B, 1), (0x18, "flags", B, 1), (0x19, "b19", B, 1),
        (0x1A, "jamPct", B, 1), (0x1C, "b1C", B, 1), (0x1E, "b1E", B, 1), (0x22, "b22", B, 1),
        (0x28, "b28", B, 16), (0x38, "modeCount", W, 1), (0x3A, "modesOfs", W, 1),
    ]),
    "FireMode": (0x12, [
        (0x00, "hitPct", B, 10), (0x0A, "dmgMin", W, 1), (0x0C, "dmgMax", W, 1),
        (0x0E, "ammoType", B, 1), (0x0F, "clip", B, 1), (0x10, "burst", B, 1), (0x11, "b11", B, 1),
    ]),
}

# (offset -> new name) for existing structs
PARTY_RENAMES = {
    0x49: "rosterId", 0x4A: "readied", 0x4B: "armorSlots", 0x52: "itemCount", 0x53: "mutPending",
    0x54: "hasMutation", 0x55: "mutWeekday", 0x56: "mutHour", 0x57: "mutagenTimer",
    0x58: "status", 0x62: "items", 0x122: "lastHurt", 0x124: "skillUse", 0x134: "skillUse2",
}
ITEMSLOT_RENAMES = {0: "id", 1: "ammoType", 2: "ammoCount", 3: "flags"}

# source text -> struct
HINTS = {"_curShop": "Shop", "_map": "MapHeader", "_monsters": "Monster", "_groups": "EnemyGroup", "_curCell": "MapCell"}
PARTY_SRC = re.compile(r"\[(bx|si|di)-2518h\]|_party\[(bx|si|di)\]|^_party$")
# (function name, source key) -> struct, for cases inference cannot settle
FUNC_HINTS = {
    # party members (pointer taken from the party table or passed as `member`)
    ("isItemEquipped", "var+arg_0"): "PartyMember", ("getMemberCon", "var+member"): "PartyMember",
    ("getArmorClass", "var+member"): "PartyMember", ("exposeToMutagens", "var+arg_0"): "PartyMember",
    ("charInfoInventory", "var+arg_0"): "PartyMember", ("applyItemEffect", "var+arg_0"): "PartyMember",
    ("applyItemEffect", "var+arg_2"): "ItemDef", ("writeMemberCondition", "var+member"): "PartyMember",
    ("characterInfoStats", "var+member"): "PartyMember", ("charInfoConditions", "var+arg_0"): "PartyMember",
    ("isMemberActive", "var+arg_0"): "PartyMember", ("addNpcToParty", "var+arg_0"): "Monster",
    ("addNpcToParty", "var+var_E"): "PartyMember", ("itemActionMenu", "var+var_20"): "ItemDef",
    ("removeMemberAmmo", "var+var_6"): "FireMode", ("chooseAmmoToLoad", "var+var_16"): "FireMode",
    ("rollWeaponVsObject", "var+arg_4"): "FireMode", ("chooseFiringMode", "var+arg_2"): "WeaponDef",
    ("chooseFiringMode", "var+var_6"): "FireMode", ("shopBuy", "var+var_12"): "ShopRow",
    ("shopSell", "var+var_16"): "ShopRow", ("serviceLoop", "var+var_24"): "ServiceRow",
    ("postCombatCleanup", "var+var_32"): "GroupSlot", ("initItemSlot", "var+arg_0"): "ItemSlot",
    ("checkLevelUps", "var+var_8"): "Profession", ("serviceLoop", "var+var_16"): "Profession",
    # attack / combatant records (first 6 bytes: type,idx,sub,targetType,targetIdx,targetSub)
    ("rollAttack", "var+arg_2"): "Combatant", ("resolveAttack", "var+arg_0"): "Combatant",
    ("attackMultiShot", "var+arg_2"): "Combatant", ("selectTargetStats", "var+arg_0"): "Combatant",
    ("attackNearbyTargets", "var+arg_0"): "Combatant", ("findNearbyTarget", "var+arg_0"): "Combatant",
    ("retargetIfDead", "var+arg_0"): "Combatant", ("memberAttack", "var+arg_2"): "Combatant",
    ("removeDefeatedFromMap", "var+arg_0"): "Combatant", ("pickWeakestInGroup", "var+arg_0"): "Combatant",
    ("combatMoveEnemy", "var+arg_0"): "Combatant", ("getCombatantInfo", "var+arg_0"): "Combatant",
    ("getCombatantPos", "var+arg_0"): "Combatant",
    ("cmpCombatantsBySpeedDesc", "var+arg_0"): "Combatant", ("cmpCombatantsBySpeedDesc", "var+arg_2"): "Combatant",
    ("cmpCombatantsBySpeedAsc", "var+arg_0"): "Combatant", ("cmpCombatantsBySpeedAsc", "var+arg_2"): "Combatant",
    ("cmpCombatantsByIndex", "var+arg_0"): "Combatant", ("cmpCombatantsByIndex", "var+arg_2"): "Combatant",
    # enemy groups and slots
    ("rollAttack", "var+var_34"): "GroupSlot", ("rollAttack", "var+var_4E"): "EnemyDef",
    ("rollAttack", "var+var_1E"): "PartyMember", ("chooseTarget", "var+var_C"): "GroupSlot",
    ("combatMoveEnemy", "var+var_E"): "GroupSlot", ("buildCombatants", "var+var_18"): "GroupSlot",
    ("pickWeakestInGroup", "var+var_E"): "GroupSlot", ("attackNearbyTargets", "var+var_12"): "GroupSlot",
    ("getCombatantInfo", "var+var_C"): "GroupSlot", ("getCombatantPos", "var+var_E"): "GroupSlot",
    ("spawnEnemyGroup", "var+arg_0"): "EnemyGroup", ("spawnEnemyGroup", "var+var_8"): "GroupSlot",
    ("spawnEnemyGroup", "var+var_C"): "EnemyDef", ("applyMonsterHitReaction", "var+arg_0"): "Monster",
    ("removeMapEntity", "var+var_E"): "Monster",
    # ground items and map cells
    ("findGroundItemsAt", "var+var_8"): "GroundItem", ("lootCell", "var+var_1A"): "GroundItem",
    ("addGroundItem", "var+var_8"): "GroundItem", ("applyCellChange", "var+arg_0"): "MapCell",
    ("setCellFields", "var+arg_0"): "MapCell", ("markEntityOnMap", "var+var_4"): "MapCell",
    ("pickMapCell", "var+var_16"): "MapCell", ("pickMapCell", "var+var_6"): "MapCell",
}

ALL_CANDIDATES = ["PartyMember", "Monster", "EnemyGroup", "GroundItem", "MapHeader", "MapCell",
                  "Combatant", "EnemyDef", "ItemDef", "WeaponDef", "FireMode", "GroupSlot", "Profession"]
# too generic to infer without a hint (their small offsets match everything)
HINT_ONLY = {"Shop", "ShopRow", "ServiceRow", "MapCell", "Combatant", "GroundItem", "GroupSlot", "FireMode"}

KIND_FLAG = {B: ida_bytes.FF_BYTE, W: ida_bytes.FF_WORD, D: ida_bytes.FF_DWORD}
KIND_SIZE = {B: 1, W: 2, D: 4}


def get_or_make(name):
    sid = ida_struct.get_struc_id(name)
    if sid == idc.BADADDR:
        sid = ida_struct.add_struc(idc.BADADDR, name, False)
    return sid


def add_member(sptr, off, name, kind, count):
    size = KIND_SIZE[kind] * count
    m = ida_struct.get_member(sptr, off)
    if m is not None and m.soff == off:
        ida_struct.set_member_name(sptr, off, name)
        return
    ida_struct.add_struc_member(sptr, name, off, KIND_FLAG[kind] | ida_bytes.FF_DATA, None, size)


def define_structs():
    for sname, (size, members) in STRUCTS.items():
        sid = get_or_make(sname)
        sptr = ida_struct.get_struc(sid)
        for off, mname, kind, count in members:
            add_member(sptr, off, mname, kind, count)
        cur = ida_struct.get_struc_size(sptr)
        if cur < size:
            ida_struct.add_struc_member(sptr, "_end", size - 1, ida_bytes.FF_BYTE | ida_bytes.FF_DATA, None, 1)
        print("struct %s size=%d" % (sname, ida_struct.get_struc_size(ida_struct.get_struc(sid))))
    # PartyMember: tidy up fields
    sid = ida_struct.get_struc_id("PartyMember")
    sptr = ida_struct.get_struc(sid)
    for off, nm in PARTY_RENAMES.items():
        m = ida_struct.get_member(sptr, off)
        if m and m.soff == off:
            ida_struct.set_member_name(sptr, off, nm)
    for off, nm, kind, cnt in [(0x21, "remissionHours", B, 1), (0x22, "lastSkillRoll", B, 1),
                               (0x144, "command", B, 1), (0x145, "cmdArg", B, 1), (0x146, "cmdArg2", B, 1),
                               (0x147, "cmdArg3", B, 1)]:
        add_member(sptr, off, nm, kind, cnt)
    m = ida_struct.get_member(sptr, 0x23)
    if m and m.soff == 0x23:
        ida_struct.set_member_name(sptr, 0x23, "lastSkillIdx")
    # exp fields as dwords, target cell far pointer
    for off, nm in [(0x5A, "exp"), (0x5E, "expNext")]:
        for o2 in (off, off + 2):
            m = ida_struct.get_member(sptr, o2)
            if m and m.soff == o2:
                ida_struct.del_struc_member(sptr, o2)
        ida_struct.add_struc_member(sptr, nm, off, ida_bytes.FF_DWORD | ida_bytes.FF_DATA, None, 4)
    m = ida_struct.get_member(sptr, 0x14B)
    if m and m.soff == 0x14B:
        ida_struct.del_struc_member(sptr, 0x14B)
    add_member(sptr, 0x148, "targetCell", D, 1)
    # ItemSlot (was MemberArray1Entry)
    sid = ida_struct.get_struc_id("MemberArray1Entry")
    if sid != idc.BADADDR:
        ida_struct.set_struc_name(sid, "ItemSlot")
    sid = ida_struct.get_struc_id("ItemSlot")
    sptr = ida_struct.get_struc(sid)
    for off, nm in ITEMSLOT_RENAMES.items():
        m = ida_struct.get_member(sptr, off)
        if m and m.soff == off:
            ida_struct.set_member_name(sptr, off, nm)


# ---- operand inference -------------------------------------------------------------
REGS = ("bx", "si", "di")
ALIASES = {"bx": ("bx", "bl", "bh"), "si": ("si",), "di": ("di",), "ax": ("ax", "al", "ah")}
RO_MNEMS = {"cmp", "test", "push", "call", "jmp"}
DISP_RE = re.compile(r"\[(bx|si|di)\+(?!bx|si|di|bp)")
SRC_VAR_RE = re.compile(r"\[bp([+-][\w]+?)(\+2)?\]")


def struct_members(sname):
    """-> list of (start, end) member ranges and size."""
    sid = ida_struct.get_struc_id(sname)
    sptr = ida_struct.get_struc(sid)
    size = ida_struct.get_struc_size(sptr)
    ranges = []
    off = 0
    while off < size:
        m = ida_struct.get_member(sptr, off)
        if m is not None:
            ranges.append((m.soff, m.eoff))
            off = m.eoff
        else:
            off += 1
    return ranges, size, sid


def valid(sname, disps, cache):
    if sname not in cache:
        cache[sname] = struct_members(sname)
    ranges, size, _ = cache[sname]
    for d in disps:
        if d < 0 or d >= size:
            return False
        if not any(s <= d < e for s, e in ranges):
            return False
    return True


def src_key(opt):
    t = opt.replace("word ptr ", "").replace("dword ptr ", "").replace("byte ptr ", "").replace(" ", "")
    t = t.replace("ss:", "").replace("ds:", "").replace("es:", "")
    m = SRC_VAR_RE.search(t)
    if m:
        return "var" + m.group(1)
    return t


def src_hint(t):
    t2 = t.replace("word ptr ", "").replace("dword ptr ", "").replace(" ", "")
    if PARTY_SRC.search(t2):
        return "PartyMember"
    return HINTS.get(t2)


def process_function(fea, usage, vhint):
    """usage[key] -> list of (ea, opn, disp); vhint[var key] -> struct (from known stores)."""
    regkey = {}
    reghint = {}
    end = idc.get_func_attr(fea, idc.FUNCATTR_END)
    ea = fea
    while ea < end:
        mnem = idc.print_insn_mnem(ea)
        t0 = idc.print_operand(ea, 0)
        t1 = idc.print_operand(ea, 1)
        # uses first (operands read through a tracked register)
        for n, t in ((0, t0), (1, t1)):
            if idc.get_operand_type(ea, n) != idc.o_displ:
                continue
            m = DISP_RE.search(t)
            if not m:
                continue
            r = m.group(1)
            if r in regkey:
                disp = idc.get_operand_value(ea, n)
                usage.setdefault(regkey[r], []).append((ea, n, disp & 0xFFFF))
        # source loads
        dst = t0.replace(" ", "")
        if mnem in ("les", "mov", "lds") and dst in ALIASES and dst in REGS:
            key = None
            h = src_hint(t1)
            if h:
                key = "hint:" + h
            elif t1.startswith("[bp") or "word ptr [bp" in t1 or "[bp" in t1.replace("ss:", ""):
                key = src_key(t1)
            elif re.fullmatch(r"(dword|word) ptr \w+|\w+", t1.replace("ds:", "")) and not t1.startswith("["):
                key = None
            if key is None and dst in ("bx", "si", "di") and t1.replace(" ", "") in REGS:
                key = regkey.get(t1.replace(" ", ""))
            if key:
                regkey[dst] = key
                if key.startswith("var") and key in vhint:
                    regkey[dst] = "hint:" + vhint[key]
            else:
                regkey.pop(dst, None)
            ea = idc.next_head(ea, end)
            continue
        # array element pointer built in ax: add ax, word ptr <hinted global>
        if mnem == "add" and dst == "ax":
            h = src_hint(t1)
            if t1.replace(" ", "") == "414h":
                h = "MapCell"
            if h:
                reghint["ax"] = h
            else:
                reghint.pop("ax", None)
        elif mnem == "mov" and dst == "ax":
            h = src_hint(t1)
            if h:
                reghint["ax"] = h
            elif "ax" in reghint and not t1.replace(" ", "").isdigit():
                reghint.pop("ax", None)
        # store of a hinted ax into a stack variable
        if mnem == "mov" and "ax" in reghint and t1.replace(" ", "") in ("ax",) and "[bp" in t0:
            vhint[src_key(t0)] = reghint["ax"]
        # register writes kill tracking
        if mnem not in RO_MNEMS and not mnem.startswith("j") and not mnem.startswith("loop"):
            for r in REGS:
                if dst in ALIASES.get(r, ()):
                    regkey.pop(r, None)
                    break
        if mnem == "call":
            regkey.pop("bx", None)
        ea = idc.next_head(ea, end)


def decide(fname, key, entries, cache):
    disps = sorted({d for _, _, d in entries})
    forced = None
    if key.startswith("hint:"):
        forced = key[5:]
    elif (fname, key) in FUNC_HINTS:
        forced = FUNC_HINTS[(fname, key)]
    if forced:
        if forced not in cache:
            cache[forced] = struct_members(forced)
        size = cache[forced][1]
        inside = [d for d in disps if d < size]
        return (forced if inside else None), disps, "forced" if len(inside) == len(disps) else "forced-partial"
    cands = [c for c in ALL_CANDIDATES if c not in HINT_ONLY and valid(c, disps, cache)]
    if len(cands) == 1:
        return cands[0], disps, "unique"
    return None, disps, "ambiguous:" + ",".join(cands)


def apply_all():
    cache = {}
    sids = {}
    report = []
    applied = 0
    for fea in idautils.Functions():
        fname = idc.get_func_name(fea)
        usage, vhint = {}, {}
        # two passes so hints discovered late (stores) can type earlier-seen loops
        process_function(fea, {}, vhint)
        process_function(fea, usage, vhint)
        merged = {}
        for key, ents in usage.items():
            merged.setdefault(key, []).extend(ents)
        for key, ents in merged.items():
            sname, disps, why = decide(fname, key, ents, cache)
            report.append("%-28s %-14s %-12s %s %s" % (fname, key, sname or "-", why,
                          " ".join("%X" % d for d in disps)))
            if sname and not REPORT_ONLY:
                if sname not in sids:
                    sids[sname] = ida_struct.get_struc_id(sname)
                for ea, n, d in ents:
                    if d >= cache[sname][1]:
                        continue
                    if idc.op_stroff(ea, n, sids[sname], 0):
                        applied += 1
    with open(os.path.join(HERE, "structs_report_KEH.txt"), "w") as fh:
        fh.write("\n".join(report))
    print("operands typed: %d, keys reported: %d" % (applied, len(report)))


define_structs()
apply_all()
