#!/usr/bin/env python3
"""
For every high-confidence, non-library match in matches.json, try to pull
the exact C/C++ function signature out of the reference source and turn it
into a prototype string IDA can apply (via apply_prototypes.py). This is
aim #3 of the project: make the IDB carry real types/prototypes so later
Hex-Rays/manual decompilation reads close to the original C.

Output: reversing/analysis/prototypes.json
  { asm_or_new_name: {"prototype": "<ret> <name>(<params>)", "source_symbol": "...",
                       "source_file": "...", "status": "ok"} , ... }
  Entries that couldn't be confidently extracted get "status": "failed" with
  a "reason", rather than being silently dropped -- so it's obvious what
  still needs a human to fill in.

Flat C++ member-function names (ClassName__Method convention used elsewhere
in this project) are mapped back to ClassName::Method for searching the
source; SpriteCache__operator_index is special-cased to `operator []`.
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
ANALYSIS = Path(__file__).resolve().parent.parent / "analysis"

# Explicit map for the ClassName__Method flat names this project has hand-
# assigned (see matches.json "manual" entries) -- NOT a generic "__" heuristic,
# since plenty of real source identifiers contain "__" without being ours
# (__actual_invscreen, __GetLocationType, __find_route, etc.) and a generic
# split would mangle those.
FLAT_CPP_NAMES = {
    "GUIButton__Draw": ("GUIButton::Draw", "Draw"),
    "GUIMain__rebuild_array": ("GUIMain::rebuild_array", "rebuild_array"),
    "TreeMap__addText": ("TreeMap::addText", "addText"),
    "InterfaceElement__InterfaceElement": ("InterfaceElement::InterfaceElement", "InterfaceElement"),
    "SpriteCache__loadSprite": ("SpriteCache::loadSprite", "loadSprite"),
    "SpriteCache__operator_index": ("SpriteCache::operator[]", "operator"),
    "SystemImports__add": ("SystemImports::add", "add"),
    "SystemImports__get_index_of": ("SystemImports::get_index_of", "get_index_of"),
    "SystemImports__remove_range": ("SystemImports::remove_range", "remove_range"),
    "SystemImports__is_script_import": ("SystemImports::is_script_import", "is_script_import"),
    "SystemImports__get_addr_of": ("SystemImports::get_addr_of", "get_addr_of"),
    "SpriteCache__precache": ("SpriteCache::precache", "precache"),
    "GUIButton__MouseDown": ("GUIButton::MouseDown", "MouseDown"),
    "GUIButton__MouseUp": ("GUIButton::MouseUp", "MouseUp"),
    "GUIButton__ReadFromFile": ("GUIButton::ReadFromFile", "ReadFromFile"),
    "GUIButton__MouseMove": ("GUIButton::MouseMove", "MouseMove"),
    "GUIButton__KeyPress": ("GUIButton::KeyPress", "KeyPress"),
    "GUIButton__WriteToFile": ("GUIButton::WriteToFile", "WriteToFile"),
    "GUIMain__get_control_type": ("GUIMain::get_control_type", "get_control_type"),
    "GUIListBox__Clear": ("GUIListBox::Clear", "Clear"),
    "GUIListBox__AddItem": ("GUIListBox::AddItem", "AddItem"),
}

# Functions where this build's ACTUAL disassembly parameter count/types are
# independently confirmed (via the stack-frame declaration + every real call
# site's own push count -- not just an assumption) to genuinely diverge from
# the naive reference-source-extracted signature above. Applying the naive
# extraction here via apply_prototypes.py would misread argument slots or
# assert parameters that don't exist in this 2002 build. Keyed by display
# name (same key as `results`); each override REPLACES the auto-extracted
# entry wholesale after the main extraction loop, so it survives a fresh
# `extract_prototypes.py` re-run rather than being silently clobbered by it.
KNOWN_SIGNATURE_OVERRIDES = {
    "_display_main": {
        "status": "ok",
        "prototype": "int _display_main(int xx, int yy, int wii, char *todis, int blocking, int usingfont, int asspch)",
        "source_symbol": "_display_main",
        "source_file": "Engine/AC.CPP",
        "note": (
            "2011 declares 10 params (AC.CPP:12819), but this build's own stack frame "
            "has exactly 7 argument slots (+8..+0x20), and BOTH real call sites "
            "(_display_at, CreateTextOverlay -- the only two callers) push exactly 7 "
            "arguments each, matching 2011's first 7 params in order. The trailing "
            "isThought/allowShrink/overlayPositionFixed parameters are CONFIRMED ABSENT "
            "from this build entirely, not merely defaulted. See matches.json's own "
            "sub_4136F6 entry and reversing/notes/struct-layout-drift.md for the full "
            "correction writeup (a prior round had wrongly assumed a 10-param signature "
            "without checking the frame declaration)."
        ),
    },
}


def flat_to_source_symbol(name):
    if name in FLAT_CPP_NAMES:
        return FLAT_CPP_NAMES[name]
    return name, name


OPERATOR_INDEX_RE = re.compile(r'operator\s*\[\s*\]\s*\(')


def find_definition(text, search_name, bare_name):
    """
    Find `search_name(` (optionally `Class::method(`) followed eventually by
    a `{` (a definition, not just a call/prototype), using paren-depth
    matching so multi-line parameter lists work. Returns (start_idx, paren_start,
    close_paren_idx) or None.

    Two passes: first require a "Class::" qualifier immediately before the
    match (out-of-line definitions); if that finds nothing, fall back to an
    unqualified match (inline-defined methods never write "Class::" in
    source at all, e.g. TreeMap::addText is defined inside the struct body
    as plain "addText").
    """
    if bare_name == "operator":
        candidates = list(OPERATOR_INDEX_RE.finditer(text))
        match_starts = [(m.start(), m.end() - 1) for m in candidates]
    else:
        pattern = re.compile(r'(?<![A-Za-z0-9_])' + re.escape(bare_name) + r'\s*\(')
        match_starts = [(m.start(), m.end() - 1) for m in pattern.finditer(text)]

    def try_matches(require_qualifier):
        for name_start, paren_start in match_starts:
            if "::" in search_name:
                cls = search_name.split("::")[0]
                prefix_window = text[max(0, name_start - len(cls) - 4):name_start]
                has_qualifier = (cls + "::") in prefix_window
                if require_qualifier and not has_qualifier:
                    continue
                if not require_qualifier and has_qualifier:
                    continue  # already tried in the qualified pass
            elif require_qualifier:
                continue
            depth = 1
            i = paren_start + 1
            while i < len(text) and depth > 0:
                if text[i] == '(':
                    depth += 1
                elif text[i] == ')':
                    depth -= 1
                i += 1
            if depth != 0:
                continue
            close_paren = i - 1
            j = close_paren + 1
            while j < len(text) and text[j] in ' \t\r\n':
                j += 1
            if text[j:j+5] == 'const':
                j += 5
                while j < len(text) and text[j] in ' \t\r\n':
                    j += 1
            if j < len(text) and text[j] == '{':
                return name_start, paren_start, close_paren
        return None

    return try_matches(require_qualifier=True) or try_matches(require_qualifier=False)


BLOCK_COMMENT_RE = re.compile(r'/\*.*?\*/', re.DOTALL)
LINE_COMMENT_RE = re.compile(r'//[^\n]*')
PREPROC_LINE_RE = re.compile(r'^\s*#.*$', re.MULTILINE)


def extract_prefix(text, name_start):
    """Walk backward from the function name to the start of its declaration
    (previous `;`, `}`, or blank line), to recover the return type. Strips
    comments and preprocessor lines that can get swept up in that span."""
    i = name_start
    while i > 0:
        c = text[i - 1]
        if c in ';}':
            break
        if c == '\n' and i - 2 >= 0 and text[i - 2] == '\n':
            break
        i -= 1
    raw = text[i:name_start]
    raw = BLOCK_COMMENT_RE.sub(' ', raw)
    raw = LINE_COMMENT_RE.sub(' ', raw)
    raw = PREPROC_LINE_RE.sub(' ', raw)
    # keep only the last line-ish chunk of what's left (return type is
    # always immediately adjacent to the function name, so trailing
    # whitespace-separated leftovers from stripped comments/preproc on
    # earlier lines can be dropped)
    lines = [l for l in raw.splitlines() if l.strip()]
    return " ".join(lines).strip()


def clean_params(params_text):
    """Strip default-value initializers ("int x = 99999" -> "int x") since
    IDA's type parser chokes on them in some contexts; keep param names."""
    parts = []
    depth = 0
    cur = ""
    for c in params_text:
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
        if c == ',' and depth == 0:
            parts.append(cur)
            cur = ""
        else:
            cur += c
    if cur.strip():
        parts.append(cur)
    cleaned = []
    for p in parts:
        p = p.strip()
        if not p or p == "void":
            continue
        eq = p.find('=')
        if eq != -1:
            p = p[:eq].strip()
        cleaned.append(p)
    return ", ".join(cleaned) if cleaned else "void"


