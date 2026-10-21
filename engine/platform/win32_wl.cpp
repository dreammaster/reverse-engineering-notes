// Win32 front-end for the recompiled Wasteland 1 (wlplay <WL.EXE or image.bin> <gamedir>): 320x200 EGA window scaled 3x.
#include "wl/recomp_rt.h"
#include "fod/bytes.h"
#include <windows.h>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

using namespace wl;

namespace {

constexpr int kScale = 3;
const uint32_t kPalette[16] = {0x000000, 0x0000aa, 0x00aa00, 0x00aaaa, 0xaa0000, 0xaa00aa, 0xaa5500, 0xaaaaaa,
                               0x555555, 0x5555ff, 0x55ff55, 0x55ffff, 0xff5555, 0xff55ff, 0xffff55, 0xffffff};

struct Window {
	Machine *machine = nullptr;
	HWND hwnd = nullptr;
	std::vector<uint32_t> bits = std::vector<uint32_t>(320 * 200);
	bool closed = false;

	void create() {
		WNDCLASSA wc{};
		wc.lpfnWndProc = &Window::wndProc;
		wc.hInstance = GetModuleHandleA(nullptr);
		wc.lpszClassName = "WastelandWindow";
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		RegisterClassA(&wc);
		const DWORD style = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
		RECT r{0, 0, 320 * kScale, 200 * kScale};
		AdjustWindowRect(&r, style, FALSE);
		hwnd = CreateWindowA("WastelandWindow", "Wasteland", style, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, nullptr, nullptr, wc.hInstance, this);
		ShowWindow(hwnd, SW_SHOW);
	}

	void pump() {
		MSG msg;
		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) closed = true;
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
		if (closed) throw rc::ExitProgram{0};
	}

	void paint() {
		for (int y = 0; y < 200; ++y)
			for (int x = 0; x < 320; ++x) bits[size_t(y) * 320 + size_t(x)] = kPalette[machine->ega.pixel(x, y)];
		InvalidateRect(hwnd, nullptr, FALSE);
		UpdateWindow(hwnd);
	}

	static LRESULT CALLBACK wndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
		Window *self = reinterpret_cast<Window *>(GetWindowLongPtrA(h, GWLP_USERDATA));
		if (msg == WM_NCCREATE) {
			SetWindowLongPtrA(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTA *>(lp)->lpCreateParams));
			return DefWindowProcA(h, msg, wp, lp);
		}
		if (!self) return DefWindowProcA(h, msg, wp, lp);
		const uint16_t scan = uint16_t(((lp >> 16) & 0xFF) << 8);
		switch (msg) {
		case WM_CLOSE: self->closed = true; DestroyWindow(h); return 0;
		case WM_DESTROY: self->closed = true; PostQuitMessage(0); return 0;
		case WM_KEYDOWN: {
			// keys without a character: arrows, function keys, cursor block (BIOS: scan code << 8, ASCII 0)
			const bool extended = (wp >= VK_F1 && wp <= VK_F12) || (wp >= VK_PRIOR && wp <= VK_DOWN) || wp == VK_INSERT || wp == VK_DELETE;
			if (extended) self->machine->keys.push_back(scan);
			return 0;
		}
		case WM_CHAR:
			if (wp < 128) self->machine->keys.push_back(uint16_t(scan | uint16_t(wp)));
			return 0;
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(h, &ps);
			BITMAPINFO bi{};
			bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bi.bmiHeader.biWidth = 320;
			bi.bmiHeader.biHeight = -200;
			bi.bmiHeader.biPlanes = 1;
			bi.bmiHeader.biBitCount = 32;
			bi.bmiHeader.biCompression = BI_RGB;
			StretchDIBits(dc, 0, 0, 320 * kScale, 200 * kScale, 0, 0, 320, 200, self->bits.data(), &bi, DIB_RGB_COLORS, SRCCOPY);
			EndPaint(h, &ps);
			return 0;
		}
		}
		return DefWindowProcA(h, msg, wp, lp);
	}
};

} // namespace

int main(int argc, char **argv) {
	const std::string image = argc >= 2 ? argv[1] : "WL.EXE", dir = argc >= 3 ? argv[2] : "wasteland1/game";
	Machine m;
	Window win;
	win.machine = &m;
	try {
		rc::Regs r;
		rc::boot(m, fod::readFile(image), dir, r);
		m.sys->realtime = true;
		win.create();
		auto last = std::chrono::steady_clock::now();
		auto refresh = [&]() {
			win.pump();
			const auto now = std::chrono::steady_clock::now();
			if (now - last >= std::chrono::milliseconds(16)) { last = now; win.paint(); }
		};
		m.sys->onTick = refresh;
		m.onIdle = [&]() { refresh(); };
		rc::run(m, r);
	} catch (const rc::ExitProgram &) {
		return 0;
	} catch (const std::exception &e) {
		MessageBoxA(nullptr, e.what(), "Wasteland", MB_OK | MB_ICONERROR);
		return 1;
	}
	return 0;
}
