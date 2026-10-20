// Headless test runner for chapter tests (docs/testing.md "Story chapters, headless"): runs a ROM in
// libmgba (the WSL fork's build) with no window and no debugger, driven by a command script, so tests
// are deterministic and run faster than real time. Build and run inside WSL:
//   gcc -O2 -o /root/st_headless tools/headless/st_headless.c -I/root/mgba-build/include \
//       -I/mnt/f/MeltyCreations/tools/downloads/mgba/include -L/root/mgba-build -lmgba -Wl,-rpath,/root/mgba-build
//   /root/st_headless ROM SCRIPT [STATE]
// Script lines (numbers in C syntax; ADDR may be NAME or NAME+OFFSET for a variable set by find):
//   frames N               run N frames
//   key MASK N             hold player 1's keys MASK for N frames, then release for 4 frames
//                          (A=1 B=2 Select=4 Start=8 Right=0x10 Left=0x20 Up=0x40 Down=0x80 R=0x100 L=0x200)
//   p2 MASK | p2 off       player 2's pad: writes 0x8000 | MASK to g_p2_remote (0x0203FF00) every frame
//   shot FILE              screenshot as PPM (convert with tools/headless/st_shots.py)
//   peek ADDR [N]          print N words;  peek8 / peek16 ADDR [N]
//   poke8 / poke16 / poke32 ADDR VALUE
//   find NAME VALUE        NAME = address of the first EWRAM word equal to VALUE (0 if none)
//   deref NAME ADDR        NAME = the word at ADDR
//   keys MASK              player 1's held keys from now on (until the next keys / key)
//   mashmap ZONE AREA MAX  press A once a second until that map is loaded
//   waitnz ADDR MAX        run until the word at ADDR is nonzero (e.g. coop2.a16 0x0203FD34)
//   stepz ADDR MAX         press A on both pads every 100 frames until the byte at ADDR is 0 (a pending win)
//   mash N [F]             press A N times, F frames apart (50);  mashent NAME VTABLE MAX  ... until such an entity exists
//   ents                   list the scene entities;  findent NAME VTABLE  NAME = first entity with that vtable
//   map                    print the current map (zone, area, variation) and player 1's position
//   waitmap ZONE AREA MAX  run until that map is loaded (at most MAX frames)
//   flags +N -N ...        story flags now (PARTY_SLOTS=8 bank at 0x0203F0EC)
//   preset +N -N ...       the StoryTest preset (coop.h, 0x0203FDC0) Party_Init applies at the next Map Test
//   maptest ENTRY [VAR]    with the debug menu open: point Map Test at map entry ENTRY and press A
//   save FILE / load FILE  savestates
//   reset                  power-cycle the GBA (the save data stays: test a Continue from the title)
//   echo TEXT
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/internal/arm/arm.h>
#include <mgba/core/serialize.h>
#include <mgba-util/vfs.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

static struct mCore *core;
static color_t *video;
static unsigned vw, vh;
static int p2_on;
static unsigned p2_word;
static unsigned long frame_no;

#define MAX_VARS 16
static char var_name[MAX_VARS][32];
static unsigned var_val[MAX_VARS];
static int nvars;

static unsigned num(const char *s) {
    char name[32];
    const char *plus = strchr(s, '+');
    if (isalpha((unsigned char)s[0])) {
        size_t n = plus ? (size_t)(plus - s) : strlen(s);
        if (n >= sizeof name) n = sizeof name - 1;
        memcpy(name, s, n);
        name[n] = 0;
        for (int i = 0; i < nvars; i++)
            if (!strcmp(var_name[i], name)) return var_val[i] + (plus ? (unsigned)strtoul(plus + 1, 0, 0) : 0);
        fprintf(stderr, "unknown variable %s\n", name);
        exit(2);
    }
    return (unsigned)strtoul(s, 0, 0);
}

static void set_var(const char *name, unsigned v) {
    for (int i = 0; i < nvars; i++)
        if (!strcmp(var_name[i], name)) { var_val[i] = v; return; }
    if (nvars < MAX_VARS) { snprintf(var_name[nvars], 32, "%s", name); var_val[nvars++] = v; }
}

