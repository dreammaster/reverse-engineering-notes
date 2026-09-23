# main() reconstruction notes

Source: `Deponia_Linux.asm`, `main` proc, asm lines 499508-500213
(address range 0x62E450-0x62EC85 roughly; see `endp` before local labels
resume at 0x62ED2E).

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
