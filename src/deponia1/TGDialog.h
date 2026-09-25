// Not yet assert-confirmed to a specific file; stays at the top level.
// Embedded by value in TGameControl (confirmed: TGameControl's ctor calls
// TGDialog::TGDialog() directly on an interior pointer, and TGameControl
// exposes it via GetDialog()/StartDialog()/EndDialog()).
#pragma once

class TGDialog {
public:
    TGDialog() = default;
};
