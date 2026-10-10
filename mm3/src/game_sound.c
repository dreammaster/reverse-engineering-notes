/* Music and sound effects: the game's own AdLib driver (ADLIB.DRV from MM3.CC) runs in the x86 interpreter, its OPL register
 * writes drive the OPL2 emulator, and the emulator's samples go to SDL audio.  The driver is clocked by the timer interrupt it
 * installs (PIT divisor 4006h = 72.8 Hz); here that interrupt is called from the audio callback every 1/72.8 s of audio.
 * Without an audio device the ticks follow the wall clock; headless runs use virtual time (2 ticks per keyboard poll). */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "opl.h"
#include "x86.h"

static int headless_mode;
static X86 cpu;
static Opl opl;
static int sound_on;
static uint16_t drv_seg;
static unsigned pit_div = 0x4006, pit_second;
static SDL_AudioDeviceID dev;
static double samples_to_tick;      /* audio samples left until the next timer interrupt */
static Uint32 last_ms;
static uint8_t pit_reg, opl_reg;
static uint16_t isr_off, isr_seg; /* the driver's INT 08h handler, read from the vector table right after its init */

static void io_out(void *u, uint16_t port, uint8_t v) {
	(void)u;
	if (port == 0x388) opl_reg = v;
	else if (port == 0x389) opl_write(&opl, opl_reg, v);
	else if (port == 0x43) pit_second = 0;
	else if (port == 0x40) { if (!pit_second) { pit_reg = v; pit_second = 1; } else { pit_div = pit_reg | (v << 8); if (!pit_div) pit_div = 65536; pit_second = 0; } }
}
static uint8_t io_in(void *u, uint16_t port) { (void)u; (void)port; return 0; }

static void run_isr(void) { /* the driver's INT 08h handler */
	X86 save = cpu;
	uint16_t off = isr_off, seg = isr_seg;
	cpu.sp -= 6;
	wr16(SEGP(cpu.ss), cpu.sp + 4, cpu.flags); wr16(SEGP(cpu.ss), cpu.sp + 2, 0xF000); wr16(SEGP(cpu.ss), cpu.sp, 0xFF00);
	cpu.cs = seg; cpu.ip = off;
	if (x86_run(&cpu, 0xF000, 0xFF00, 200000) && getenv("MM3_SNDLOG")) fprintf(stderr, "isr failed: ivt %04X:%04X drv_seg %04X cs:ip %04X:%04X sp %04X\n", seg, off, drv_seg, cpu.cs, cpu.ip, cpu.sp);
	cpu = save;
}

/* digital speech of the intro (S1.S..S7.S): unsigned 8-bit samples at 1193182/149 = 8008 Hz (what BLASTER.DRV's sample ISR plays),
 * mixed over the music; the game polls the player state (API 0Ch with offset 1) until the sample has ended */
#define SAMPLE_RATE 8008.0
static uint8_t *smp_buf; static uint32_t smp_len; static double smp_pos; static int smp_playing;
static void smp_mix(int16_t *out, int n) {
	for (int i = 0; i < n && smp_playing; i++) {
		uint32_t p = (uint32_t)smp_pos;
		if (p >= smp_len) { smp_playing = 0; break; }
		int v = out[i] + ((int)smp_buf[p] - 128) * 160;
		out[i] = v > 32767 ? 32767 : v < -32768 ? -32768 : v;
		smp_pos += SAMPLE_RATE / OPL_RATE;
	}
}

static double tick_samples(void) { return (double)OPL_RATE * pit_div / 1193182.0; }

static void audio_cb(void *u, Uint8 *stream, int len) {
	(void)u;
	int16_t *out = (int16_t *)stream;
	int n = len / 2;
	while (n > 0) {
		while (samples_to_tick <= 0) { run_isr(); samples_to_tick += tick_samples(); }
		int chunk = (int)samples_to_tick + 1;
		if (chunk > n) chunk = n;
		opl_samples(&opl, out, chunk);
		smp_mix(out, chunk);
		out += chunk; n -= chunk; samples_to_tick -= chunk;
	}
}

static uint16_t drv_call(uint16_t entry, const uint16_t *args, int nargs) {
	if (!sound_on) return 0;
	if (dev) SDL_LockAudioDevice(dev);
	if (getenv("MM3_SNDLOG")) { fprintf(stderr, "drv api %02X(", entry); for (int i = 0; i < nargs; i++) fprintf(stderr, "%04X ", args[i]); fprintf(stderr, ")\n"); }
	x86_call_far(&cpu, drv_seg, entry, args, nargs, 5000000);
	uint16_t ax = cpu.ax;
	if (getenv("MM3_SNDLOG")) fprintf(stderr, "  -> %04X (word_F=%04X)\n", ax, rd16(SEGP(drv_seg), 0xF));
	if (dev) SDL_UnlockAudioDevice(dev);
	return ax;
}

