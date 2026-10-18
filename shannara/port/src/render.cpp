#include "render.h"

#include "pic.h"

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

static uint32_t crc32(const uint8_t *p, size_t n, uint32_t c = 0) {
	static uint32_t table[256];
	if (!table[1]) {
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t v = i;
			for (int k = 0; k < 8; k++)
				v = v & 1 ? 0xEDB88320u ^ (v >> 1) : v >> 1;
			table[i] = v;
		}
	}
	c = ~c;
	for (size_t i = 0; i < n; i++)
		c = table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
	return ~c;
}

static void be32(Bytes &b, uint32_t v) {
	for (int s = 24; s >= 0; s -= 8)
		b.push_back((uint8_t)(v >> s));
}

static void chunk(FILE *f, const char *type, const Bytes &data) {
	Bytes b;
	be32(b, (uint32_t)data.size());
	fwrite(b.data(), 1, 4, f);
	Bytes td(type, type + 4);
	td.insert(td.end(), data.begin(), data.end());
	fwrite(td.data(), 1, td.size(), f);
	Bytes c;
	be32(c, crc32(td.data(), td.size()));
	fwrite(c.data(), 1, 4, f);
}

void writePng(const std::string &path, const Image &img) {
	Bytes raw;
	for (int y = 0; y < img.h; y++) {
		raw.push_back(0); // filter: none
		raw.insert(raw.end(), img.rgb.begin() + (size_t)y * img.w * 3, img.rgb.begin() + (size_t)(y + 1) * img.w * 3);
	}
	Bytes z = {0x78, 0x01};
	size_t pos = 0;
	uint32_t a = 1, b = 0;
	for (uint8_t v : raw) {
		a = (a + v) % 65521;
		b = (b + a) % 65521;
	}
	while (pos < raw.size() || raw.empty()) {
		size_t n = std::min<size_t>(65535, raw.size() - pos);
		bool last = pos + n >= raw.size();
		z.push_back(last ? 1 : 0);
		z.push_back((uint8_t)(n & 0xFF));
		z.push_back((uint8_t)(n >> 8));
		z.push_back((uint8_t)(~n & 0xFF));
		z.push_back((uint8_t)((~n >> 8) & 0xFF));
		z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
		pos += n;
		if (last)
			break;
	}
	be32(z, (b << 16) | a);
	FILE *f = fopen(path.c_str(), "wb");
	if (!f)
		throw FormatError("cannot write " + path);
	static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
	fwrite(sig, 1, 8, f);
	Bytes ihdr;
	be32(ihdr, (uint32_t)img.w);
	be32(ihdr, (uint32_t)img.h);
	ihdr.push_back(8);
	ihdr.push_back(2); // RGB
	ihdr.push_back(0);
	ihdr.push_back(0);
	ihdr.push_back(0);
	chunk(f, "IHDR", ihdr);
	chunk(f, "IDAT", z);
	chunk(f, "IEND", {});
	fclose(f);
}

} // namespace shannara
