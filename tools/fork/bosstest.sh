#!/bin/bash
# Boss-pair test driver for a fork slot (FORK_SLOT=n; uses /root/gdbsrv.sh, so launch with runfork.ps1).
# usage: bosstest.sh spawn IDX            Map_SpawnScript(IDX) like TriggerMapEvent (the boss record)
#        bosstest.sh rec ADDR             spawn enemy record ADDR (any map's) 48 px right of player 1 + boss bar
#        bosstest.sh peek [TAG]           pair 0: hp/max, position, sprite id, stat index of both bosses
#        bosstest.sh hp orig|twin VALUE   set a boss's HP
#        bosstest.sh near orig|twin       teleport player 1 to 20 px left of that boss
#        bosstest.sh p2near orig|twin     teleport player 2 to 20 px right of that boss
#        bosstest.sh god                  every party entry's HP to 30000/30000
#        bosstest.sh partner SPRITE       coop2.partner_override (0 = the boss rules decide)
#        bosstest.sh off BITS             coop2.off (1 damage, 2 twins, 4 speed, 8 HP, 16 partner models, 32 both-down)
# Offsets: coop2 0x0203FB00 (twins +0x124, off +0x1EC, cmd +0x208, arg +0x20C, partner +0x22C, downed
# +0x250); a co-op enemy block = entity (0x1F0) + stat copy (0x18, max HP at +4) + record copy
# (stat index +8, sprite +0x10).
. "$(dirname "$0")/slot.sh"
G="$(dirname "$0")/gdbsrv.sh"
C=0x0203FB00
sel() { [ "$1" = twin ] && echo 0x128 || echo 0x124; }
peek() {
  $G pause; sleep 0.6
  for k in orig twin; do
    o=$(sel $k)
    $G cmd "set \$e = *(unsigned*)($C+$o)"
    $G cmd "if \$e
printf \"PEEK $1 $k e=%x hp=%d/%d x=%d y=%d sprite=%d stat=%d act=%x anim=%x\\n\", \$e, *(int*)(\$e+0x188), *(int*)(\$e+0x1F4), *(int*)(\$e+0x158)>>8, *(int*)(\$e+0x15C)>>8, *(int*)(\$e+0x218), *(int*)(\$e+0x210), *(unsigned*)(\$e+0x184), *(unsigned*)(\$e+0x4C)
else
printf \"PEEK $1 $k none\\n\"
end"
  done
  $G cmd "printf \"PEEK $1 downed=%d spawned=%d\\n\", *(unsigned short*)($C+0x250), 0"
  sleep 0.8; $G cmd continue; sleep 0.3
  grep "PEEK $1 " "$GDB_LOG" | tail -3
}
case "$1" in
spawn)
  $G pause; sleep 0.6
  $G cmd "set *(unsigned*)($C+0x20C) = $2"
  $G cmd "set *(unsigned*)($C+0x208) = 1"
  sleep 0.3; $G cmd continue; sleep 3
  peek spawn ;;
rec)
  $G pause; sleep 0.6
  $G cmd "set *(unsigned*)($C+0x20C) = $2"
  $G cmd "set *(unsigned*)($C+0x208) = 8"
  sleep 0.3; $G cmd continue; sleep 3
  peek rec ;;
peek) peek "${2:-now}" ;;
hp)
  $G pause; sleep 0.6
  $G cmd "set *(int*)(*(unsigned*)($C+$(sel $2))+0x188) = $3"
  sleep 0.3; $G cmd continue ;;
near|p2near)
  $G pause; sleep 0.6
  $G cmd "set \$e = *(unsigned*)($C+$(sel $2))"
  if [ "$1" = near ]; then P='*(unsigned*)0x03001FA8'; D='- (20<<8)'; else P='*(unsigned*)(0x0203FF04+4)'; D='+ (20<<8)'; fi
  $G cmd "set *(int*)($P+0x158) = *(int*)(\$e+0x158) $D"
  $G cmd "set *(int*)($P+0x15C) = *(int*)(\$e+0x15C)"
  sleep 0.3; $G cmd continue ;;
god)
  $G pause; sleep 0.6
  for i in 0 1 2 3 4 5 6 7; do
    a=$((0x0203F00C + i * 0x1C))
    $G cmd "set *(unsigned short*)$a = 30000"
    $G cmd "set *(unsigned short*)($a+2) = 30000"
  done
  sleep 0.3; $G cmd continue ;;
partner)
  $G pause; sleep 0.6
  $G cmd "set *(unsigned*)($C+0x22C) = $2"
  sleep 0.3; $G cmd continue ;;
off)
  $G pause; sleep 0.6
  $G cmd "set *(unsigned*)($C+0x1EC) = $2"
  sleep 0.3; $G cmd continue ;;
esac