static unsigned r8(unsigned a) { return core->busRead8(core, a); }
static unsigned r16(unsigned a) { return core->busRead16(core, a); }
static unsigned r32(unsigned a) { return core->busRead32(core, a); }
static void w8(unsigned a, unsigned v) { core->busWrite8(core, a, v); }
static void w16(unsigned a, unsigned v) { core->busWrite16(core, a, v); }
static void w32(unsigned a, unsigned v) { core->busWrite32(core, a, v); }

static void run_frames(int n) {
    for (int i = 0; i < n; i++) {
        if (p2_on) w16(0x0203FF00, 0x8000 | (p2_word & 0x3FF));
        core->runFrame(core);
        frame_no++;
    }
}

static void shot(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return; }
    fprintf(f, "P6\n%u %u\n255\n", vw, vh);
    for (unsigned y = 0; y < vh; y++) {
        for (unsigned x = 0; x < vw; x++) {
            uint32_t c = video[y * vw + x];
            unsigned char px[3] = { c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF };
            fwrite(px, 1, 3, f);
        }
    }
    fclose(f);
    printf("shot %s (frame %lu)\n", path, frame_no);
}

static unsigned map_state(void) { return r32(0x083B5C78 + 0x10); }

static void print_map(void) {
    unsigned m = map_state();
    unsigned pl = r32(0x03001FA8);
    printf("map zone %u area %u var %u player %d,%d (frame %lu)\n", r8(m), r8(m + 1), r8(m + 2),
           pl ? (int)r32(pl + 0x158) >> 8 : -1, pl ? (int)r32(pl + 0x15C) >> 8 : -1, frame_no);
}

static void flag_ops(char **tok, int n, unsigned set_base, int preset) {
    int si = 0, ci = 0;
    if (preset) {
        w32(0x0203FDC0, 0x53545259);
        for (int i = 0; i < 16; i++) w16(0x0203FDC4 + 2 * i, 0xFFFF);
        for (int i = 0; i < 8; i++) w16(0x0203FDE4 + 2 * i, 0xFFFF);
    }
    for (int i = 0; i < n; i++) {
        unsigned f = (unsigned)strtoul(tok[i] + 1, 0, 0);
        if (preset) {
            if (tok[i][0] == '+' && si < 16) w16(0x0203FDC4 + 2 * si++, f);
            else if (tok[i][0] == '-' && ci < 8) w16(0x0203FDE4 + 2 * ci++, f);
        } else {
            unsigned a = set_base + f / 8, b = 1u << (f % 8);
            w8(a, tok[i][0] == '+' ? (r8(a) | b) : (r8(a) & ~b));
        }
    }
    printf("%s:", preset ? "preset" : "flags");
    for (int i = 0; i < n; i++) printf(" %s", tok[i]);
    printf("\n");
}

static void press(unsigned mask, int n) {
    core->setKeys(core, mask);
    run_frames(n);
    core->setKeys(core, 0);
    run_frames(4);
}

static int crashes;
static void quiet_log(struct mLogger *l, int category, enum mLogLevel level, const char *fmt, va_list args) {
    (void)l; (void)category;
    if ((level & (mLOG_FATAL | mLOG_ERROR)) && strstr(fmt, "invalid address") && crashes++ < 1 && core) {
        struct ARMCore *cpu = core->cpu;   // the first jump to nowhere: who jumped
        fprintf(stderr, "CRASH regs:");
        for (int i = 0; i < 16; i++) fprintf(stderr, " r%d=%08x", i, cpu->gprs[i]);
        fputc(10, stderr);
    }
    if (crashes > 3) return;
    if (level & (mLOG_FATAL | mLOG_ERROR)) { vfprintf(stderr, fmt, args); fputc(10, stderr); }
}
static struct mLogger logger = { .log = quiet_log };