/* MM3_WAV=file: headless runs write the music/effects they would have played */
static int16_t *wav_buf; static size_t wav_n, wav_cap;
static void wav_write(void) {
	const char *path = getenv("MM3_WAV");
	FILE *w = path ? fopen(path, "wb") : NULL;
	if (!w) return;
	uint32_t datalen = (uint32_t)(wav_n * 2), rate = OPL_RATE, v;
	fwrite("RIFF", 1, 4, w); v = 36 + datalen; fwrite(&v, 4, 1, w); fwrite("WAVEfmt ", 1, 8, w); v = 16; fwrite(&v, 4, 1, w);
	uint16_t fmt[2] = { 1, 1 }; fwrite(fmt, 2, 2, w); fwrite(&rate, 4, 1, w); v = rate * 2; fwrite(&v, 4, 1, w);
	uint16_t ba[2] = { 2, 16 }; fwrite(ba, 2, 2, w); fwrite("data", 1, 4, w); fwrite(&datalen, 4, 1, w);
	fwrite(wav_buf, 2, wav_n, w); fclose(w);
}
static void wav_tick(void) {
	size_t n = (size_t)tick_samples();
	if (wav_n + n > wav_cap) { wav_cap = (wav_cap + n) * 2; wav_buf = realloc(wav_buf, wav_cap * 2); }
	opl_samples(&opl, wav_buf + wav_n, (int)n); wav_n += n;
}

/* timer interrupts for the no-audio-device and headless cases */
void sound_pump(int headless_polls) {
	if (!sound_on || dev) return;
	if (headless_polls) { static int np; if (getenv("MM3_SNDLOG") && ++np % 500 == 0) fprintf(stderr, "pump %d word_F=%u\n", np, rd16(SEGP(drv_seg), 0xF)); for (int i = 0; i < 2; i++) { run_isr(); smp_pos += tick_samples() * SAMPLE_RATE / OPL_RATE; if (wav_buf || getenv("MM3_WAV")) wav_tick(); } return; }
	Uint32 now = SDL_GetTicks();
	double due = (now - last_ms) * 1193182.0 / 1000.0 / pit_div;
	if (due >= 1.0) { int n = (int)due > 8 ? 8 : (int)due; for (int i = 0; i < n; i++) { run_isr(); smp_pos += tick_samples() * SAMPLE_RATE / OPL_RATE; } last_ms = now; }
}

int sound_init(int headless) {
	headless_mode = headless;
	size_t len;
	uint8_t *drv = mm3_cc_read(&G.cc, "adlib.drv", &len);
	if (!drv) { fprintf(stderr, "no adlib.drv in MM3.CC: no sound\n"); return -1; }
	drv_seg = dos_alloc((uint32_t)len + 32);
	uint16_t stack = dos_alloc(0x1000);
	if (!drv_seg || !stack) { free(drv); return -1; }
	memcpy(SEGP(drv_seg), drv, len);
	free(drv);
	opl_reset(&opl);
	memset(&cpu, 0, sizeof cpu);
	cpu.mem = MEM; cpu.out8 = io_out; cpu.in8 = io_in;
	cpu.ss = stack; cpu.sp = 0x0F00; cpu.ds = DSEG; cpu.es = DSEG;
	wr16(MEM, 0x20, 0x0FFE); wr16(MEM, 0x22, stack); SEGP(stack)[0x0FFE] = 0xCF; /* the old INT 08h handler the driver chains to: an iret (top of the driver's stack block) */
	sound_on = 1;
	uint16_t port = 0x388;
	drv_call(0, &port, 1);
	isr_off = rd16(MEM, 0x20); isr_seg = rd16(MEM, 0x22);
	DG[0x158] = DG[0x159] = 1; /* byte_28848 / byte_28849: sound effects and music available */
	last_ms = SDL_GetTicks();
	if (headless && getenv("MM3_WAV")) atexit(wav_write);
	if (!headless) {
		SDL_AudioSpec want, have;
		SDL_InitSubSystem(SDL_INIT_AUDIO);
		SDL_zero(want);
		want.freq = OPL_RATE; want.format = AUDIO_S16SYS; want.channels = 1; want.samples = 1024; want.callback = audio_cb;
		dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
		if (dev) { SDL_PauseAudioDevice(dev, 0); } else fprintf(stderr, "no audio device (%s): music is silent\n", SDL_GetError());
	}
	return 0;
}

/* hosts for the driver's API wrappers (seg009 of the executable): API 0 init, 3 restore, 6 music, 9 effect, 0Ch sample */
void host_sub_2693F(Cpu *c) { c->ax = drv_call(0, (uint16_t[]){ host_arg(c, 0) }, 1); }
void host_sub_26952(Cpu *c) { c->ax = drv_call(3, NULL, 0); }
void host_sub_26965(Cpu *c) { c->ax = drv_call(6, (uint16_t[]){ host_arg(c, 0), host_arg(c, 1) }, 2); }
/* effect id, or 0FFFFh = read the tick counter (the intro and the copy-protection delays wait on it, so a query also lets time pass
 * when there is no audio thread) */
void host_soundDriverPlay(Cpu *c) {
	uint16_t id = host_arg(c, 0);
	if (id == 0xFFFF) sound_pump(headless_mode);
	c->ax = drv_call(9, (uint16_t[]){ id }, 1);
}
/* API 0Ch: (offset, segment, length) starts a sample, (0, 0, 0) stops it, (1, 0, 0) returns non-zero while one is playing */
void host_sub_2698B(Cpu *c) {
	uint16_t off = host_arg(c, 0), seg = host_arg(c, 1), len = host_arg(c, 2);
	if (dev) SDL_LockAudioDevice(dev);
	if (seg) {
		free(smp_buf);
		smp_len = len; smp_buf = malloc(len ? len : 1); memcpy(smp_buf, SEGP(seg) + off, len);
		smp_pos = 0; smp_playing = 1;
		c->ax = 1;
	} else if (off == 1) {
		if (!dev && smp_playing && smp_pos >= smp_len) smp_playing = 0;
		c->ax = smp_playing;
	} else { smp_playing = 0; c->ax = 0; }
	if (dev) SDL_UnlockAudioDevice(dev);
}
