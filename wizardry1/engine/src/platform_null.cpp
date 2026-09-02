#include "wiz/platform.h"

#include <cstdio>

namespace wiz {

namespace {
class NullPlatform : public Platform {
public:
    explicit NullPlatform(std::string dir) : dir_(std::move(dir)) {}

    void present(const Surface &s, const Color *pal, int palLen) override {
        if (dir_.empty()) return;
        char path[512];
        std::snprintf(path, sizeof path, "%s/frame%04d.ppm", dir_.c_str(), frame_++);
        s.savePPM(path, pal, palLen);
        std::printf("[null] wrote %s\n", path);
    }
    int pollKey() override { return KEY_NONE; }
    int waitKey() override { return KEY_QUIT; }        // nothing to wait on
    void delayMs(int) override {}
    bool running() const override { return frame_ < 1; }   // one frame then stop

private:
    std::string dir_;
    int frame_ = 0;
};
} // namespace

std::unique_ptr<Platform> makeNullPlatform(const std::string &dumpDir) {
    return std::make_unique<NullPlatform>(dumpDir);
}

#ifndef WIZ_HAVE_SDL
std::unique_ptr<Platform> makeSdlPlatform(const char *, int) { return nullptr; }
#endif

} // namespace wiz