def main():
    matches = json.loads((ANALYSIS / "matches.json").read_text(encoding="utf-8"))
    source_cache = {}
    results = {}

    for entry in matches:
        if entry.get("is_library"):
            continue
        if entry.get("confidence") not in ("high",):
            continue
        source_file = entry.get("source_file")
        if not source_file or isinstance(source_file, list):
            continue

        display_name = entry.get("new_name") or entry["asm_name"]
        search_name, bare_name = flat_to_source_symbol(display_name)

        path = ROOT / source_file
        if not path.exists():
            results[display_name] = {"status": "failed", "reason": f"source file not found: {source_file}"}
            continue
        if path not in source_cache:
            source_cache[path] = path.read_text(encoding="utf-8", errors="replace")
        text = source_cache[path]

        found = find_definition(text, search_name, bare_name)
        if not found:
            results[display_name] = {"status": "failed", "reason": "definition not found in source", "source_file": source_file}
            continue

        name_start, paren_start, close_paren = found
        prefix = extract_prefix(text, name_start)
        params_text = text[paren_start + 1:close_paren]
        params = clean_params(params_text)

        # prefix may include "ClassName::" for out-of-line methods -- strip
        # trailing "ClassName::" from the return-type prefix if present,
        # since IDA prototype strings for our flat names shouldn't include it
        ret_type = prefix
        if "::" in ret_type:
            ret_type = re.sub(r'\b\w+::\s*$', '', ret_type).strip()
        if not ret_type:
            ret_type = "void"

        if display_name in FLAT_CPP_NAMES:
            # C++ member function: the source signature omits the implicit
            # `this` pointer and uses __thiscall under MSVC/x86 -- without
            # both, IDA will misread argument slots by one. Add them
            # explicitly so the applied type is actually correct, not just
            # documentary.
            cls = FLAT_CPP_NAMES[display_name][0].split("::")[0]
            this_param = f"{cls} *this"
            params = this_param if params == "void" else f"{this_param}, {params}"
            prototype = f"{ret_type} __thiscall {display_name}({params})"
        else:
            prototype = f"{ret_type} {display_name}({params})"
        results[display_name] = {
            "status": "ok",
            "prototype": prototype,
            "source_symbol": search_name,
            "source_file": source_file,
        }

    for display_name, override in KNOWN_SIGNATURE_OVERRIDES.items():
        if display_name in results:
            results[display_name] = override

    ANALYSIS.mkdir(parents=True, exist_ok=True)
    (ANALYSIS / "prototypes.json").write_text(json.dumps(results, indent=1), encoding="utf-8")

    ok = sum(1 for v in results.values() if v["status"] == "ok")
    failed = sum(1 for v in results.values() if v["status"] == "failed")
    print(f"Extracted {ok} prototypes, {failed} failed, out of {len(results)} candidates")


if __name__ == "__main__":
    main()
