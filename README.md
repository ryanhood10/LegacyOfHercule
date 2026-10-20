# The Legacy of Hercule: a co-op mod of Legacy of Goku II

A mod of *Dragon Ball Z: The Legacy of Goku II* (GBA, USA, `ALFE`) in two layers:

- **Two-player co-op**: a second player on the same screen with their own character, HUD, damage
  and EXP, a midpoint camera, stronger enemies and twin bosses, player-vs-player duels, and an
  optional wide view on big screens.
- **The Legacy of Hercule**: a story mod on top, with Hercule as the hidden hero (Super Saiyan
  Hercule, Krillin playable, new chapters, bosses and maps). *Buu's Fury* is a later phase.

It is not a rebuildable decomp. The build compiles our C (`src/coop/`) into a blob, drops it into the
stock ROM padded to 16 MiB and patches the ROM to call it (vtable slots, entry jumps, relocated
tables, data records). This repo contains code, tools and notes only; no ROM data or game assets are
committed.

Working on it with Claude? `CLAUDE.md` is the one-page quick-start and `docs/INDEX.md` maps every
document, source file and tool; `docs/log.md` says what landed last and what is open.

## ROMs

Drop dumps of your own cartridges here (git-ignored):

```
roms/log2.gba     Dragon Ball Z: The Legacy of Goku II (USA)   game code ALFE
roms/buu.gba      Dragon Ball Z: Buu's Fury (USA)              optional: a few converted sprites (left out when absent, or with BUU=0)
```

Then check them:

```
python tools/romcheck.py roms/log2.gba roms/buu.gba
```

## Ghidra

Builds a headless Ghidra project in `build/ghidra/` with the GBA memory map and functions seeded from
`bl` targets and pointer tables (Ghidra's own code finder is useless on this ROM):

```
tools\ghidra_import.ps1 roms\log2.gba
```

Open `build\ghidra\log2.gpr` in the Ghidra GUI afterwards. `tools/ghidra/DecompileFunctions.java`
writes decompiled C per function; the whole-ROM corpus in `build/decomp_all/` is the first place to
grep. Findings go in the topic doc they belong to (engine, maps, sprites, scripts, ...); `docs/INDEX.md`
says which.

## Building

`src/coop/` is compiled with `arm-none-eabi-gcc` (Thumb, arm7tdmi) into a blob linked at
`0x08800000`; `tools/patch_rom.py` pads the original to 16 MiB, drops the blob in and applies the
patches, the Hercule data (`data/hercule.json`, `data/story/`) and the new maps (`data/maps/`).
Run `make` from Git Bash with the ARM toolchain and GnuWin32 make on the PATH (`CLAUDE.md` "Build"
has the exact line; `make` does not run from PowerShell):

```
make                        # build/log2-coop.gba
make WIDE=1                 # build/log2-wide.gba, the wide view for the WSL mGBA fork
make DEBUG_MENU=1           # the title screen opens the game's leftover debug menu (Map Test)
```

The build flags (`PARTY_SLOTS`, `HERCULE`, test triggers, sprite ids) are listed in `CLAUDE.md`.

With `DEBUG_MENU=1`, leave the title screen alone: after 30 s with no key held (1800 frames) it opens
the debug menu instead of the intro video, about 75 s after power-on since the logos and intro come
first. Map Test loads any map with a level-40 party.

Debugging: the mGBA fork in WSL (`tools/fork/`, `tools\fork\runfork.ps1 <rom> -Slot n`) has a GDB stub
on port 2345+n with a persistent GDB client, and a TCP pad on 7778+n. Recipes, the headless test
runner (`tools/headless/`) and the Windows mGBA Lua helpers are in `docs/testing.md`.

## Who player 2 is

Player 2 is a party member other than player 1's character: one with party flag bits 0 (in the
party) and 1 (available), Hercule first when he qualifies. The story's flag opcodes therefore pick
and lock player 2, and with only one party member there is no player 2. Player 2 presses Select to
switch to the next available member; the two players never share a character (if player 1 takes
player 2's character at a save pad, player 2 gets player 1's old one). Player 2 appears once its pad
sends anything; with no second pad the game plays as single-player.

## Player 2's input

A GBA game sees only its own 10 buttons, so player 2's pad reaches the ROM through an EWRAM word
(`g_p2_remote`) that the emulator side writes:

| transport | use | status |
|---|---|---|
| TCP bridge (`tools/p2_keys.py` + `tools/mgba/coop.lua`) | two players on one PC (Windows mGBA) | works |
| lockstep netplay (`tools/mgba/netplay.lua` + `tools/netplay_relay.py`) | online, each player runs the ROM | works (tested on one PC, two instances) |
| fork pad port (`MGBA_P2=1`, `tools/p2_keys.py --port`) | the WSL fork, wide view | works |
| link cable (`P2_MODE` 0, a second GBA as pad) | real hardware | code in place; stalls under mGBA 0.10.5, untested on hardware |

### Two players on one PC

