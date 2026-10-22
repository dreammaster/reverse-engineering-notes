// Save files: DISK<slot> (the 0xEC0-byte Disk1 record, as in the original) and MAPS<slot> (our own container of the
// map chunks the party has changed, standing in for the map files KEH rewrites on the save disk).
#include "fod/savegame.h"
#include <cstdio>

namespace fod {

namespace {
void writeFile(const std::string &path, const Bytes &data) {
	std::FILE *f = std::fopen(path.c_str(), "wb");
	if (!f) throw std::runtime_error("cannot write " + path);
	std::fwrite(data.data(), 1, data.size(), f);
	std::fclose(f);
}
}

bool saveGame(World &w, const std::string &dir, int slot) {
	try {
		w.syncChangesToDisk();
		++w.disk.saveCounter;
		writeFile(dir + "/DISK" + std::to_string(slot), w.disk.serialize());
		Bytes maps;
		const auto exported = w.exportMaps();
		auto put32 = [&](uint32_t v) { for (int i = 0; i < 4; ++i) maps.push_back(uint8_t(v >> (8 * i))); };
		put32(uint32_t(exported.size()));
		for (const auto &kv : exported) {
			put32(uint32_t(kv.first));
			put32(uint32_t(kv.second.size()));
			maps.insert(maps.end(), kv.second.begin(), kv.second.end());
		}
		writeFile(dir + "/MAPS" + std::to_string(slot), maps);
		return true;
	} catch (const std::exception &) {
		return false;
	}
}

bool loadGame(const std::string &dir, int slot, Disk1 &disk, std::vector<std::pair<int, Bytes>> &maps) {
	try {
		disk = Disk1::parse(readFile(dir + "/DISK" + std::to_string(slot)));
		maps.clear();
		try {
			const Bytes b = readFile(dir + "/MAPS" + std::to_string(slot));
			Reader r(b);
			const uint32_t n = r.u32();
			for (uint32_t i = 0; i < n; ++i) {
				const int no = int(r.u32());
				const uint32_t size = r.u32();
				Bytes chunk(size);
				r.bytes(chunk.data(), size);
				maps.emplace_back(no, std::move(chunk));
			}
		} catch (const std::exception &) {
			maps.clear();
		}
		return true;
	} catch (const std::exception &) {
		return false;
	}
}

int newestSave(const std::string &dir) {
	int best = 0;
	uint32_t bestCounter = 0;
	for (int slot = 1; slot <= 3; ++slot) {
		try {
			const Disk1 d = Disk1::parse(readFile(dir + "/DISK" + std::to_string(slot)));
			if (best == 0 || d.saveCounter > bestCounter) { best = slot; bestCounter = d.saveCounter; }
		} catch (const std::exception &) {
		}
	}
	return best;
}

} // namespace fod
