/* Host routines for the translated 3D view code (src/gen/view_gen.c): the C runtime, video and sound calls the original made.
 * The video module call that receives the finished draw list (vdrv_2D_printText) hands it to a callback. */
#include <stdio.h>

#include "view_host.h"

static MmViewListFn list_callback;
static void *list_user;

void mm3_view_set_list_callback(MmViewListFn fn, void *user) { list_callback = fn; list_user = user; }

static const char *cstr(unsigned off) { return (const char *)&DG[off & 0xFFFF]; }

void host__memset(Cpu *c) {
	unsigned s = host_arg(c, 0), ch = host_arg(c, 1), n = host_arg(c, 2);
	for (unsigned i = 0; i < n; i++) DG[(s + i) & 0xFFFF] = (uint8_t)ch;
	c->ax = (uint16_t)s;
}

/* sprintf with the formats the renderer uses: %p (near pointer, 4 hex digits), %u, %d, %s, optional width */
void host__sprintf(Cpu *c) {
	unsigned buf = host_arg(c, 0), fmt = host_arg(c, 1), ai = 2, n = 0;
	char tmp[64];
	for (const char *f = cstr(fmt); *f; f++) {
		if (*f != '%') { DG[(buf + n++) & 0xFFFF] = (uint8_t)*f; continue; }
		const char *spec = f + 1;
		char width[8] = "";
		int w = 0;
		while (*spec >= '0' && *spec <= '9') { if (w < 6) width[w++] = *spec; spec++; }
		width[w] = 0;
		unsigned v = host_arg(c, (int)ai++);
		switch (*spec) {
		case 'p': snprintf(tmp, sizeof tmp, "%04X", v); break;
		case 'u': { char f2[16]; snprintf(f2, sizeof f2, "%%%su", width); snprintf(tmp, sizeof tmp, f2, v); break; }
		case 'd': { char f2[16]; snprintf(f2, sizeof f2, "%%%sd", width); snprintf(tmp, sizeof tmp, f2, (int16_t)v); break; }
		case 's': snprintf(tmp, sizeof tmp, "%s", cstr(v)); break;
		default: snprintf(tmp, sizeof tmp, "?"); break;
		}
		for (char *p = tmp; *p; p++) DG[(buf + n++) & 0xFFFF] = (uint8_t)*p;
		f = spec;
	}
	DG[(buf + n) & 0xFFFF] = 0;
	c->ax = (uint16_t)n;
}

void host_vdrv_2D_printText(Cpu *c) {
	unsigned off = host_arg(c, 0);
	const unsigned char *s = (const unsigned char *)cstr(off);
	for (; *s; s++)
		if (*s == 5) { /* control code 05h + four hex digits: draw list at that DGROUP offset */
			unsigned addr = 0;
			for (int i = 1; i <= 4; i++) {
				unsigned d = s[i] <= '9' ? s[i] - '0' : s[i] - 'A' + 10;
				addr = addr * 16 + d;
			}
			if (list_callback) list_callback(list_user, addr);
			s += 4;
		}
}

void host_freeSpriteSlot(Cpu *c) {
	unsigned p = host_arg(c, 0);
	DG[p & 0xFFFF] = DG[(p + 1) & 0xFFFF] = DG[(p + 2) & 0xFFFF] = DG[(p + 3) & 0xFFFF] = 0; /* the sprite memory itself is not ours to free */
}

void host_moveMonsters(Cpu *c) { (void)c; /* TODO: monster movement (not translated yet) */ }
void host_playSoundEffect(Cpu *c) { (void)c; }
void host_vdrv_0F_fade(Cpu *c) { (void)c; }
void host_getCommand(Cpu *c) { (void)c; abort(); }
void host_vdrv_06_closeWindows(Cpu *c) { (void)c; }
void host_sub_28649(Cpu *c) { (void)c; /* overlay routine called when the map changes (not translated) */ }
void host_sub_2819E(Cpu *c) { (void)c; /* overlay routine called when the map changes (not translated) */ }

static MmViewMapOps map_ops;
void mm3_view_set_map_ops(const MmViewMapOps *ops) { map_ops = *ops; }
void host_j_loadMapData(Cpu *c) { if (map_ops.load_map_data) map_ops.load_map_data(map_ops.user, host_arg(c, 0)); }
void host_j_loadMapGraphics(Cpu *c) { if (map_ops.load_map_graphics) map_ops.load_map_graphics(map_ops.user, host_arg(c, 0)); }
void host_j_Map_load(Cpu *c) { if (map_ops.map_load) map_ops.map_load(map_ops.user, host_arg(c, 0)); }
void host_vdrv_1E_openWindow(Cpu *c) { (void)c; abort(); }

/* ---- entry points */
void call_prepareIndoorView(Cpu *c);
void call_renderIndoorView(Cpu *c);
void call_drawViewOutdoors(Cpu *c);
void call_mazeUpdateSlot(Cpu *c);

void mm3_view_run_outdoor(void) {
	Cpu c;
	memset(&c, 0, sizeof c);
	c.sp = 0xFF00;
	PUSH(&c, 0); PUSH(&c, 0);
	call_drawViewOutdoors(&c);
}

void mm3_view_update_slot(void) {
	Cpu c;
	memset(&c, 0, sizeof c);
	c.sp = 0xFF00;
	PUSH(&c, 0); PUSH(&c, 0);
	call_mazeUpdateSlot(&c);
}

void mm3_view_run(void) {
	Cpu c;
	memset(&c, 0, sizeof c);
	c.sp = 0xFF00;
	PUSH(&c, 0); PUSH(&c, 0);
	call_prepareIndoorView(&c);
	c.sp = 0xFF00;
	PUSH(&c, 0); PUSH(&c, 0);
	call_renderIndoorView(&c);
}
