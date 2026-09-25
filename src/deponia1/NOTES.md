# main() reconstruction notes

## TGameControl batch: reference-return accessors, and a custom character hash table

While reconstructing TGameControl's shortest methods (address-gap analysis,
asm lines ~455750-462554), two patterns worth flagging for future passes:

- **Several "getter" methods return a reference/pointer to a member, not a
  copy**, even though `manifest/proprietary_functions.tsv` had them
  classified with by-value return types (`std::vector<TGCharacter*>`,
  `wxString`). The tell: the function body is a bare `lea rax,[rdi+OFFSET];
  retn` with no second hidden-pointer parameter - a real by-value return of
  a non-trivial type would need the Itanium ABI's RVO calling convention
  (caller-supplied destination pointer in rdi, `this` moved to rsi).
  `GetAllCharacters()` and `GetGamePath()` were both fixed from by-value to
  by-reference for this reason; worth checking any other accessor whose
  manifest signature returns a non-trivial type before assuming it's a copy.
- **TGameControl has a hand-rolled hash table mapping character IDs to
  `m_characters` indices** (`GetCharacter`/`GetCharacterPointer`/
  `GetCharacterPointerEx`, asm lines 456360-456578): `TVisObjRef::GetId()`
  returns a 3-byte identifier that gets packed into a 32-bit hash (byte 0 |
  byte1<<8 | sign-extended byte2<<16-24), divided by a bucket count at
  `+0x378`, and walked as a singly-linked chain (`node+0`: stored hash for
  comparison, `node+4`: a 32-bit index into `m_characters` at `+0x340`,
  `node+8`: next). Left as stubs (falling back to `m_currentCharacter` on an
  empty ref, `nullptr` otherwise) rather than guessing at the node struct
  layout or how entries get inserted - a real implementation would probably
  just use `std::unordered_map<int, TGCharacter*>` for equivalent behavior
  without reproducing the exact bucket mechanics, per the project's
  behavioral-fidelity-over-binary-fidelity stance (see the COW-string-ABI
  note below).
- **Controller buttons map into a custom keysym space starting at
  1000001**: `ConvertControllerButtonToSymKey` (asm lines 457042-457058)
  looks up a 15-entry table (`CSWTCH_876`) that's just `1000001 + button`
  for buttons 0-14, -1 otherwise - presumably reserved above the Unicode
  range used for regular keyboard key codes elsewhere in the engine.

## Lesson: don't fill an unconfirmed gap with a plausible-looking guess

While TMasterControl was being written, no evidence was found for where
its `m_sceneControl` pointer gets set (its own constructor, as actually
traced, never touches it) - so it got a `new TSceneControl()` in
TMasterControl's constructor "to make GetMainControl() work," clearly
marked as a guess at the time. Once TGameControl was reversed, its
constructor turned out to embed a *real* `TSceneControl` by value and
point `m_sceneControl` at that instead - so `~TMasterControl()`'s `delete
m_sceneControl` ended up calling `delete` on a non-heap address, silently
corrupting the heap (caught immediately by testing: MinGW's CRT reported
`STATUS_HEAP_CORRUPTION`, no output at all since the crash happened before
stdout - fully buffered when not attached to a console - ever flushed).

Fixed by removing the guess entirely: `m_sceneControl` is left null in
TMasterControl's own constructor, and only the owning subclass (currently
just TGameControl) is responsible for pointing it at whatever TSceneControl
it actually owns. The general lesson: a "fill the gap with something
plausible so it compiles" placeholder is fine short-term, but needs
revisiting - and re-testing - the moment a later class's real evidence
contradicts it, rather than assuming the guess was harmless because it
compiled and ran once.

Source: `Deponia_Linux.asm`, `main` proc, asm lines 499508-500213
(address range 0x62E450-0x62EC85 roughly; see `endp` before local labels
resume at 0x62ED2E).

## Capstone finding: this binary uses the old COW std::string ABI

Confirmed while reversing `TComposedFile`'s entry-growth loop (which copies
an `SEntryInfo`'s `std::wstring` field via a **single 8-byte pointer copy**,
then five more separate qwords): this binary was compiled with
`-D_GLIBCXX_USE_CXX11_ABI=0`, i.e. libstdc++'s pre-C++11 **copy-on-write**
string representation, where a `std::string`/`std::wstring` object is
*literally just one pointer* to a shared, ref-counted buffer (a hidden
`_Rep` header - refcount, length, capacity - living immediately before the
visible character data). Modern libstdc++ (the SSO/"short string
optimization" ABI most of us are used to) makes a `std::wstring` a 32-byte
object with an inline small-buffer; this binary's strings are 8 bytes.

This single fact retroactively explains several things noted independently,
class by class, earlier in this project:

- Why `wxString`/`wxFileName`-typed values kept turning out to be
  reinterpretable as a bare `std::wstring` pointer with no visible
  wrapper overhead (main.cpp's `main+0x37D`, `TComposedFile`'s entries,
  `TMasterControl`'s several string members) - they're COW strings, so a
  "wxString" that wraps one has the exact same 8-byte layout as the
  wstring itself.
- Why so many *different* classes' destructors collapsed under the same
  ICF-folded symbol (`std::pair<std::string const,ulong>::~pair`,
  `std::list<ctdrpc::earlyrole_ip_t>::_M_clear`, etc., see below): a COW
  string's destructor is just "decrement the shared refcount, free the
  `_Rep` if it hit zero" - completely generic machine code that doesn't
  depend on the character type or the wrapping class at all, so the linker
  folds huge numbers of unrelated destructors together.

Not relevant to *our* rebuild: we use ordinary modern `std::wstring` inside
`wxString` (see `WxStub.h`) throughout, since this project targets
behavioral fidelity on a fresh compiler/ABI, not binary compatibility with
the original executable. Recorded here purely because it resolves a
question ("why does this keep looking like just a pointer?") raised
independently in at least three earlier classes' notes.

## Feasibility verdict

`main` itself is tractable: it's a straightforward sequence of calls with a
handful of early-return branches and the usual GCC `-O2` exception-handling
landing pads (each cleanup block is clearly commented by IDA as
`; cleanup() // owned by <addr>`, which made matching them back to their
try-region trivial). No STL containers are iterated, no templates are
instantiated inline other than `std::wstring`/`std::function`, and the two
`std::function` thunks (`_M_invoke`, `_M_manager`) were easy to read directly
because they're tiny (the lambda body is a single `jmp` to
`TPictureIO::RetryFailedPicturesLoad`).

