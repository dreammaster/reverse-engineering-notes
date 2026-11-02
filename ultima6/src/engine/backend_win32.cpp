#ifdef _WIN32

#include "engine/backend.h"

#include <windows.h>

#include <deque>

namespace Ultima6 {

namespace {

class Win32Backend : public Backend {
public:
	Win32Backend(const char *title, int scale) : _hwnd(nullptr), _scale(scale), _quit(false) {
		WNDCLASSA wc = {};
		wc.lpfnWndProc = &Win32Backend::wndProc;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.lpszClassName = "Ultima6Engine";
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		RegisterClassA(&wc);
		RECT r = {0, 0, Framebuffer::WIDTH * scale, Framebuffer::HEIGHT * scale};
		AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE);
		_hwnd = CreateWindowA("Ultima6Engine", title, (WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX)) | WS_VISIBLE,
							  CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, nullptr, nullptr, wc.hInstance, this);
		SetWindowLongPtrA(_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	}

	void present(const Framebuffer &screen) override {
		_frame = screen;
		_rgb.resize(Framebuffer::WIDTH * Framebuffer::HEIGHT);
		const uint8_t *p = screen.pixels();
		for (int i = 0; i < Framebuffer::WIDTH * Framebuffer::HEIGHT; ++i) {
			int r, g, b;
			screen.colour(p[i], r, g, b);
			_rgb[i] = (r << 16) | (g << 8) | b;
		}
		InvalidateRect(_hwnd, nullptr, FALSE);
		pump();
	}

	int pollKey() override {
		pump();
		if (_quit)
			return KEY_QUIT;
		if (_keys.empty())
			return KEY_NONE;
		int k = _keys.front();
		_keys.pop_front();
		return k;
	}

	int waitKey() override {
		for (;;) {
			int k = pollKey();
			if (k != KEY_NONE)
				return k;
			WaitMessage();
		}
	}

	unsigned ticks() override {
		return GetTickCount();
	}

	void delay(unsigned ms) override {
		Sleep(ms);
		pump();
	}

private:
	void pump() {
		MSG msg;
		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				_quit = true;
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}

	void paint(HDC dc) {
		if (_rgb.empty())
			return;
		BITMAPINFO bmi = {};
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = Framebuffer::WIDTH;
		bmi.bmiHeader.biHeight = -Framebuffer::HEIGHT;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		StretchDIBits(dc, 0, 0, Framebuffer::WIDTH * _scale, Framebuffer::HEIGHT * _scale, 0, 0, Framebuffer::WIDTH, Framebuffer::HEIGHT,
					  &_rgb[0], &bmi, DIB_RGB_COLORS, SRCCOPY);
	}

	static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
		Win32Backend *self = (Win32Backend *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
		if (self) {
			switch (msg) {
			case WM_CLOSE:
				self->_quit = true;
				return 0;
			case WM_PAINT: {
				PAINTSTRUCT ps;
				HDC dc = BeginPaint(hwnd, &ps);
				self->paint(dc);
				EndPaint(hwnd, &ps);
				return 0;
			}
			case WM_KEYDOWN:
				switch (wp) {
				case VK_UP: self->_keys.push_back(KEY_UP); break;
				case VK_DOWN: self->_keys.push_back(KEY_DOWN); break;
				case VK_LEFT: self->_keys.push_back(KEY_LEFT); break;
				case VK_RIGHT: self->_keys.push_back(KEY_RIGHT); break;
				case VK_HOME: self->_keys.push_back(KEY_HOME); break;
				case VK_END: self->_keys.push_back(KEY_END); break;
				case VK_PRIOR: self->_keys.push_back(KEY_PGUP); break;
				case VK_NEXT: self->_keys.push_back(KEY_PGDN); break;
				case VK_ESCAPE: self->_keys.push_back(KEY_ESC); break;
				default: break;
				}
				return 0;
			case WM_CHAR:
				if (wp == 13 || wp == 8 || wp == 32 || (wp >= 33 && wp < 127))
					self->_keys.push_back((int)wp);
				return 0;
			default:
				break;
			}
		}
		return DefWindowProcA(hwnd, msg, wp, lp);
	}

	HWND _hwnd;
	int _scale;
	bool _quit;
	std::deque<int> _keys;
	std::vector<uint32_t> _rgb;
	Framebuffer _frame;
};

} // namespace

Backend *createWindowBackend(const char *title, int scale) {
	return new Win32Backend(title, scale);
}

} // namespace Ultima6

#else

#include "engine/backend.h"

namespace Ultima6 {
Backend *createWindowBackend(const char *, int) {
	return nullptr;
}
} // namespace Ultima6

#endif
