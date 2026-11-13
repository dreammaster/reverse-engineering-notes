// A Windows frontend for trying the standalone game: one window (3x the 320x200 screen), the mouse and keyboard as the original uses them.
// Usage: xanth_win32 <game folder>. (An SDL2 frontend is the portable one; this needs nothing but the Windows API.)
#include <windows.h>

#include <cstdio>
#include <memory>

#include "common/files.h"
#include "xanth/display.h"
#include "xanth/interface.h"

namespace {

const int kScale = 3;

struct App : Xanth::InputSource {
	HWND window;
	Xanth::Display *display;
	Xanth::Interface *ui;
	DWORD lastTick;
	bool quit;
	std::vector<unsigned char> rgb;
	std::vector<unsigned> bgra;

	App() : window(NULL), display(NULL), ui(NULL), lastTick(0), quit(false), rgb(320 * 200 * 3), bgra(320 * 200) {}

	void paint(HDC dc) {
		Legend::Surface shown;
		display->render(shown);
		shown.toRgb(&rgb[0]);
		for (int i = 0; i < 320 * 200; i++)
			bgra[i] = ((unsigned)rgb[i * 3] << 16) | ((unsigned)rgb[i * 3 + 1] << 8) | rgb[i * 3 + 2];
		BITMAPINFO bi;
		memset(&bi, 0, sizeof(bi));
		bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bi.bmiHeader.biWidth = 320;
		bi.bmiHeader.biHeight = -200;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biCompression = BI_RGB;
		StretchDIBits(dc, 0, 0, 320 * kScale, 200 * kScale, 0, 0, 320, 200, &bgra[0], &bi, DIB_RGB_COLORS, SRCCOPY);
	}

	void repaint() {
		HDC dc = GetDC(window);
		paint(dc);
		ReleaseDC(window, dc);
	}

	void tick() {
		DWORD now = GetTickCount();
		if (lastTick)
			display->advance((unsigned)(now - lastTick));
		lastTick = now;
		repaint();
	}

	// the next thing the player does, while the game waits (a conversation, "click to go on")
	virtual Xanth::InputEvent wait() {
		Xanth::InputEvent e;
		MSG m;
		repaint();
		while (!quit && GetMessage(&m, NULL, 0, 0)) {
			if (m.message == WM_MOUSEMOVE || m.message == WM_LBUTTONDOWN) {
				e.type = m.message == WM_LBUTTONDOWN ? Xanth::InputEvent::Click : Xanth::InputEvent::Move;
				e.x = (short)LOWORD(m.lParam) / kScale;
				e.y = (short)HIWORD(m.lParam) / kScale;
				display->setMouse(e.x, e.y);
				return e;
			}
			if (m.message == WM_KEYDOWN) {
				e.type = Xanth::InputEvent::Key;
				e.key = m.wParam == VK_UP ? 0x148 : m.wParam == VK_DOWN ? 0x150 : m.wParam == VK_RETURN ? 0x0D : (int)m.wParam;
				return e;
			}
			if (m.message == WM_TIMER)
				tick();
			else {
				TranslateMessage(&m);
				DispatchMessage(&m);
			}
		}
		e.type = Xanth::InputEvent::Quit;
		return e;
	}
};

App g_app;

LRESULT CALLBACK proc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_DESTROY:
		g_app.quit = true;
		PostQuitMessage(0);
		return 0;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(w, &ps);
		g_app.paint(dc);
		EndPaint(w, &ps);
		return 0;
	}
	case WM_MOUSEMOVE:
		g_app.ui->move((short)LOWORD(lp) / kScale, (short)HIWORD(lp) / kScale);
		return 0;
	case WM_LBUTTONDOWN:
		g_app.ui->click((short)LOWORD(lp) / kScale, (short)HIWORD(lp) / kScale);
		g_app.repaint();
		return 0;
	case WM_TIMER:
		g_app.tick();
		return 0;
	}
	return DefWindowProc(w, msg, wp, lp);
}

} // namespace

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("usage: xanth_win32 <game folder>\n");
		return 2;
	}
	Common::GameFiles gf;
	Common::Bytes dat, str;
	if (!gf.setFolder(argv[1]) || !gf.load("OBJECT.DAT", dat) || !gf.load("XANTHSTR.DAT", str)) {
		printf("cannot read the game files in %s\n", argv[1]);
		return 1;
	}
	Xanth::ObjectNames names;
	Legend::MessageFile messages;
	if (!names.load(dat) || !messages.load(str)) {
		printf("cannot load OBJECT.DAT / XANTHSTR.DAT\n");
		return 1;
	}
	Xanth::Display display(gf);
	if (!display.init()) {
		printf("cannot load the interface pictures and fonts\n");
		return 1;
	}
	Xanth::Game game(display, messages, names);
	display.attach(&game);
	game.newGame();
	Xanth::Interface ui(game, display);
	g_app.display = &display;
	g_app.ui = &ui;
	display.setInput(&g_app);

	WNDCLASSA wc;
	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = proc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = "XanthWin";
	RegisterClassA(&wc);
	RECT r = {0, 0, 320 * kScale, 200 * kScale};
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE);
	g_app.window = CreateWindowA("XanthWin", "Companions of Xanth", (WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX)) | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
	                             r.right - r.left, r.bottom - r.top, NULL, NULL, wc.hInstance, NULL);
	SetTimer(g_app.window, 1, 50, NULL);
	ShowCursor(FALSE);
	ui.start();
	MSG m;
	while (GetMessage(&m, NULL, 0, 0)) {
		TranslateMessage(&m);
		DispatchMessage(&m);
	}
	return 0;
}
