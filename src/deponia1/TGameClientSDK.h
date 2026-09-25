// Not yet assert-confirmed to a specific file; stays at the top level.
// Confirmed from TMasterControl's constructor: a small polymorphic wrapper
// (own vtable has only destructors) holding pointers to platform-specific
// backends.
#pragma once

class TSteamSDK {
public:
    TSteamSDK() = default;
};

class TGalaxySDK {
public:
    TGalaxySDK() = default;
};

class TGameClientSDK {
public:
    TGameClientSDK();
    virtual ~TGameClientSDK();

private:
    TSteamSDK* m_steam;
    TGalaxySDK* m_galaxy;
};
