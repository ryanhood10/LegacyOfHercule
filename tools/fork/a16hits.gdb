# Log every hit a player takes (player vtable slot 14 = 0x08008807, r0 = player, r1 = message + 0x24):
# damage, the message's multiplier and attack descriptor, the attacker. source /root/a16hits.gdb
break *0x08008806
commands
  silent
  printf "HIT f=%d def=%08x dmg=%d mult=%x atk=%08x by=%08x\n", *(int*)0x03000E20, $r0, *(int*)($r1 + 8), *(int*)($r1 - 0xC), *(unsigned*)($r1 - 8), *(unsigned*)($r1 - 4)
  continue
end
# hits Android 16 takes (enemy OnHit 0x0800E485 on the boss): damage
break *0x0800E484 if $r0 == *(unsigned*)0x0203FD34
commands
  silent
  printf "BOSS f=%d dmg=%d hp=%d\n", *(int*)0x03000E20, *(int*)($r1 + 8), *(int*)($r0 + 0x188)
  continue
end
