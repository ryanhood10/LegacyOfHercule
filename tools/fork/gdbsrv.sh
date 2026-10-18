#!/bin/bash
# Persistent GDB client for the mGBA fork's stub: commands are appended to the FIFO, output lands in
# /root/gdb.log. usage: gdbsrv.sh start | cmd "<gdb command>" | pause | log [n]
case "$1" in
  start)
    pkill -x gdb-multiarch 2>/dev/null; sleep 0.3
    rm -f /tmp/gdbin; mkfifo /tmp/gdbin
    setsid nohup bash -c "exec sleep 1000000 > /tmp/gdbin" > /dev/null 2>&1 &
    : > /root/gdb.log
    setsid nohup gdb-multiarch -q -nx < /tmp/gdbin >> /root/gdb.log 2>&1 &
    sleep 0.5
    printf 'set pagination off\nset confirm off\nset architecture armv4t\nset arm force-mode thumb\nset remotetimeout 20\ntarget remote 127.0.0.1:2345\n' > /tmp/gdbin
    sleep 2; tail -3 /root/gdb.log ;;
  cmd) printf '%s\n' "$2" > /tmp/gdbin ;;
  pause) pkill -INT -x gdb-multiarch ;;
  log) tail -n "${2:-20}" /root/gdb.log ;;
esac
