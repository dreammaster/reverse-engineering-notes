#ifdef _WIN32

#include "engine/backend.h"

#include <windows.h>
#include <mmsystem.h>

#include "engine/music_player.h"

#include <deque>
#include <memory>

namespace Ultima6 {

namespace {

// The sound card: the AdLib driver (MusicPlayer) renders into four waveOut buffers from a thread of its own.
class MusicOutput {
public:
	MusicOutput() : _hwo(nullptr), _thread(nullptr), _quit(false) {
		InitializeCriticalSection(&_lock);
		WAVEFORMATEX fmt = {};
		fmt.wFormatTag = WAVE_FORMAT_PCM;
		fmt.nChannels = 1;
		fmt.nSamplesPerSec = Opl2::SAMPLE_RATE;
		fmt.wBitsPerSample = 16;
		fmt.nBlockAlign = 2;
		fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * 2;
		_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
		if (waveOutOpen(&_hwo, WAVE_MAPPER, &fmt, (DWORD_PTR)_event, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR) {
			_hwo = nullptr;
			return;
		}
		for (int i = 0; i < BUFFERS; ++i) {
			_hdr[i] = WAVEHDR();
			_hdr[i].lpData = (LPSTR)_pcm[i];
			_hdr[i].dwBufferLength = SAMPLES * 2;
			waveOutPrepareHeader(_hwo, &_hdr[i], sizeof(WAVEHDR));
			fill(i);
		}
		_thread = CreateThread(nullptr, 0, &MusicOutput::run, this, 0, nullptr);
	}

	~MusicOutput() {
		_quit = true;
		if (_thread) {
			SetEvent(_event);
			WaitForSingleObject(_thread, 2000);
			CloseHandle(_thread);
		}
		if (_hwo) {
			waveOutReset(_hwo);
			for (int i = 0; i < BUFFERS; ++i)
				waveOutUnprepareHeader(_hwo, &_hdr[i], sizeof(WAVEHDR));
			waveOutClose(_hwo);
		}
		CloseHandle(_event);
		DeleteCriticalSection(&_lock);
	}

	void play(const std::vector<uint8_t> &track) {
		EnterCriticalSection(&_lock);
		_player.stop();
		_player.play(track);
		LeaveCriticalSection(&_lock);
	}

	void stop() {
		EnterCriticalSection(&_lock);
		_player.stop();
		LeaveCriticalSection(&_lock);
	}

private:
	enum { BUFFERS = 4, SAMPLES = 2048 };

	void fill(int i) {
		EnterCriticalSection(&_lock);
		_player.render(_pcm[i], SAMPLES);
		LeaveCriticalSection(&_lock);
		waveOutWrite(_hwo, &_hdr[i], sizeof(WAVEHDR));
	}

	static DWORD WINAPI run(LPVOID arg) {
		MusicOutput *self = (MusicOutput *)arg;
		while (!self->_quit) {
			WaitForSingleObject(self->_event, 200);
			for (int i = 0; i < BUFFERS && !self->_quit; ++i)
				if (self->_hdr[i].dwFlags & WHDR_DONE)
					self->fill(i);
		}
		return 0;
	}

	HWAVEOUT _hwo;
	HANDLE _thread, _event;
	volatile bool _quit;
	CRITICAL_SECTION _lock;
	WAVEHDR _hdr[BUFFERS];
	int16_t _pcm[BUFFERS][SAMPLES];
	MusicPlayer _player;
};

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

	void playMusic(const std::vector<uint8_t> &track) override {
		if (!_music)
			_music.reset(new MusicOutput);
		_music->play(track);
	}

	void stopMusic() override {
		if (_music)
			_music->stop();
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
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
				self->mouseX = (int)(short)LOWORD(lp) / self->_scale;
				self->mouseY = (int)(short)HIWORD(lp) / self->_scale;
				self->_keys.push_back(msg == WM_LBUTTONDOWN ? KEY_MOUSE_LEFT : KEY_MOUSE_RIGHT);
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
	std::unique_ptr<MusicOutput> _music;
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
