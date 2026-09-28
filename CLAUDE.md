# Code style: ScummVM conventions

This project's eventual destination is an engine module inside
[ScummVM](https://www.scummvm.org/). All C++ source under `src/` (for
Deponia and any future game sharing this engine) must follow ScummVM's own
[Code Formatting Conventions](https://wiki.scummvm.org/index.php?title=Code_Formatting_Conventions).
This file distills those rules for quick reference and records the
project-specific exception to them (see "Naming - the one deliberate
exception" below). When in doubt, defer to the wiki page itself.

## One overriding exception: preserve recovered identifiers

This codebase is reconstructed from a disassembly of the original
Visionnaire engine binary. Many class and method names are not stylistic
choices - they are the *actual original identifiers*, recovered byte-for-
byte from mangled Itanium ABI symbols (e.g. `_ZN12TGameControl11CenterSceneEv`
demangles to the exact original method name `TGameControl::CenterScene`).

**Do not rename recovered class or method names to match ScummVM's camelCase
convention.** `TGameControl`, `CenterScene`, `HandleMouseMove`,
`SkipCurrentText`, etc. stay exactly as recovered, even though ScummVM's own
convention would write `centerScene`/`handleMouseMove`. This is evidence,
not formatting - renaming it destroys the reverse-engineering record.

Everything **invented** during reconstruction - member variable names, local
variable names, enum value names, helper/free-function names with no
mangled-symbol evidence - follows ScummVM's naming convention in full (see
below). When adding a NOTES.md-style comment that explains how a name was
recovered vs. invented, that comment is what tells a future reader which
rule applied.

## Indentation and braces

- **Tabs**, tab width 4, for indentation. Never spaces for indent depth.
  (Spaces ARE fine for *vertical alignment* within a continuation line, e.g.
  lining up wrapped parameters under an opening paren - see "Vertical
  alignment" below.)
- **Attached ("hugging") braces**: opening brace on the same line as the
  `if`/`for`/`while`/`class`/function signature it belongs to.
  ```cpp
  for (int i = 0; i < t; i++) {
      ...
  }

  if (j < k) {
      ...
  } else {
      ...
  }
  ```
- Single-statement bodies may omit braces, but if/else PAIRS should be
  braced consistently (both sides braced, or neither), and NESTED if/else
  must always be braced to avoid ambiguity.
- **No extra indent after a `namespace` clause.**
- `switch`/`case`: `case` labels sit at the SAME indent level as `switch`,
  not indented further. Mark intentional fallthrough with `// fall through`.

## Whitespace

- Space around binary operators: `a = (b + c) * d;`
- Space between a C++ keyword and `(`: `while (true) {`
- Space after commas: `someFunction(a, b, c);`
- `// ` and `/* ... */` comments have a space after the opening marker.
- Empty `for`/`while` bodies need `{}`, never a bare `;`.
- Space around `:` in class inheritance and in `?:`:
  `class Foo : public Bar {`, `(cond) ? a : b;`
- No space after the array `delete[]` operator's brackets: `delete[] foo;`
- `template<typename T>` - no space between `template` and `<`.
- Operator overloading: no space before `(` in `operator()()`, but a
  conversion operator keeps a space: `operator bool()`.

## Pointers and references

Space **before** `*`/`&`, never after - the operator attaches to the
variable/parameter name, not the type:
```cpp
const char *ptr = (const char *)foobar;
int &ref = i;
```
This applies in variable declarations, return types, AND function
parameters. **Known astyle limitation**: astyle's `align-pointer`/
`align-reference` only fixes declarations and return types, not parameters
- always double check (or re-run the supplementary regex, see below) after
an astyle pass.

For an unnamed/unused parameter kept as a commented-out placeholder, the
operator attaches directly to the comment with no space, exactly as it
would to a real name: `const TVisObjRef &/*value*/`.

No spaces around `->`: `int a = foo->bar;`

## Vertical alignment and vertical space

- Extra spaces for column alignment are fine when they help readability
  (e.g. lining up a run of `=` signs), but don't put opening/closing
  brackets alone on their own line just for alignment.
- Avoid composite one-liners: `if (condition) doFoo();` should be
  ```cpp
  if (condition)
      doFoo();
  ```
- One variable declaration per line where practical.
- Blank lines between logical blocks within a function.

## Preprocessor

`#include`, `#define`, `#if`/`#ifdef`/`#endif`, `#pragma`, etc. always start
in column 1, even inside an indented block.

## Naming

- **Types** (class/struct/typedef/enum/template): `CamelCase`, starting
  uppercase. *Exception: recovered original type names, see above.*
- **Methods and free functions**: `camelCase`, starting lowercase.
  *Exception: recovered original method names, see above.*
- **Member variables**: `_camelCase` - a single leading underscore, then
  camelCase, starting lowercase after the underscore (e.g. `_currentText`).
  These are invented during reconstruction (the binary carries no member-
  name debug info), so always follow this convention with no exception.
- **Local variables and parameters**: `camelCase`, starting lowercase, no
  underscore.
- **Global variables**: prefix `g_`, then camelCase (e.g. `g_logfile`).
  *Exception: a global whose name is itself recovered from the binary's own
  symbol table keeps that recovered name (documented in `AppGlobals.h`).*
- **Constants and enum values**: `kCamelCase` (preferred in this project) or
  `ALL_CAPS_WITH_UNDERSCORES`. Prefer a real `enum class`/`const` over
  `#define`.
- **Doxygen comments**: JavaDoc style (`/** ... * @param x ... */`), using
  `@` (not `\`) for commands. A short trailing comment on a member uses
  `///<`.
- **Special comment keywords**: `FIXME`, `TODO`, `WORKAROUND` (explain what
  original-engine bug is being worked around), `I18N:`.

## Reformatting tooling

`astyle` (Artistic Style) approximates these conventions automatically. The
wiki's own suggested option string includes `convert-tabs`, which actually
**overrides tab indentation back to spaces** in this astyle version - omit
it. The working invocation used for this project's bulk reformatting pass:

```
astyle --indent=tab=4 --style=attach --pad-oper --pad-header \
    --align-pointer=name --align-reference=name --unpad-paren \
    --indent-preproc-block --indent-preproc-define --indent-preproc-cond \
    --suffix=none <files...>
```

Because astyle does not fix pointer/reference spacing inside function
*parameter lists* (only variable declarations and return types), a
follow-up pass is needed for those - see the regex approach used in this
project's formatting-conversion commit for a comment-aware implementation
(it must skip `//` and `/* */` comment text entirely, or it will corrupt
prose that happens to contain `*`/`&`, e.g. markdown-style `*emphasis*` or
a sentence mentioning a pointer type like "the manifest's `void*` was...").

## What NOT to reformat

- Recovered class/method identifiers (see the exception above).
- Recovered global variable names (see `AppGlobals.h`'s own notes on which
  names are recovered vs. invented).
- Third-party/stub API surfaces that intentionally mirror a real library's
  naming for future drop-in compatibility (`SdlStub.h`'s `SDL_*` names,
  `WxStub.h`'s `wx*` names) - these must match the real SDL2/wxWidgets API
  exactly, not ScummVM convention.
