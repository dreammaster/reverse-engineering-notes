// Win32 front-end: 320x200 window scaled 3x, KEH key codes.
#include "fod/game.h"
#include <windows.h>
#include <cstdio>
#include <deque>

using namespace fod;

namespace {

class Win32Platform : public Platform {
public:
	Win32Platform() {
		WNDCLASSA wc{};
		wc.lpfnWndProc = &Win32Platform::wndProc;
		wc.hInstance = GetModuleHandleA(nullptr);
		wc.lpszClassName = "FodWindow";
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		RegisterClassA(&wc);
		RECT r{0, 0, 320 * kScale, 200 * kScale};
		AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);
		_hwnd = CreateWindowA("FodWindow", "Fountain of Dreams", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
		                      CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, nullptr, nullptr, wc.hInstance, this);
		ShowWindow(_hwnd, SW_SHOW);
		_bits.resize(320 * 200);
	}

	int waitKey() override {
		for (;;) {
			if (!_keys.empty()) {
				const int k = _keys.front();
				_keys.pop_front();
				return k;
			}
			if (_closed) return -1;
			MSG msg;
			if (GetMessageA(&msg, nullptr, 0, 0) <= 0) { _closed = true; return -1; }
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}

	void present(const Screen &s) override {
		const Rgb *pal = masterPalette();
		const Bytes &px = s.pixels();
		for (size_t i = 0; i < px.size(); ++i) {
			const Rgb c = pal[px[i] & 15];
			_bits[i] = (uint32_t(c.r) << 16) | (uint32_t(c.g) << 8) | c.b;
		}
		InvalidateRect(_hwnd, nullptr, FALSE);
		UpdateWindow(_hwnd);
	}

private:
	static constexpr int kScale = 3;

	static LRESULT CALLBACK wndProc(HWND h, UINT m, WPARAM wp, LPARAM lp) {
		Win32Platform *self = reinterpret_cast<Win32Platform *>(GetWindowLongPtrA(h, GWLP_USERDATA));
		if (m == WM_NCCREATE) {
			auto *cs = reinterpret_cast<CREATESTRUCTA *>(lp);
			SetWindowLongPtrA(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
			return DefWindowProcA(h, m, wp, lp);
		}
		if (!self) return DefWindowProcA(h, m, wp, lp);
		switch (m) {
		case WM_CLOSE: self->_closed = true; DestroyWindow(h); return 0;
		case WM_DESTROY: self->_closed = true; PostQuitMessage(0); return 0;
		case WM_KEYDOWN: {
			int k = 0;
			switch (wp) {
			case VK_UP: k = 0xFD; break;
			case VK_DOWN: k = 0xFC; break;
			case VK_LEFT: k = 0xFB; break;
			case VK_RIGHT: k = 0xFA; break;
			case VK_ESCAPE: k = 0xFF; break;
			case VK_RETURN: case VK_SPACE: k = 0xFE; break;
			case VK_F1: case VK_F2: case VK_F3: case VK_F4: case VK_F5: k = 0x3B + int(wp - VK_F1); break;
			default: break;
			}
			if (k) self->_keys.push_back(k);
			return 0;
		}
		case WM_CHAR: {
			const int c = int(wp);
			if (c >= 'a' && c <= 'z') self->_keys.push_back(c - 32);
			else if (c >= '0' && c <= '9') self->_keys.push_back(c);
			else if (c >= 'A' && c <= 'Z') self->_keys.push_back(c);
			else if (c == 8) self->_keys.push_back(8);
			return 0;
		}
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
			StretchDIBits(dc, 0, 0, 320 * kScale, 200 * kScale, 0, 0, 320, 200, self->_bits.data(), &bi, DIB_RGB_COLORS, SRCCOPY);
			EndPaint(h, &ps);
			return 0;
		}
		}
		return DefWindowProcA(h, m, wp, lp);
	}

	HWND _hwnd = nullptr;
	bool _closed = false;
	std::deque<int> _keys;
	std::vector<uint32_t> _bits;
};

} // namespace

int main(int argc, char **argv) {
	const std::string dir = argc >= 2 ? argv[1] : "fod/game";
	try {
		GameData data(dir);
		Assets assets(dir);
		Win32Platform platform;
		Game game(data, assets, platform, Game::defaultParty(data));
		game.run();
	} catch (const std::exception &e) {
		MessageBoxA(nullptr, e.what(), "Fountain of Dreams", MB_OK | MB_ICONERROR);
		return 1;
	}
	return 0;
}
