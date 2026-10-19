// shannara_win EXE GAMEDIR PICDIR SAVEDIR: the recompiled game in a Windows window.
// The frame store of the emulated VESA card (8-bit indices + the VGA DAC palette) is shown with GDI; mouse and keyboard events go to the host's INT 33h / INT 21h emulation.
// Everything else (timing, menus, dialogs, combat ...) is the original code running natively. Needs the data files of the retail game (not part of the repository).
//
// Automated driving (for tests): WIN_SCRIPT=file with lines "SECONDS click X Y | down X Y | up X Y | move X Y | key CHAR | scan CODE | snap FILE.png | quit",
// or WIN_SNAPSHOT=out.png,SECONDS[,X,Y,CLICK_AT] as a shorthand for one click plus one snapshot. The program exits after the last event.
#include "host.h"
#include "png.h"

#include <windows.h>
#include <windowsx.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace shannara;

static Host *g_host = nullptr;
static HWND g_hwnd = nullptr;
static std::vector<uint32_t> g_pix(640 * 480);
static int g_mx = 320, g_my = 240, g_mb = 0;
static bool g_quit = false;
static DWORD g_lastPresent = 0;

static void frameToRgb(Host &host, Image &img) {
	host.flushVram();
	img.w = 640;
	img.h = 480;
	img.rgb.resize(640 * 480 * 3);
	const uint8_t *f = host.frame(), *p = host.palette();
	for (int i = 0; i < 640 * 480; i++)
		for (int c = 0; c < 3; c++) {
			uint8_t v = p[f[i] * 3 + c] & 63;
			img.rgb[i * 3 + c] = (uint8_t)(v * 4 + (v >> 4));
		}
}

static void present() {
	g_host->flushVram();
	const uint8_t *f = g_host->frame(), *p = g_host->palette();
	uint8_t lut[256][3];
	for (int i = 0; i < 256; i++)
		for (int c = 0; c < 3; c++) {
			uint8_t v = p[i * 3 + c] & 63;
			lut[i][c] = (uint8_t)(v * 4 + (v >> 4));
		}
	for (int i = 0; i < 640 * 480; i++)
		g_pix[i] = ((uint32_t)lut[f[i]][0] << 16) | ((uint32_t)lut[f[i]][1] << 8) | lut[f[i]][2];
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = 640;
	bmi.bmiHeader.biHeight = -480;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	RECT rc;
	GetClientRect(g_hwnd, &rc);
	HDC dc = GetDC(g_hwnd);
	StretchDIBits(dc, 0, 0, rc.right, rc.bottom, 0, 0, 640, 480, g_pix.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
	ReleaseDC(g_hwnd, dc);
}

static void mouseFrom(LPARAM l, int buttons) {
	RECT rc;
	GetClientRect(g_hwnd, &rc);
	int x = GET_X_LPARAM(l), y = GET_Y_LPARAM(l);
	g_mx = rc.right ? x * 640 / rc.right : x;
	g_my = rc.bottom ? y * 480 / rc.bottom : y;
	if (g_mx < 0) g_mx = 0;
	if (g_my < 0) g_my = 0;
	if (g_mx > 639) g_mx = 639;
	if (g_my > 479) g_my = 479;
	g_mb = buttons;
	g_host->setMouse(g_mx, g_my, g_mb);
}

static int scanOf(WPARAM vk) {
	switch (vk) {
	case VK_UP: return 0x48;
	case VK_DOWN: return 0x50;
	case VK_LEFT: return 0x4B;
	case VK_RIGHT: return 0x4D;
	case VK_HOME: return 0x47;
	case VK_END: return 0x4F;
	case VK_PRIOR: return 0x49;
	case VK_NEXT: return 0x51;
	case VK_INSERT: return 0x52;
	case VK_DELETE: return 0x53;
	case VK_F11: return 0x85;
	case VK_F12: return 0x86;
	default:
		if (vk >= VK_F1 && vk <= VK_F10)
			return 0x3B + (int)(vk - VK_F1);
		return 0;
	}
}

static LRESULT CALLBACK wndProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
	switch (msg) {
	case WM_MOUSEMOVE:
		mouseFrom(l, g_mb);
		return 0;
	case WM_LBUTTONDOWN:
		SetCapture(h);
		mouseFrom(l, g_mb | 1);
		return 0;
	case WM_LBUTTONUP:
		ReleaseCapture();
		mouseFrom(l, g_mb & ~1);
		return 0;
	case WM_RBUTTONDOWN:
		mouseFrom(l, g_mb | 2);
		return 0;
	case WM_RBUTTONUP:
		mouseFrom(l, g_mb & ~2);
		return 0;
	case WM_KEYDOWN: {
		int sc = scanOf(w);
		if (sc)
			g_host->pushKey(0, sc);
		return 0;
	}
	case WM_CHAR:
		g_host->pushKey((int)w, 0);
		return 0;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		BeginPaint(h, &ps);
		EndPaint(h, &ps);
		present();
		return 0;
	}
	case WM_CLOSE:
	case WM_DESTROY:
		g_quit = true;
		return 0;
	default:
		return DefWindowProc(h, msg, w, l);
	}
}

