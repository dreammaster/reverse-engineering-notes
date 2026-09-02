#include "wiz/platform.h"

#include <cstdio>

namespace wiz {

namespace {
class NullPlatform : public Platform {
public:
    NullPlatform(std::string keys, std::string dir)
        : keys_(std::move(keys)), dir_(std::move(dir)) {}

    void present(const Surface &s, const Color *pal, int palLen) override {
        if (dir_.empty()) return;
        char path[512];
        std::snprintf(path, sizeof path, "%s/frame%04d.ppm", dir_.c_str(), frame_++);
        s.savePPM(path, pal, palLen);
    }
    int pollKey() override { return nextKey(); }
    int waitKey() override { return nextKey(); }
    void delayMs(int) override {}
    bool running() const override { return pos_ < keys_.size(); }

private:
    int nextKey() {
        if (pos_ >= keys_.size()) return KEY_QUIT;
        return (unsigned char)keys_[pos_++];
    }
    std::string keys_, dir_;
    size_t pos_ = 0;
    int frame_ = 0;
};
} // namespace

std::unique_ptr<Platform> makeNullPlatform(const std::string &keyScript,
                                           const std::string &dumpDir) {
    return std::make_unique<NullPlatform>(keyScript, dumpDir);
}

#ifndef WIZ_HAVE_SDL
std::unique_ptr<Platform> makeSdlPlatform(const char *, int) { return nullptr; }
#endif

} // namespace wiz
