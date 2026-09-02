// SDL2 backend for Platform.  Compiled only when WIZ_HAVE_SDL is defined
// (CMake found SDL2).
#ifdef WIZ_HAVE_SDL
#include "wiz/platform.h"

#include <SDL.h>
#include <vector>

namespace wiz {

namespace {
class SdlPlatform : public Platform {
public:
    SdlPlatform(const char *title, int scale) : scale_(scale < 1 ? 1 : scale) {
        SDL_Init(SDL_INIT_VIDEO);
        win_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                640 * scale_, 400 * scale_, SDL_WINDOW_RESIZABLE);
        ren_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_ACCELERATED);
        SDL_RenderSetLogicalSize(ren_, 640, 400);
        running_ = win_ && ren_;
    }
    ~SdlPlatform() override {
        if (tex_) SDL_DestroyTexture(tex_);
        if (ren_) SDL_DestroyRenderer(ren_);
        if (win_) SDL_DestroyWindow(win_);
        SDL_Quit();
    }

    void present(const Surface &s, const Color *pal, int palLen) override {
        if (!ren_) return;
        if (!tex_ || texW_ != s.width() || texH_ != s.height()) {
            if (tex_) SDL_DestroyTexture(tex_);
            tex_ = SDL_CreateTexture(ren_, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, s.width(), s.height());
            texW_ = s.width();
            texH_ = s.height();
        }
        rgba_.resize(size_t(s.width()) * s.height());
        const u8 *px = s.data();
        for (size_t i = 0; i < rgba_.size(); ++i) {
            Color c = px[i] < palLen ? pal[px[i]] : Color{255, 0, 255};
            rgba_[i] = 0xFF000000u | (c.r << 16) | (c.g << 8) | c.b;
        }
        SDL_UpdateTexture(tex_, nullptr, rgba_.data(), s.width() * 4);
        SDL_RenderClear(ren_);
        SDL_RenderCopy(ren_, tex_, nullptr, nullptr);
        SDL_RenderPresent(ren_);
    }

    int pollKey() override { return pump(false); }
    int waitKey() override {
        for (;;) {
            int k = pump(true);
            if (k != KEY_NONE) return k;
        }
    }
    void delayMs(int ms) override { SDL_Delay(u32(ms)); }
    bool running() const override { return running_; }

private:
    int pump(bool wait) {
        SDL_Event e;
        while (wait ? SDL_WaitEvent(&e) : SDL_PollEvent(&e)) {
            wait = false;
            if (e.type == SDL_QUIT) { running_ = false; return KEY_QUIT; }
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:   return KEY_LEFT;
                    case SDLK_RIGHT:  return KEY_RIGHT;
                    case SDLK_UP:     return KEY_UP;
                    case SDLK_DOWN:   return KEY_DOWN;
                    case SDLK_RETURN: case SDLK_KP_ENTER: return KEY_RETURN;
                    case SDLK_ESCAPE: return KEY_ESC;
                    case SDLK_BACKSPACE: return KEY_BACKSPACE;
                    default:
                        if (e.key.keysym.sym >= 32 && e.key.keysym.sym < 127) {
                            int c = e.key.keysym.sym;
                            if ((e.key.keysym.mod & KMOD_SHIFT) && c >= 'a' && c <= 'z')
                                c -= 32;
                            return c;
                        }
                }
            }
        }
        return KEY_NONE;
    }

    int scale_;
    SDL_Window *win_ = nullptr;
    SDL_Renderer *ren_ = nullptr;
    SDL_Texture *tex_ = nullptr;
    int texW_ = 0, texH_ = 0;
    std::vector<u32> rgba_;
    bool running_ = false;
};
} // namespace

std::unique_ptr<Platform> makeSdlPlatform(const char *title, int scale) {
    return std::make_unique<SdlPlatform>(title, scale);
}

} // namespace wiz
#endif // WIZ_HAVE_SDL
