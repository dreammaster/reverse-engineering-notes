// The C runtime routines (Watcom register convention: eax, edx, ebx) as native shims; shared by the headless game-layer engine and the full-program engine.
#include "machine.h"

#include <cctype>

namespace shannara {

static uint32_t strlen_m(Machine &m, uint32_t a) {
	uint32_t n = 0;
	while (m.r8(a + n))
		n++;
	return n;
}

void installCrtShims(Machine &m) {
	// pure C runtime routines (the oracle runs the original code for these; semantics are the standard ones)
	m.setShim("memmove_", [](Machine &mm) {
		uint32_t dst = mm.eax, src = mm.edx, n = mm.ebx;
		if (n > Machine::kMemSize) throw FormatError("memmove size");
		std::vector<uint8_t> tmp(n);
		for (uint32_t i = 0; i < n; i++) tmp[i] = mm.r8(src + i);
		for (uint32_t i = 0; i < n; i++) mm.w8(dst + i, tmp[i]);
		mm.finishShim("memmove_");
	});
	m.setShim("memcpy_", [](Machine &mm) {
		uint32_t dst = mm.eax, src = mm.edx, n = mm.ebx;
		for (uint32_t i = 0; i < n; i++) mm.w8(dst + i, mm.r8(src + i));
		mm.finishShim("memcpy_");
	});
	m.setShim("memset_", [](Machine &mm) {
		uint32_t dst = mm.eax, v = mm.edx & 0xFF, n = mm.ebx;
		for (uint32_t i = 0; i < n; i++) mm.w8(dst + i, (uint8_t)v);
		mm.finishShim("memset_");
	});
	m.setShim("strlen_", [](Machine &mm) { mm.eax = strlen_m(mm, mm.eax); mm.finishShim("strlen_"); });
	m.setShim("strcmp_", [](Machine &mm) {
		uint32_t a = mm.eax, b = mm.edx;
		int r = 0;
		for (;; a++, b++) {
			int x = mm.r8(a), y = mm.r8(b);
			if (x != y) { r = x < y ? -1 : 1; break; }
			if (!x) break;
		}
		mm.eax = (uint32_t)r;
		mm.finishShim("strcmp_");
	});
	m.setShim("stricmp_", [](Machine &mm) {
		uint32_t a = mm.eax, b = mm.edx;
		int r = 0;
		for (;; a++, b++) {
			int x = toupper(mm.r8(a)), y = toupper(mm.r8(b));
			if (x != y) { r = x < y ? -1 : 1; break; }
			if (!x) break;
		}
		mm.eax = (uint32_t)r;
		mm.finishShim("stricmp_");
	});
	m.setShim("strcpy_", [](Machine &mm) {
		uint32_t dst = mm.eax, src = mm.edx, i = 0;
		for (;; i++) { uint8_t c = mm.r8(src + i); mm.w8(dst + i, c); if (!c) break; }
		mm.finishShim("strcpy_");
	});
	m.setShim("toupper_", [](Machine &mm) { mm.eax = (uint32_t)toupper((int)(mm.eax & 0xFF)); mm.finishShim("toupper_"); });
	m.setShim("tolower_", [](Machine &mm) { mm.eax = (uint32_t)tolower((int)(mm.eax & 0xFF)); mm.finishShim("tolower_"); });
	// the rest of the C runtime routines the oracle runs for real (Watcom register convention: eax, edx, ebx)
	m.setShim("strupr_", [](Machine &mm) {
		for (uint32_t a = mm.eax; mm.r8(a); a++) mm.w8(a, (uint8_t)toupper(mm.r8(a)));
		mm.finishShim("strupr_"); // returns its argument (eax unchanged)
	});
	m.setShim("strlwr_", [](Machine &mm) {
		for (uint32_t a = mm.eax; mm.r8(a); a++) mm.w8(a, (uint8_t)tolower(mm.r8(a)));
		mm.finishShim("strlwr_");
	});
	m.setShim("strncpy_", [](Machine &mm) {
		uint32_t d = mm.eax, s = mm.edx, n = mm.ebx;
		bool end = false;
		for (uint32_t i = 0; i < n; i++) {
			uint8_t c = end ? 0 : mm.r8(s + i);
			if (!c) end = true;
			mm.w8(d + i, c);
		}
		mm.finishShim("strncpy_"); // returns dest (eax unchanged)
	});
	m.setShim("strncmp_", [](Machine &mm) {
		uint32_t a = mm.eax, b = mm.edx, n = mm.ebx;
		int r = 0;
		for (uint32_t i = 0; i < n; i++) {
			int x = mm.r8(a + i), y = mm.r8(b + i);
			if (x != y) { r = x < y ? -1 : 1; break; }
			if (!x) break;
		}
		mm.eax = (uint32_t)r;
		mm.finishShim("strncmp_");
	});
	m.setShim("memicmp_", [](Machine &mm) {
		uint32_t a = mm.eax, b = mm.edx, n = mm.ebx;
		int r = 0;
		for (uint32_t i = 0; i < n; i++) {
			int x = tolower(mm.r8(a + i)), y = tolower(mm.r8(b + i));
			if (x != y) { r = x < y ? -1 : 1; break; }
		}
		mm.eax = (uint32_t)r;
		mm.finishShim("memicmp_");
	});
	m.setShim("strstr_", [](Machine &mm) {
		uint32_t h = mm.eax, nd = mm.edx;
		uint32_t found = 0;
		if (!mm.r8(nd)) found = h;
		else {
			for (uint32_t i = h; mm.r8(i) && !found; i++) {
				uint32_t k = 0;
				while (mm.r8(nd + k) && mm.r8(i + k) == mm.r8(nd + k)) k++;
				if (!mm.r8(nd + k)) found = i;
			}
		}
		mm.eax = found;
		mm.finishShim("strstr_");
	});
}

} // namespace shannara