## Recovering the original directory layout from x_assert()

The engine's assert macro, `x_assert(bool cond, const char* expr, const
char* file, int line)`, embeds `__FILE__` as a full build-machine path at
every call site, e.g. `/home/simon/Documents/jenkins/branchPillars/src/
vsplayer/control/gameController.cpp`. `tools/extract_source_layout.py` scans
every one of the 378 `x_assert` call sites in the binary, resolves the
file/expression string operands, and cross-references the enclosing
function - giving a confirmed mapping from many classes to their real
source file, without guessing. Results: `manifest/source_layout.tsv` (raw,
one row per assert) and `manifest/source_layout_summary.tsv` (grouped by
recovered file, with sample functions).

This is now the basis for this project's directory layout: `src/deponia1/`
mirrors the original `src/` root (confirmed via asserts:
`baselib/composedfile.cpp` for `TComposedFile`, `graphicslib/picture.cpp`
for `TPictureIO`, `vsplayer/control/gameController.cpp` for
`TGameController`, `vsplayer/main/mainSDL.cpp` for `main`/`Init`). A class
only gets moved into that structure once an assert (or other hard evidence)
actually confirms its file; `TStandardPaths`, `TMasterControl`, and
`TComposedFileManager` have no assert hits yet, so they stay at the top
level of `src/deponia1/` rather than being placed on a guess. Each moved
header/source has a comment recording which assert confirmed its location.

Re-run the extractor (cheap, a few seconds) whenever a new class's methods
get reversed, in case they resolve to a file not yet seen - some files in
`source_layout_summary.tsv` only have one or two sampled functions and are
likely to grow other members as more classes get worked through.

## Key finding: ICF (identical code folding) is corrupting symbol names

Several call sites in `main` resolve to
`std::pair<std::string const,unsigned long>::~pair()` (with IDA-appended
`_0`/`_1` suffixes to disambiguate), even at addresses that are structurally
required to be destroying a `wxFileName` or a `wxString`. That demangled name
almost certainly does not describe the real type; it is the name the linker
happened to keep for one member of a set of functions whose *machine code*
is byte-identical and got folded into a single symbol (a `pair<string,
unsigned long>` destructor that just tears down two trivial members compiles
to the same instructions as many other small classes' destructors). This is
consistent with what you ran into before with the String class: any type
whose special members are simple enough to collide with another type's under
ICF will get an arbitrary, misleading label, and Hexrays' type propagation
compounds the error once it borrows that symbol's declared type. Cross
referencing *how* a value is used (what it's copy-constructed into, what
it's passed to) proved more reliable than trusting the demangled destructor
name.

Concretely, this is what let me determine `g_logfile` is a `wxFileName`, not
a `wxString`: the call believed to be `wxString::operator=` at main+0x213
assigns into `g_logfile`, but `g_logfile` is later passed as the implicit
`this` to `wxFileName::GetFullPath()` (main+0x22C) - only consistent if
`g_logfile` is a `wxFileName` and the "wxString::operator=" symbol is really
`wxFileName::operator=` folded together with it (a wxFileName whose copy
assignment just copies its one wxString field would compile identically to
wxString's own copy assignment).

Separately, at main+0x2AE (`std::wstring::basic_string(std::wstring
const&)` constructing a local straight from a `wxString*`), the call is only
valid if `wxString`'s layout starts with (or *is*) a `std::wstring` - so
`WxStub.h`'s `wxString` wraps a `std::wstring` as its data, which is
consistent with both observations.

## Things intentionally simplified for the stub build

- **Control flow**: the original has ~15 duplicated
  `wxCmdLineParser::~wxCmdLineParser()` calls, one per exception/early-return
  path, because GCC inlined the destructor at every exit instead of sharing
  an epilogue. Ordinary C++ scoping (`cmdLineParser` going out of scope) is
  semantically equivalent and far more readable, so `main.cpp` uses plain
  `if (...) return -1;` instead of chasing every `jmp`/landing pad 1:1.
- **`fopen64` -> `fopen`**: `fopen64` is glibc's LFS entry point for the
  32-bit-`off_t` ABI; on a from-scratch MinGW build there's no reason to
  keep it, `fopen` is the right call.
- **SDL/wxWidgets are stand-ins** (`SdlStub.h`, `WxStub.h`), not the real
  libraries - neither is installed in this environment yet. Function names
  and signatures were kept identical to the real APIs so swapping in real
  SDL2 headers and a real wxString/wxFileName implementation later should be
  close to a drop-in replacement for the call sites in `main.cpp`.
- **Stub bodies** (`AppFunctions.cpp`, `T*.cpp`) do just enough to let the
  smoke test run to completion: `Init` and `ParseCommandLine` report success,
  `ShowFrame` clears `isProgramLooping` after one call so the "game loop"
  actually exits, `g_pGameControl` is heap-allocated up front since the real
  binary must set it up somewhere before `main` runs and that code hasn't
  been located yet.

## Recovered string literals (verified byte-for-byte against the .asm data)

| Symbol | Value |
|---|---|
| `unk_D6EAD8` | `L"/messages.log"` |
| `dword_D6EB70` | `L"Init failed, could not load game"` |
| `dword_D6EB10` | `L"Unable to open SDL: %s"` |
| `unk_D6EBF8` | `L"gamecontrollerdb.txt"` |
| `dword_D6EC50` | `L"Added Controller mappings from File %s"` |
| `asc_D6F40C` | `L"l"` (the `-l` / `--l` command-line option name) |
| `aInitFailed` | `"Init failed"` |
| `aInitFailedCoul` | `"Init failed, could not load game.\nThere is no game file or the game file is corrupted.\nFor more info view messages.log."` |
| `aSdl` | `"SDL "` |
| `aCouldnTInitSdl` | `"Couldn't init SDL. App will quit now."` |

`ShowMessageBox`'s parameter order was confirmed from both call sites:
`rdi` (first arg) is always the short string ("Init failed" / "SDL "), `rsi`
(second arg) the long descriptive one, so the signature is
`ShowMessageBox(const wxString& title, const wxString& message)`.

The `fopen64` mode string is `(offset aW+1)`, i.e. it points one byte into
the existing `"-w"` literal to reuse its trailing `"w"` - a linker
string-suffix-merging trick, not a real "-w" flag; the actual open mode is
just `"w"`.

`SDL_Init` is called with flags `0x21` = `SDL_INIT_VIDEO (0x20) |
SDL_INIT_TIMER (0x01)`. `SDL_EventState` is called with type `0x303`, which
is `SDL_TEXTINPUT` in real SDL2.

## Not yet resolved

- Exact bit-width/type of `AppStatus`, `isProgramLooping`, `eMouseMessage`,
  `byte_11F8B01`/`byte_11F8B02` (all just `mov [addr], imm8` stores in this
  function - their real type would need to be inferred from other
  reader/writer sites).
- Where `g_pGameControl` and `standardPaths` are really initialized (not in
  `main`).
- `wxString`/`wxFileName`'s true field layout - deferred until we're
  implementing them for real rather than stubbing them.
