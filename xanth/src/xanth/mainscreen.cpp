#include "xanth/mainscreen.h"

#include <cstdio>

#include "legend/draw.h"
#include "legend/textwindow.h"

namespace Xanth {

using namespace Legend;

bool MainScreen::load(const Common::GameFiles &files) {
	Common::Bytes d0, d1;
	PicFile p0, p1;
	Common::Bytes f1, f10;
	if (!files.load("XANTH_01.FNT", f1) || !_font1.load(f1) || !files.load("XANTH_10.FNT", f10) || !_font10.load(f10))
		return false;
	if (!files.load("XANTH_00.PIC", d0) || !files.load("XANTH_01.PIC", d1) || !p0.load(d0) || !p1.load(d1))
		return false;
	bool ok = p0.decode(31, _marble) && p0.decode(0, _compass) && p1.decode(1, _slotPanel) && p1.decode(2, _listPanel) && p1.decode(4, _scrollPanel) &&
	          p1.decode(0, _interfacePalette);
	for (int i = 0; i < 5; i++)
		ok = ok && p0.decode(21 + i, _buttons[i]);
	for (int i = 0; i < 10; i++)
		ok = ok && p0.decode(1 + i, _arrows[i]);
	return ok;
}

void MainScreen::restoreSubPalettes(byte *pal) const {
	for (int i = 0; i < 16; i++)
		pal[i * 3] = pal[i * 3 + 1] = pal[i * 3 + 2] = (byte)(i * 63 / 15);
	memcpy(pal + 16 * 3, _marble.palette + 16 * 3, 16 * 3);
	memcpy(pal + 32 * 3, _interfacePalette.palette + 32 * 3, 48 * 3);
	pal[16 * 3] = 63; pal[16 * 3 + 1] = 63; pal[16 * 3 + 2] = 0;
	pal[17 * 3] = 0; // the original clears the 16-bit word at blue(16) / red(17)
	pal[254 * 3] = pal[254 * 3 + 1] = pal[254 * 3 + 2] = 16;
	pal[255 * 3] = pal[255 * 3 + 1] = pal[255 * 3 + 2] = 63;
}

static void pic(Surface &s, const PicImage &p, int x, int y) {
	drawPicture(s, x + p.x, y + p.y, &p.pixels[0], p.width, p.height, (p.flags & 0x40) != 0);
}

void MainScreen::drawFrame(Surface &s) const {
	byte pal[768];
	memcpy(pal, _marble.palette, 768);
	restoreSubPalettes(pal);
	s.setPalette(pal);

	pic(s, _marble, 0, 0);
	// the text window and its frame, the picture window frame, the inventory panels
	bevelFrame(s, 50, 128, 315, 149);
	fillArea(s, 51, 129, 314, 148, 23);
	bevelFrame(s, 50, 3, 315, 126);
	bevelFrame(s, 50, 151, 83, 196);
	pic(s, _slotPanel, 51, 152);
	bevelFrame(s, 87, 151, 288, 196);
	pic(s, _listPanel, 88, 152);
	bevelFrame(s, 292, 151, 315, 196);
	pic(s, _scrollPanel, 293, 152); // 22x44: both arrows dim (picture 3 has them lit)
	bevelFrame(s, 3, 128, 46, 196);
	pic(s, _compass, 4, 129);
	static const int kButtons[4][4] = {{4, 168, 24, 181}, {25, 168, 45, 181}, {4, 182, 24, 195}, {25, 182, 45, 195}};
	static const int kButtonPic[4] = {0, 1, 2, 4};
	for (int i = 0; i < 4; i++) {
		const int *b = kButtons[i];
		buttonFrame(s, b[0], b[1], b[2], b[3], true);
		pic(s, _buttons[kButtonPic[i]], b[0] + 2, b[1] + 2);
	}
}

void MainScreen::drawVerbs(Surface &s, const std::vector<std::string> &verbs) const {
	int y = 4;
	for (size_t i = 0; i < verbs.size(); i++) {
		_font1.drawText(s, 2 + 1, y + 1, verbs[i], 0);
		_font1.drawText(s, 2, y, verbs[i], 15);
		y += _font1.lineHeight();
	}
}

void MainScreen::drawCompassArrows(Surface &s, unsigned exits) const {
	for (int i = 0; i < 10; i++)
		if (exits & (1u << i))
			pic(s, _arrows[i], 4, 129);
}

void MainScreen::printMessage(Surface &s, const std::string &text) const {
	TextWindow w;
	w.setup(51, 128, 314, 151, &_font10, 15, 23);
	w.print(s, text);
}

bool MainScreen::drawRoomPicture(Surface &s, const Common::GameFiles &files, int picNum) const {
	char name[16];
	snprintf(name, sizeof(name), "XANTH_%02d.PIC", picNum >> 8);
	Common::Bytes d;
	PicFile pf;
	PicImage img;
	if (!files.load(name, d) || !pf.load(d) || !pf.decode(picNum & 255, img))
		return false;
	byte pal[768];
	memcpy(pal, img.hasPalette ? img.palette : s.palette, 768);
	restoreSubPalettes(pal);
	s.setPalette(pal);
	pic(s, img, 51, 4);
	return true;
}

} // namespace Xanth
