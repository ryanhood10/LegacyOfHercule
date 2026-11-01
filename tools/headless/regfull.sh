#!/bin/bash
# The full headless regression (docs/testing.md "Full regression"): every chapter and tournament script on a
# DEBUG_MENU=1 build in both modes (co-op as written; arctic_summit, postgame and ch9_gero again with player 2 off),
# then the prologue on the play ROM. Pass: no TIMEOUT / CRASH / FAIL line in the log.
#   make BUILD=build/<dir> DEBUG_MENU=1 && make
#   python tools/headless/st_test.py tools/headless/mkmenu.txt build/<dir>/log2-coop.gba --out build/<dir>/hl
#   bash tools/headless/regfull.sh build/<dir>        (default build/tny; about 15 minutes)
#   PLAY=build/<dir>/play/log2-coop.gba bash ...     the play ROM for the prologue (default build/log2-coop.gba)
#   grep -nE "TIMEOUT|CRASH|FAIL" build/<dir>/hl/reg.log
cd "$(dirname "$0")/../.." || exit 1
DIR=${1:-build/tny}
ROM=$DIR/log2-coop.gba; PLAY=${PLAY:-build/log2-coop.gba}; OUT=$DIR/hl; LOG=$OUT/reg.log; : > "$LOG"
run() { echo "## $1" >> "$LOG"; python tools/headless/st_test.py "$2" "$ROM" "$OUT/menu.ss" --out "$OUT" --cols 6 2>&1 | grep -E "^==|^map zone|^0[0-9a-f]{7}:|entrec|reached|stepnz|FAIL|rror|TIMEOUT|CRASH" >> "$LOG"; }
for t in a16_moves tny_eggs tny_crowd tny_npcs tny_polish tny_engine tny_ui tny_moves tny_rumble tny_play tny_artnew \
         tny_artbuu tny_grounds cell_games cell_games_1p krillin_disc arctic_summit postgame ch9_gero p2_special revive difficulty \
         journal dropout ch1_room_forest scene_actor exp_credit a16_gone broly_ride duel_end tny_hpbars tny_fightfix \
         tny_nomirror tny_talk tny_combo_draw tny_combo_meter tny_combo_run tny_combo_tune wm_landmark tny_pp_combo \
         tny_caption tny_annwalk tny_turn tny_walk gohan_ssj1 cell_juniors ch9_actors ch1_notes player_lines lead_scenes; do
  run $t tools/headless/$t.txt
done
for t in arctic_summit postgame ch9_gero; do
  sed -e 's/^p2 0$/p2 off/' -e '/^p2 [^o]/d' tools/headless/$t.txt > "$OUT/${t}_1p.txt"
  run ${t}_1p "$OUT/${t}_1p.txt"
done
# Super Saiyan Hercule (docs/testing.md "Super Saiyan Hercule"): self-checking, st_expect.py prints FAIL on a mismatch
echo "## ssj_hercule" >> "$LOG"
python tools/headless/st_expect.py tools/headless/ssj_hercule.txt "$ROM" "$OUT/menu.ss" --out "$OUT" --cols 6 2>&1   | grep -E "^==|^map zone|^0[0-9a-f]{7}:|FAIL|rror|TIMEOUT|CRASH" >> "$LOG"
echo "## prologue (play ROM)" >> "$LOG"
python tools/headless/st_test.py tools/headless/prologue_companion.txt "$PLAY" --out "$DIR/play" 2>&1 | grep -E "^==|^0[0-9a-f]{7}:|stepnz|rror" >> "$LOG"
echo done >> "$LOG"
grep -c "^## " "$LOG"
if grep -nE "TIMEOUT|CRASH|FAIL" "$LOG"; then exit 1; fi   # a failure: exit 1; clean: exit 0
