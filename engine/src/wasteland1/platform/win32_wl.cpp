#include <algorithm>
// Win32 front-end for the recompiled Wasteland 1 (wlplay <WL.EXE or image.bin> <gamedir>): 320x200 EGA window scaled 3x.
#include "wasteland1/recomp_rt.h"
#include "shared/bytes.h"
#include <windows.h>
#include <mmsystem.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

using namespace wl;

namespace {

constexpr int kScale = 3;
const uint32_t kPalette[16] = {0x000000, 0x0000aa, 0x00aa00, 0x00aaaa, 0xaa0000, 0xaa00aa, 0xaa5500, 0xaaaaaa,
                               0x555555, 0x5555ff, 0x55ff55, 0x55ffff, 0xff5555, 0xff55ff, 0xffff55, 0xffffff};

// PC speaker: a square wave generated on a background thread (waveOut, 22050 Hz, 8 bit mono)
class Speaker {
public:
	Speaker() {
		WAVEFORMATEX fmt{};
		fmt.wFormatTag = WAVE_FORMAT_PCM;
		fmt.nChannels = 1;
		fmt.nSamplesPerSec = kRate;
		fmt.wBitsPerSample = 8;
		fmt.nBlockAlign = 1;
		fmt.nAvgBytesPerSec = kRate;
		if (waveOutOpen(&_out, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) return;
		_thread = std::thread([this] { run(); });
	}
	~Speaker() {
		_quit = true;
		if (_thread.joinable()) _thread.join();
		if (_out) { waveOutReset(_out); for (auto &b : _bufs) waveOutUnprepareHeader(_out, &b.hdr, sizeof b.hdr); waveOutClose(_out); }
	}
	void set(int hz) { _hz = hz; }

private:
	static constexpr int kRate = 22050, kChunk = 512;
	struct Buf { WAVEHDR hdr{}; std::vector<uint8_t> data = std::vector<uint8_t>(kChunk); };

	void run() {
		for (auto &b : _bufs) {
			b.hdr.lpData = reinterpret_cast<LPSTR>(b.data.data());
			b.hdr.dwBufferLength = kChunk;
			waveOutPrepareHeader(_out, &b.hdr, sizeof b.hdr);
			b.hdr.dwFlags |= WHDR_DONE;
		}
		double phase = 0;
		while (!_quit) {
			bool any = false;
			for (auto &b : _bufs) {
				if (!(b.hdr.dwFlags & WHDR_DONE)) continue;
				const int hz = _hz;
				for (int i = 0; i < kChunk; ++i) {
					if (hz) { phase += double(hz) / kRate; if (phase >= 1.0) phase -= 1.0; }
					b.data[size_t(i)] = hz ? (phase < 0.5 ? 160 : 96) : 128;
				}
				b.hdr.dwFlags &= ~DWORD(WHDR_DONE);
				waveOutWrite(_out, &b.hdr, sizeof b.hdr);
				any = true;
			}
			if (!any) Sleep(4);
		}
	}

	HWAVEOUT _out = nullptr;
	std::thread _thread;
	std::atomic<bool> _quit{false};
	std::atomic<int> _hz{0};
	Buf _bufs[4];
};

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

	void post(wl::Event::Type type) {
		wl::Event e;
		e.type = type;
		machine->input.post(e);
	}

	void mouse(wl::Event::Type type, LPARAM lp, int button) {   // window coordinates -> game pixels
		wl::Event e;
		e.type = type;
		e.x = std::clamp(int(short(LOWORD(lp))) / kScale, 0, 319);
		e.y = std::clamp(int(short(HIWORD(lp))) / kScale, 0, 199);
		e.button = button;
		machine->input.post(e);
	}

	void pump() {
		MSG msg;
		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) closed = true;
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
		machine->input.pump(*machine);
		if (closed || machine->input.quit) throw rc::ExitProgram{0};
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
		case WM_CLOSE: self->closed = true; self->post(wl::Event::Quit); DestroyWindow(h); return 0;
		case WM_DESTROY: self->closed = true; PostQuitMessage(0); return 0;
		case WM_KEYDOWN: {
			// keys without a character: arrows, function keys, cursor block (BIOS: scan code << 8, ASCII 0)
			int code = 0;
			if (wp >= VK_F1 && wp <= VK_F12) code = wl::KEY_F1 + int(wp - VK_F1);
			else switch (wp) {
			case VK_UP: code = wl::KEY_UP; break;
			case VK_DOWN: code = wl::KEY_DOWN; break;
			case VK_LEFT: code = wl::KEY_LEFT; break;
			case VK_RIGHT: code = wl::KEY_RIGHT; break;
			case VK_HOME: code = wl::KEY_HOME; break;
			case VK_END: code = wl::KEY_END; break;
			case VK_PRIOR: code = wl::KEY_PAGEUP; break;
			case VK_NEXT: code = wl::KEY_PAGEDOWN; break;
			case VK_INSERT: code = wl::KEY_INSERT; break;
			case VK_DELETE: code = wl::KEY_DELETE; break;
			default: break;
			}
			if (code) {
				wl::Event e;
				e.type = wl::Event::KeyDown;
				e.key.keycode = code;
				e.key.mods = (GetKeyState(VK_SHIFT) < 0 ? wl::KMOD_SHIFT : 0) | (GetKeyState(VK_CONTROL) < 0 ? wl::KMOD_CTRL : 0) | (GetKeyState(VK_MENU) < 0 ? wl::KMOD_ALT : 0);
				self->machine->input.post(e);
			}
			return 0;
		}
		case WM_CHAR:
			if (wp < 128) {
				wl::Event e;
				e.type = wl::Event::KeyDown;
				e.key.ascii = int(wp);
				e.key.keycode = wp >= 'A' && wp <= 'Z' ? int(wp) + 32 : int(wp);
				self->machine->input.post(e);
			}
			(void)scan;
			return 0;
		case WM_MOUSEMOVE: self->mouse(wl::Event::MouseMove, lp, 0); return 0;
		case WM_LBUTTONDOWN: self->mouse(wl::Event::MouseDown, lp, wl::BUTTON_LEFT); return 0;
		case WM_LBUTTONUP: self->mouse(wl::Event::MouseUp, lp, wl::BUTTON_LEFT); return 0;
		case WM_RBUTTONDOWN: self->mouse(wl::Event::MouseDown, lp, wl::BUTTON_RIGHT); return 0;
		case WM_RBUTTONUP: self->mouse(wl::Event::MouseUp, lp, wl::BUTTON_RIGHT); return 0;
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
		rc::boot(m, shared::readFile(image), dir, r);
		m.sys->realtime = true;
		m.input.mouseEnabled = true;
		const uint8_t ivt[4] = {0x10, 0x00, 0x10, 0xF0};   // a non-zero int 33h vector tells the game a mouse driver is installed
		for (int i = 0; i < 4; ++i) m.mem[0xCC + i] = ivt[i];
		Speaker speaker;
		m.sys->onSpeaker = [&speaker](int hz) { speaker.set(hz); };
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
