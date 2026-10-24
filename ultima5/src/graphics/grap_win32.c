// Native Win32 (GDI) graphics backend, used by the MinGW build when SDL3 is not available.
//
// Set U5_HEADLESS=1 to run without creating a window (automated testing).  Screenshots are written as PNG
// with GRAP_WIN32_SaveScreenshot(); see key_win32.c for the U5_KEYS script that can trigger them.

#include "common/common.h"
#include "common/settings.h"

#include "grap_buf.h"
#include "grap_ops.h"
#include "origin.h"
#include "wd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef ARRAYSIZE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

extern void KEY_WIN32_OnKeyDown(unsigned vk, int ctrl);
extern void KEY_WIN32_OnChar(unsigned ch, int ctrl);

static HWND s_hwnd;
static int s_headless;
static u32 s_rgb[hiresWidth * hiresHeight];
static int s_windowWidth = 1280;
static int s_windowHeight = 960;

static const u32 s_egaPalette[16] = {
    0x000000, 0x0000aa, 0x00aa00, 0x00aaaa, 0xaa0000, 0xaa00aa, 0xaa5500, 0xaaaaaa,
    0x555555, 0x5555ff, 0x55ff55, 0x55ffff, 0xff5555, 0xff55ff, 0xffff55, 0xffffff,
};

static void LinearToRGB(void)
{
    for (int y = 0; y < loresHeight; y++)
    {
        for (int x = 0; x < loresWidth; x++)
        {
            u32 color = s_egaPalette[g_linearEgaBuffer0[y * loresWidth + x] & 0xf];
            u32* dst = &s_rgb[(y * 2) * hiresWidth + (x * 2)];
            dst[0] = dst[1] = dst[hiresWidth] = dst[hiresWidth + 1] = color;
        }
    }

#if defined(ENABLE_GRAP_OVERLAY)
    for (int i = 0; i < hiresWidth * hiresHeight; i++)
    {
        byte pixel = g_linearOverlayBuffer[i] & 0xf;
        if (pixel != 0)
            s_rgb[i] = s_egaPalette[pixel];
    }
#endif
}