struct Ev {
	double t = 0;
	std::string kind, path;
	int a = 0, b = 0;
};

int main(int argc, char **argv) {
	if (argc < 5) {
		fprintf(stderr, "usage: shannara_win EXE GAMEDIR PICDIR SAVEDIR\n");
		return 2;
	}
	try {
		Host host(argv[1], argv[2], argv[3], argv[4]);
		g_host = &host;
		host.verbose = getenv("HOST_VERBOSE") != nullptr;
		host.loadNames("reversing/analysis/retail_function_map.tsv");
		WNDCLASSA wc = {};
		wc.lpfnWndProc = wndProc;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.lpszClassName = "ShannaraNative";
		RegisterClassA(&wc);
		RECT r = {0, 0, 640, 480};
		AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, FALSE);
		g_hwnd = CreateWindowA("ShannaraNative", "Shannara (native recompile)", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
		                       nullptr, nullptr, wc.hInstance, nullptr);
		ShowWindow(g_hwnd, SW_SHOW);

		std::vector<Ev> script;
		auto addEv = [&](double t, const std::string &k, int a, int b, const std::string &path) {
			Ev e;
			e.t = t;
			e.kind = k;
			e.a = a;
			e.b = b;
			e.path = path;
			script.push_back(e);
		};
		if (const char *sf = getenv("WIN_SCRIPT")) {
			FILE *f = fopen(sf, "r");
			char line[512];
			while (f && fgets(line, sizeof line, f)) {
				double t = 0;
				char kind[32] = {}, path[400] = {};
				int a = 0, b = 0;
				if (sscanf(line, "%lf %31s", &t, kind) < 2)
					continue;
				if (!strcmp(kind, "snap"))
					sscanf(line, "%lf %31s %399s", &t, kind, path);
				else
					sscanf(line, "%lf %31s %d %d", &t, kind, &a, &b);
				addEv(t, kind, a, b, path);
			}
			if (f)
				fclose(f);
		} else if (const char *snap = getenv("WIN_SNAPSHOT")) {
			std::string t = snap;
			size_t c1 = t.find(',');
			std::string snapPath = t.substr(0, c1);
			double v[4] = {0, 0, 0, -1};
			int n = 0;
			size_t pos = c1 + 1;
			while (n < 4 && pos < t.size()) {
				size_t q = t.find(',', pos);
				v[n++] = atof(t.substr(pos, q == std::string::npos ? std::string::npos : q - pos).c_str());
				if (q == std::string::npos)
					break;
				pos = q + 1;
			}
			if (v[3] >= 0)
				addEv(v[3], "click", (int)v[1], (int)v[2], "");
			addEv(v[0], "snap", 0, 0, snapPath);
			addEv(v[0] + 0.1, "quit", 0, 0, "");
		}
		size_t nextEv = 0;
		DWORD t0 = GetTickCount();
		bool pendingUp = false;
		double upAt = 0;
		host.onPump = [&]() {
			double el = (GetTickCount() - t0) / 1000.0;
			if (pendingUp && el >= upAt) {
				host.setMouse(g_mx, g_my, 0);
				pendingUp = false;
			}
			while (nextEv < script.size() && script[nextEv].t <= el) {
				const Ev e = script[nextEv++];
				if (e.kind == "click") {
					g_mx = e.a;
					g_my = e.b;
					host.setMouse(g_mx, g_my, 1);
					pendingUp = true;
					upAt = el + 0.25;
				} else if (e.kind == "down") {
					g_mx = e.a;
					g_my = e.b;
					host.setMouse(g_mx, g_my, 1);
				} else if (e.kind == "up" || e.kind == "move") {
					g_mx = e.a;
					g_my = e.b;
					host.setMouse(g_mx, g_my, 0);
				} else if (e.kind == "key") {
					host.pushKey(e.a, 0);
				} else if (e.kind == "scan") {
					host.pushKey(0, e.a);
				} else if (e.kind == "snap") {
					Image img;
					frameToRgb(host, img);
					writePng(e.path, img);
				} else if (e.kind == "stack") {
					fprintf(stderr, "call stack:");
					for (uint32_t a : host.machine().trace)
						fprintf(stderr, " %s >", host.describe(a).c_str());
					fprintf(stderr, "\n");
				} else if (e.kind == "quit") {
					throw ProgramExit{0};
				}
			}
			MSG msg;
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (g_quit)
				throw ProgramExit{0};
			DWORD now = GetTickCount();
			if (now - g_lastPresent >= 33) {
				g_lastPresent = now;
				present();
			}
			Sleep(1);
		};
		int rc = host.run(INT64_MAX / 2);
		fprintf(stderr, "game ended (%d)\n", rc);
	} catch (const std::exception &x) {
		fprintf(stderr, "error: %s\n", x.what());
		return 1;
	}
	return 0;
}
