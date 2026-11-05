/* XANTH_nn.RGN: the clickable regions (hotspots) of the pictures of the same-numbered .PIC file.
 *
 * Layout: a directory with one 6-byte entry per .PIC entry {u16 dataOffset; u16 baseId; u16 count} (dataOffset 0 = no regions), then the regions
 * {i16 id; i16 type; ...}: type 2 = rectangle {x1, y1, x2, y2}, any other value = polygon with `type` vertices {x, y}. In Xanth the id is the object id of the
 * thing under the hotspot and baseId is the room's object id. The first region that contains the pointer wins.
 */
#ifndef LEGEND_RGN_H
#define LEGEND_RGN_H

#include "common/types.h"

namespace Legend {

class RgnFile {
public:
	struct Region {
		int16 id;
		bool rect;                       // true: pts = {x1, y1, x2, y2}; false: polygon, pts = x,y pairs
		std::vector<int16> pts;
		bool contains(int x, int y) const;
	};
	struct Scene {
		uint16 baseId;
		std::vector<Region> regions;
		Scene() : baseId(0) {}
	};

	bool load(const Common::Bytes &data);
	size_t size() const { return _scenes.size(); }
	const Scene &scene(size_t i) const { return _scenes[i]; }
	/** The first region of scene i that contains the point, or nullptr. */
	const Region *hit(size_t i, int x, int y) const;

private:
	std::vector<Scene> _scenes;
};

} // namespace Legend

#endif