static void Paint(HDC dc)
{
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = hiresWidth;
    bmi.bmiHeader.biHeight = -hiresHeight; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    RECT rc;
    GetClientRect(s_hwnd, &rc);
    SetStretchBltMode(dc, HALFTONE);
    StretchDIBits(dc, 0, 0, rc.right, rc.bottom, 0, 0, hiresWidth, hiresHeight, s_rgb, &bmi, DIB_RGB_COLORS, SRCCOPY);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        exit(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        Paint(dc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        KEY_WIN32_OnKeyDown((unsigned)wp, (GetKeyState(VK_CONTROL) & 0x8000) != 0);
        return 0;

    case WM_CHAR:
        KEY_WIN32_OnChar((unsigned)wp, (GetKeyState(VK_CONTROL) & 0x8000) != 0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

void GRAP_WIN32_PumpMessages(void)
{
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

int GRAP_WIN32_IsHeadless(void)
{
    return s_headless;
}

// ---------------------------------------------------------------------------------------------------------
// PNG screenshot (uncompressed deflate; no external dependencies)

static u32 s_crcTable[256];

static void CrcInit(void)
{
    for (u32 n = 0; n < 256; n++)
    {
        u32 c = n;
        for (int k = 0; k < 8; k++)
            c = (c & 1) ? 0xedb88320u ^ (c >> 1) : c >> 1;
        s_crcTable[n] = c;
    }
}

static u32 Crc(u32 crc, const u8* buf, size_t len)
{
    crc = ~crc;
    while (len--)
        crc = s_crcTable[(crc ^ *buf++) & 0xff] ^ (crc >> 8);
    return ~crc;
}

static void PutBE32(u8* p, u32 v)
{
    p[0] = (u8)(v >> 24);
    p[1] = (u8)(v >> 16);
    p[2] = (u8)(v >> 8);
    p[3] = (u8)v;
}

static void WriteChunk(FILE* fp, const char* type, const u8* data, u32 len)
{
    u8 tmp[4];
    PutBE32(tmp, len);
    fwrite(tmp, 1, 4, fp);
    fwrite(type, 1, 4, fp);
    if (len)
        fwrite(data, 1, len, fp);
    u32 crc = Crc(0, (const u8*)type, 4);
    crc = Crc(crc, data, len);
    PutBE32(tmp, crc);
    fwrite(tmp, 1, 4, fp);
}

int GRAP_WIN32_SaveScreenshot(const char* path)
{
    FILE* fp = fopen(path, "wb");
    if (!fp)
        return 0;

    if (!s_crcTable[1])
        CrcInit();

    LinearToRGB();

    size_t rawLen = (size_t)hiresHeight * (1 + hiresWidth * 3);
    u8* raw = malloc(rawLen);
    u8* p = raw;
    for (int y = 0; y < hiresHeight; y++)
    {
        *p++ = 0; // filter: none
        for (int x = 0; x < hiresWidth; x++)
        {
            u32 c = s_rgb[y * hiresWidth + x];
            *p++ = (u8)(c >> 16);
            *p++ = (u8)(c >> 8);
            *p++ = (u8)c;
        }
    }

    // zlib stream made of stored blocks
    size_t blocks = (rawLen + 65534) / 65535;
    size_t zlen = 2 + rawLen + blocks * 5 + 4;
    u8* z = malloc(zlen);
    u8* q = z;
    *q++ = 0x78;
    *q++ = 0x01;
    u32 a = 1, b = 0;
    for (size_t off = 0; off < rawLen;)
    {
        size_t n = rawLen - off > 65535 ? 65535 : rawLen - off;
        *q++ = (off + n == rawLen) ? 1 : 0;
        *q++ = (u8)n;
        *q++ = (u8)(n >> 8);
        *q++ = (u8)~n;
        *q++ = (u8)(~n >> 8);
        memcpy(q, raw + off, n);
        for (size_t i = 0; i < n; i++)
        {
            a = (a + raw[off + i]) % 65521;
            b = (b + a) % 65521;
        }
        q += n;
        off += n;
    }
    PutBE32(q, (b << 16) | a);
    q += 4;

    static const u8 sig[8] = {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
    fwrite(sig, 1, 8, fp);
    u8 ihdr[13];
    PutBE32(ihdr, hiresWidth);
    PutBE32(ihdr + 4, hiresHeight);
    ihdr[8] = 8; // bit depth
    ihdr[9] = 2; // RGB
    ihdr[10] = ihdr[11] = ihdr[12] = 0;
    WriteChunk(fp, "IHDR", ihdr, 13);
    WriteChunk(fp, "IDAT", z, (u32)(q - z));
    WriteChunk(fp, "IEND", NULL, 0);

    free(raw);
    free(z);
    fclose(fp);
    return 1;
}

// ---------------------------------------------------------------------------------------------------------

void GRAP_WIN32_FlushFrame(void)
{
    if (s_headless || !s_hwnd)
        return;

    LinearToRGB();

    HDC dc = GetDC(s_hwnd);
    Paint(dc);
    ReleaseDC(s_hwnd, dc);
}

void GRAP_WIN32_Initialize(void)
{
    GRAP_BUF_Initialize(GRAP_WIN32_FlushFrame);

    s_headless = getenv("U5_HEADLESS") != NULL;
    if (s_headless)
        return;

    s_windowWidth = SETTINGS_GetInt("window", "width", s_windowWidth);
    s_windowHeight = SETTINGS_GetInt("window", "height", s_windowHeight);

    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.lpszClassName = "U5Window";
    RegisterClassA(&wc);

    RECT rc = {0, 0, s_windowWidth, s_windowHeight};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    s_hwnd = CreateWindowA("U5Window", "Ultima V: Warriors of Destiny", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
                           CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, wc.hInstance, NULL);
}

void GRAP_WIN32_Cleanup(void)
{
    GRAP_BUF_Cleanup();

    if (s_hwnd)
    {
        HWND h = s_hwnd;
        s_hwnd = NULL;
        DestroyWindow(h);
    }
}

static GraphicsDriverOps s_winOps = {.Initialize = GRAP_WIN32_Initialize,
                                     .Cleanup = GRAP_WIN32_Cleanup,
                                     .Present = GRAP_BUF_Present,
                                     .FlushPendingPresent = GRAP_BUF_FlushPendingPresent,
                                     .SetPenColor = GRAP_BUF_SetPenColor,
                                     .SetPage = GRAP_BUF_SetPage,
                                     .PrintChar = GRAP_BUF_PrintChar,
                                     .ScrollWindow = GRAP_BUF_ScrollWindow,
                                     .Line = GRAP_BUF_Line,
                                     .Pset = GRAP_BUF_Pset,
                                     .FillWindow = GRAP_BUF_FillWindow,
                                     .LoadTileset = GRAP_BUF_LoadTileset,
                                     .UnloadTileset = GRAP_BUF_UnloadTileset,
                                     .AnimateTileset = GRAP_BUF_AnimateTileset,
                                     .UpdateTimeTileset = GRAP_BUF_UpdateTimeTileset,
                                     .ShowNextWDFrame = GRAP_BUF_ShowNextWDFrame,
                                     .AnimateWD = GRAP_BUF_AnimateWD,
                                     .AnimateOriginLogo = GRAP_BUF_AnimateOriginLogo,
                                     .PutAnimatedMoongateTile = GRAP_BUF_PutAnimatedMoongateTile,
                                     .PutTileRevealStep = GRAP_BUF_PutTileRevealStep,
                                     .PutTile = GRAP_BUF_PutTile,
                                     .PutImage = GRAP_BUF_PutImage,
                                     .PutBitImage = GRAP_BUF_PutBitImage,
                                     .TransferPage = GRAP_BUF_TransferPage,
                                     .TransferPage_Reveal = GRAP_BUF_TransferPage_Reveal};

GraphicsDriverOps* GRAP_WIN32_GetOps(void)
{
    return &s_winOps;
}