int main(int argc, char **argv) {
    mLogSetDefaultLogger(&logger);
    if (argc < 3) { fprintf(stderr, "usage: st_headless ROM SCRIPT [STATE]\n"); return 1; }
    core = mCoreFind(argv[1]);
    if (!core || !core->init(core)) { fprintf(stderr, "no core for %s\n", argv[1]); return 1; }
    mCoreInitConfig(core, NULL);
    core->desiredVideoDimensions(core, &vw, &vh);
    video = calloc(vw * vh, sizeof(color_t));
    core->setVideoBuffer(core, video, vw);
    if (!mCoreLoadFile(core, argv[1])) { fprintf(stderr, "cannot load %s\n", argv[1]); return 1; }
    core->reset(core);
    if (argc > 3) {
        struct VFile *vf = VFileOpen(argv[3], O_RDONLY);
        if (!vf || !mCoreLoadStateNamed(core, vf, SAVESTATE_SAVEDATA)) { fprintf(stderr, "cannot load state %s\n", argv[3]); return 1; }
        vf->close(vf);
    }
    FILE *sf = !strcmp(argv[2], "-") ? stdin : fopen(argv[2], "r");
    if (!sf) { perror(argv[2]); return 1; }
    char line[512];
    while (fgets(line, sizeof line, sf)) {
        char *hash = strchr(line, '#');
        if (hash) *hash = 0;
        char *tok[32];
        int n = 0;
        for (char *t = strtok(line, " \t\r\n"); t && n < 32; t = strtok(0, " \t\r\n")) tok[n++] = t;
        if (!n) continue;
        const char *c = tok[0];
        if (!strcmp(c, "frames")) run_frames((int)num(tok[1]));
        else if (!strcmp(c, "key")) press(num(tok[1]), n > 2 ? (int)num(tok[2]) : 6);
        else if (!strcmp(c, "p2")) { if (!strcmp(tok[1], "off")) { p2_on = 0; w16(0x0203FF00, 0); } else { p2_on = 1; p2_word = num(tok[1]); } }
        else if (!strcmp(c, "shot")) shot(tok[1]);
        else if (!strcmp(c, "peek") || !strcmp(c, "peek8") || !strcmp(c, "peek16")) {
            unsigned a = num(tok[1]), k = n > 2 ? num(tok[2]) : 1;
            int sz = c[4] == '8' ? 1 : c[4] == '1' ? 2 : 4;
            printf("%08x:", a);
            for (unsigned i = 0; i < k; i++)
                printf(sz == 1 ? " %02x" : sz == 2 ? " %04x" : " %08x", sz == 1 ? r8(a + i) : sz == 2 ? r16(a + 2 * i) : r32(a + 4 * i));
            printf("\n");
        }
        else if (!strcmp(c, "poke8")) w8(num(tok[1]), num(tok[2]));
        else if (!strcmp(c, "poke16")) w16(num(tok[1]), num(tok[2]));
        else if (!strcmp(c, "poke32")) w32(num(tok[1]), num(tok[2]));
        else if (!strcmp(c, "find")) {
            unsigned v = num(tok[2]), at = 0;
            for (unsigned a = 0x02000000; a < 0x02040000; a += 4) if (r32(a) == v) { at = a; break; }
            set_var(tok[1], at);
            printf("find %s = %08x\n", tok[1], at);
        }
        else if (!strcmp(c, "deref")) { set_var(tok[1], r32(num(tok[2]))); printf("deref %s = %08x\n", tok[1], r32(num(tok[2]))); }
        else if (!strcmp(c, "keys")) core->setKeys(core, num(tok[1]));
        else if (!strcmp(c, "map")) print_map();
        else if (!strcmp(c, "ents")) {      // the entity list (0x03001120: count +4, items +8)
            unsigned cnt = r32(0x03001124);
            printf("entities %u\n", cnt);
            for (unsigned i = 0; i < cnt && i < 64; i++) {
                unsigned e = r32(0x03001128 + 4 * i);
                printf("  ent %2u %08x vt %08x x %d y %d w180 %08x hp %d\n", i, e, r32(e), (int)r32(e + 0x158) >> 8,
                       (int)r32(e + 0x15C) >> 8, r32(e + 0x180), (int)r32(e + 0x188));
            }
        }
        else if (!strcmp(c, "mashent")) {   // mashent NAME VTABLE MAX: press A once a second until such an entity exists
            unsigned v = num(tok[2]), mx = num(tok[3]), at = 0, k;
            for (k = 0; k < mx && !at; k++) {
                press(1, 6);
                run_frames(50);
                unsigned cnt = r32(0x03001124);
                for (unsigned i = 0; i < cnt && i < 0x182; i++) { unsigned e = r32(0x03001128 + 4 * i); if (r32(e) == v) { at = e; break; } }
            }
            set_var(tok[1], at);
            printf("mashent %s = %08x after %u presses\n", tok[1], at, k);
        }
        else if (!strcmp(c, "mashmap")) {  // mashmap ZONE AREA MAX: press A once a second until that map is loaded
            unsigned z = num(tok[1]), ar = num(tok[2]), mx = num(tok[3]), k;
            for (k = 0; k < mx; k++) { unsigned m = map_state(); if (r8(m) == z && r8(m + 1) == ar) break; press(1, 6); run_frames(50); }
            printf("mashmap %u %u: %s after %u presses \n", z, ar, k < mx ? "reached" : "TIMEOUT", k);
        }
        else if (!strcmp(c, "waitnz")) {   // waitnz ADDR MAX: run until the word at ADDR is nonzero
            unsigned a = num(tok[1]), mx = num(tok[2]), i;
            for (i = 0; i < mx && !r32(a); i++) run_frames(1);
            printf("waitnz %08x: %s after %u frames\n", a, i < mx ? "set" : "TIMEOUT", i);
        }
        else if (!strcmp(c, "stepz")) {   // stepz ADDR MAX: A on both pads every 100 frames until the byte at ADDR is 0
            unsigned a = num(tok[1]), mx = num(tok[2]), k, was_on = p2_on, was = p2_word;
            for (k = 0; k < mx && r8(a); k++) {
                p2_on = 1; p2_word = was | 1;
                press(1, 6);
                p2_on = was_on; p2_word = was;
                if (!p2_on) w16(0x0203FF00, 0);
                run_frames(94);
            }
            printf("stepz %08x: %s after %u steps (frame %lu)\n", a, k < mx ? "zero" : "TIMEOUT", k, frame_no);
        }
        else if (!strcmp(c, "mash")) { unsigned k = num(tok[1]); for (unsigned i = 0; i < k; i++) { press(1, 6); run_frames(n > 2 ? (int)num(tok[2]) : 50); } }
        else if (!strcmp(c, "findent")) {   // findent NAME VTABLE: the first listed entity with that vtable
            unsigned cnt = r32(0x03001124), v = num(tok[2]), at = 0;
            for (unsigned i = 0; i < cnt && i < 0x182; i++) { unsigned e = r32(0x03001128 + 4 * i); if (r32(e) == v) { at = e; break; } }
            set_var(tok[1], at);
            printf("findent %s = %08x\n", tok[1], at);
        }
        else if (!strcmp(c, "waitmap")) {
            unsigned z = num(tok[1]), ar = num(tok[2]), mx = num(tok[3]), i;
            for (i = 0; i < mx; i++) { unsigned m = map_state(); if (r8(m) == z && r8(m + 1) == ar) break; run_frames(1); }
            printf("waitmap %u %u: %s after %u frames\n", z, ar, i < mx ? "reached" : "TIMEOUT", i);
        }
        else if (!strcmp(c, "flags")) flag_ops(tok + 1, n - 1, 0x0203F0EC, 0);
        else if (!strcmp(c, "preset")) flag_ops(tok + 1, n - 1, 0, 1);
        else if (!strcmp(c, "maptest")) {
            unsigned m = 0;
            for (unsigned a = 0x02000000; a < 0x0203F000; a += 4) if (r32(a) == 0x08023EE8) { m = a; break; }
            if (!m) { printf("maptest: debug menu not found\n"); continue; }
            w32(m + 8, 2);
            w32(m + 0x14, num(tok[1]));
            w32(m + 0x18, n > 2 ? num(tok[2]) : 0);
            press(1, 6);
            printf("maptest entry %u (menu %08x)\n", num(tok[1]), m);
        }
        else if (!strcmp(c, "save") || !strcmp(c, "load")) {
            struct VFile *vf = VFileOpen(tok[1], c[0] == 's' ? O_CREAT | O_TRUNC | O_RDWR : O_RDONLY);
            int ok = vf && (c[0] == 's' ? mCoreSaveStateNamed(core, vf, SAVESTATE_SAVEDATA) : mCoreLoadStateNamed(core, vf, SAVESTATE_SAVEDATA));
            if (vf) vf->close(vf);
            printf("%s %s: %s\n", c, tok[1], ok ? "ok" : "FAILED");
        }
        else if (!strcmp(c, "reset")) { core->reset(core); p2_on = 0; printf("reset (frame %lu)\n", frame_no); }
        else if (!strcmp(c, "echo")) { for (int i = 1; i < n; i++) printf("%s ", tok[i]); printf("\n"); }
        else fprintf(stderr, "unknown command %s\n", c);
        fflush(stdout);
    }
    core->deinit(core);
    return 0;
}