1. Open `build\log2-coop.gba` in mGBA (the Qt `mGBA.exe`).
2. Tools > Scripting, type `dofile("F:/MeltyCreations/log2/tools/mgba/coop.lua")` and Run. That
   loads the player-2 bridge plus the console helpers `peek()`, `ents()` and `p2_force` (mGBA 0.10.5
   has no command-line script option, so this step is manual).
3. Run `python tools\p2_keys.py` in a terminal. Player 1 uses mGBA's normal keys; player 2 uses
   IJKL to move, O = A, U = B, [ ] = L R, N = Select, M = Start. `p2_keys.py` reads the keyboard
   globally (`GetAsyncKeyState`), not a gamepad: change player 2's keys in its `KEYMAP`. A gamepad can
   be player 1's, bound in mGBA.

Keep an mGBA savestate (`Shift+F1`) on a map; `mGBA.exe -t build\log2-coop.ss1 build\log2-coop.gba`
starts straight from it. Make savestates with the current build: player 2's vtable copies in EWRAM
hold code addresses, so a state from an older build crashes (`Jumped to invalid address`) as soon as
player 2 acts.

### Playing online (lockstep netplay)

Both players run the same `build\log2-coop.gba` in their own mGBA, and `tools/mgba/netplay.lua`
keeps the two emulations identical: every frame each side sends its pad word and waits for the
other's before the frame runs. The host is player 1 on mGBA's normal keys, the guest is player 2
on *its* mGBA's normal keys. No screen share: each player watches their own window. A tiny Python
relay pairs the two connections, so only the relay's machine needs a forwarded port.

1. **Relay** (on the host's PC, or any machine both can reach):
   `python tools\netplay_relay.py` listens on TCP 7778. Forward that port on the router of
   whoever runs it (or run it on a VPS); the players connect outbound only.
2. **Host**: open the ROM in mGBA, get to where you want to start (fresh boot, or load a savestate
   made with this ROM build), then in Tools > Scripting run
   `NETPLAY = { role = "host", relay = "127.0.0.1", port = 7778, delay = 2 }  dofile("F:/MeltyCreations/log2/tools/mgba/netplay.lua")`
   (relay = the relay's address as seen from the host).
3. **Guest**: open the ROM in mGBA (any state; it is replaced) and run
   `NETPLAY = { role = "guest", relay = "<relay ip>", port = 7778 }  dofile("<path>/tools/mgba/netplay.lua")`.
   The console on both sides should show "peer connected", the snapshot transfer (~400 KB) and
   "lockstep running"; from then on both windows show the same game and player 2 appears.
4. Every second both sides compare a hash of the game state; a mismatch is logged as **DESYNC**
   and the host re-sends its snapshot. `netplay_status()` prints frame, stalls and checks;
   `netplay_stop()` leaves netplay (the game continues locally as single player).

`delay` is the input lag in frames, the same for both players: inputs pressed at frame N apply at
frame N+delay, so the game never waits as long as one-way latency stays under (delay-1) frames
(16.7 ms each). Use 2 on a LAN, 3-4 across the internet; the guest adopts the host's value.
The host's snapshot includes its save data, so the guest's `.sav` is overwritten by the host's.
Do not load `p2_input.lua`/`coop.lua` in the same mGBA as `netplay.lua`: both write player 2's pad.
Both sides need a ROM from the same `make` flags (the handshake compares the ROM's CRC).

### Wide view on big screens (WSL fork)

`make WIDE=1` builds `build\log2-wide.gba`, which widens the view up to 720 x 480 as the two
players separate and shrinks it back when they rejoin. It needs the mGBA fork in `tools/mgba-wide`,
which Windows Smart App Control blocks when built natively, so it is built and run in WSL Ubuntu:

```
wsl -d Ubuntu -u root -- bash /mnt/f/MeltyCreations/log2/tools/mgba-wide/build-wsl.sh
wsl -d Ubuntu -u root -- bash -c "cd /root/mgba-build/sdl && MGBA_P2=1 MGBA_P2_PORT=7778 ./mgba -2 /mnt/f/MeltyCreations/log2/build/log2-wide.gba"
python tools\p2_keys.py --port 7778
```

The window appears on the Windows desktop (WSLg). See `docs/widescreen.md` for the design and
limits (the Lua helpers and netplay script do not run in the fork).

## Layout

```
src/coop/   the C blob: co-op, bosses, story hooks, party, duels, waves, wide view (coop.ld pins its RAM)
tools/      build steps (patch_rom.py, romdata.py, mapbuild.py, script_asm.py, sprite generators),
            ROM readers and dumpers, mgba/ (Lua helpers), fork/ (WSL fork helpers), headless/
            (headless test runner), mgba-wide/ (the fork's patches), ghidra/, gdb/
data/       our story and data edits: hercule.json, story/ (chapter scripts), maps/ (new maps)
docs/       design notes and reverse-engineering findings; docs/INDEX.md is the map
roms/       your ROM dumps (ignored)
build/      build output, decompile corpus, test states (ignored)
```
