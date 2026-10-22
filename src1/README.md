# Book I, Chapter 1 reimplementation — not started

C++ reimplementation of Chapter 1, intended to be ported to ScummVM once
complete and validated. Chapter 1 uses a different engine from Chapters 2/3
(see [`docs1/README.md`](../docs1/README.md)), so it lives here rather than in
[`src23/`](../src23) (which is C).

Each original executable is its own namespace under `Yendor1` (`Yendor1::Sw`,
`Yendor1::Prog0` … `Yendor1::Prog18`), alongside the shared `Yendor1::Vid`,
`Yendor1::Data` and `Yendor1::Shared` layers. Programs never depend on each
other's namespaces. See [`docs1/strategy.md`](../docs1/strategy.md).
