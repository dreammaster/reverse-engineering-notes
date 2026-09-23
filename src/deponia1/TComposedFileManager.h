#pragma once

// Not yet reverse-engineered; only the one static method TStandardPaths
// calls is stubbed here (see manifest/proprietary_classes.tsv for its full
// method list when this class gets its own pass).
class TComposedFileManager {
public:
    static bool IsGameCompiled();
};
