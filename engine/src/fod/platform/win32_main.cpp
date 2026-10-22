// Win32 front-end: 320x200 window scaled 3x, KEH key codes.
#include "fod/game.h"
#include "fod/creator.h"
#include "fod/savegame.h"
#include <windows.h>
#include <mmsystem.h>
#include <cstring>
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

	int waitKeyTimeout(int ms) override {
		const DWORD end = GetTickCount() + DWORD(ms);
		for (;;) {
			if (!_keys.empty()) { const int k = _keys.front(); _keys.pop_front(); return k; }
			if (_closed) return -1;
			const DWORD now = GetTickCount();
			if (int(end - now) <= 0) return -2;
			MsgWaitForMultipleObjects(0, nullptr, FALSE, end - now, QS_ALLINPUT);
			MSG msg;
			while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) { _closed = true; break; }
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
			}
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

	void playPcm(const std::vector<int16_t> &samples, int rate) override {
		if (samples.empty()) return;
		std::vector<uint8_t> wav(44 + samples.size() * 2);
		auto put32 = [&](size_t o, uint32_t v) { for (int i = 0; i < 4; ++i) wav[o + size_t(i)] = uint8_t(v >> (8 * i)); };
		auto put16 = [&](size_t o, uint16_t v) { wav[o] = uint8_t(v); wav[o + 1] = uint8_t(v >> 8); };
		std::memcpy(&wav[0], "RIFF", 4); put32(4, uint32_t(wav.size() - 8)); std::memcpy(&wav[8], "WAVEfmt ", 8);
		put32(16, 16); put16(20, 1); put16(22, 1); put32(24, uint32_t(rate)); put32(28, uint32_t(rate) * 2); put16(32, 2); put16(34, 16);
		std::memcpy(&wav[36], "data", 4); put32(40, uint32_t(samples.size() * 2));
		std::memcpy(&wav[44], samples.data(), samples.size() * 2);
		_wav.swap(wav);   // keep the buffer alive while it plays
		PlaySoundA(reinterpret_cast<LPCSTR>(_wav.data()), nullptr, SND_MEMORY | SND_ASYNC);
	}

	std::vector<uint8_t> _wav;
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
		const std::string saveDir = argc >= 3 && std::string(argv[2]).rfind("--", 0) != 0 ? argv[2] : "saves";
		CreateDirectoryA(saveDir.c_str(), nullptr);
		bool forceNew = false, useDefault = false;
		for (int i = 1; i < argc; ++i) {
			if (std::string(argv[i]) == "--new") forceNew = true;
			if (std::string(argv[i]) == "--default-party") useDefault = true;
		}
		Disk1 disk;
		std::vector<std::pair<int, Bytes>> maps;
		const int slot = forceNew ? 0 : newestSave(saveDir);
		if (slot > 0 && loadGame(saveDir, slot, disk, maps)) {
			// continue the saved game
		} else if (useDefault) {
			disk = Game::defaultParty(data);
		} else {
			disk = emptyRoster();
			if (!runPartyCreator(data, assets, platform, disk)) return 0;
		}
		Game game(data, assets, platform, disk, maps, saveDir);
		game.run();
	} catch (const std::exception &e) {
		MessageBoxA(nullptr, e.what(), "Fountain of Dreams", MB_OK | MB_ICONERROR);
		return 1;
	}
	return 0;
}
