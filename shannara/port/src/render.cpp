#include "render.h"

#include "pic.h"
#include "png.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <sstream>

namespace shannara {

static uint8_t scale6(uint8_t v) {
	v &= 63;
	return (uint8_t)std::min(255, v * 4 + (v >> 4));
}

namespace {
struct PicCache {
	std::string dir;
	std::map<int, std::unique_ptr<PicFile>> files;
	PicFile &get(int file) {
		auto it = files.find(file);
		if (it == files.end()) {
			char name[64];
			snprintf(name, sizeof name, "/SHAN%03d.PIC", file);
			it = files.emplace(file, std::make_unique<PicFile>(readFile(dir + name))).first;
		}
		return *it->second;
	}
};
} // namespace

Image renderRoom(Engine &e, const std::string &picDir, RenderInfo *info) {
	Machine &m = e.machine();
	RenderInfo ri;
	ri.room = m.r16(0xADE94 + 2);
	ri.picId = (int)(e.callFunction(0x1D324, (uint32_t)ri.room) & 0xFFFF); // get_room_pic_id

	// The draw phase is a request stream: the room handler calls setup_pic_regions?(table, count). A temporary shim evaluates the game's own overlay_visible? for every entry
	// at that very moment (the handler may have changed state just before) and records the visible ones; the whole machine state is restored afterwards.
	struct Overlay { int picId; };
	std::vector<std::vector<Overlay>> tables;
	int entriesSeen = 0;
	Bytes saved = m.mem;
	uint32_t savedHeap = m.heapPtr;
	std::string savedText = e.rawOutput();
	m.setShim("setup_pic_regions?", [&](Machine &mm) {
		uint32_t table = mm.eax, count = mm.edx;
		uint32_t sEax = mm.eax, sEcx = mm.ecx, sEdx = mm.edx, sEbx = mm.ebx, sEsi = mm.esi, sEdi = mm.edi, sEbp = mm.ebp;
		std::vector<Overlay> vis;
		for (uint32_t i = 0; i < count && i < 256; i++) {
			uint32_t entry = table + 12 * i;
			entriesSeen++;
			mm.push(0);
			mm.eax = entry;
			mm.call(0x1BC20); // overlay_visible?
			if (mm.eax & 0xFF)
				vis.push_back({(int)mm.r16(entry + 4)});
		}
		tables.push_back(vis);
		mm.eax = sEax; mm.ecx = sEcx; mm.edx = sEdx; mm.ebx = sEbx; mm.esi = sEsi; mm.edi = sEdi; mm.ebp = sEbp;
		mm.eax = 0;
		mm.finishShim("setup_pic_regions?");
	});
	try {
		e.callFunction(0x21AF4, (uint32_t)ri.room, 0x16); // call_object_handler?(room, draw)
	} catch (...) {
		m.clearShim("setup_pic_regions?");
		m.mem = saved;
		m.heapPtr = savedHeap;
		throw;
	}
	m.clearShim("setup_pic_regions?");
	m.mem = saved;
	m.heapPtr = savedHeap;
	e.takeText();
	ri.tables = (int)tables.size();
	ri.entries = entriesSeen;

	PicCache cache;
	cache.dir = picDir;
	PicFile &bgFile = cache.get(ri.picId >> 8);
	PicImage bg = bgFile.load(ri.picId & 0xFF);
	const PicEntry &bgEntry = bgFile.entries()[ri.picId & 0xFF];
	int W = bgEntry.w, H = bgEntry.h;
	Bytes img(bg.pixels.begin(), bg.pixels.begin() + (size_t)W * H);
	uint8_t pal[768] = {};
	if (bg.hasPalette)
		memcpy(pal, bg.palette, 768);

	for (auto &t : tables) {
		for (auto &ov : t) {
			int picId = ov.picId;
			PicFile &f = cache.get(picId >> 8);
			if ((size_t)(picId & 0xFF) >= f.entries().size())
				continue;
			const PicEntry &pe = f.entries()[picId & 0xFF];
			PicImage sp = f.load(picId & 0xFF);
			int x = sp.offsets.empty() ? 0 : sp.offsets[0].x, y = sp.offsets.empty() ? 0 : sp.offsets[0].y;
			if (x < 0 || y < 0 || x + pe.w > W || y + pe.h > H) {
				ri.skippedBounds++;
				continue;
			}
			if (sp.hasPalette)
				memcpy(pal, sp.palette, 768);
			if (pe.isXorSprite()) {
				PicFile::compositeXor(img, W, sp.pixels.data(), x, y, pe.w, pe.h);
			} else { // plain sprite (characters, props): copied over the picture; with flag 0x20 colour index 254 is transparent
				for (int yy = 0; yy < pe.h; yy++)
					for (int xx = 0; xx < pe.w; xx++) {
						uint8_t v = sp.pixels[(size_t)yy * pe.w + xx];
						if ((pe.flags & 0x20) && v == 254)
							continue;
						img[(size_t)(y + yy) * W + x + xx] = v;
					}
			}
			ri.drawn++;
		}
	}
	Image out;
	out.w = W;
	out.h = H;
	out.rgb.resize((size_t)W * H * 3);
	for (size_t k = 0; k < (size_t)W * H; k++)
		for (int c = 0; c < 3; c++)
			out.rgb[k * 3 + c] = scale6(pal[img[k] * 3 + c]);
	if (info)
		*info = ri;
	return out;
}

} // namespace shannara
