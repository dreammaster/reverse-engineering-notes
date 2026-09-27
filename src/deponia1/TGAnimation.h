// Not yet assert-confirmed to a specific file; stays at the top level.
// The character/scene animation subsystem. Only the entry points
// TGameControl calls are declared here (confirmed call shapes, not
// reversed bodies): ClearAnimations (~TGameControl, Deponia_Linux.asm line
// 474247) and SaveAnimations (TGameControl::Save, asm line 462896). Both
// are called without an object of this type ever being constructed at
// their call sites, so modeled as static methods (same pattern as
// TGAction's entry points).
#pragma once

class TGAnimation {
public:
    static void ClearAnimations();
    static void SaveAnimations();
};
