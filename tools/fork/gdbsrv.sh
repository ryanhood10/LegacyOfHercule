#!/bin/bash
# Persistent GDB client for the mGBA fork's stub: commands are appended to a FIFO, output lands in
# the slot's log (/root/gdb.log for slot 0). usage: [FORK_SLOT=n] gdbsrv.sh start | cmd "<gdb command>" | pause | log [n] | stop
. /root/slot.sh
case "$1" in
  start)
    [ -f "$GDB_PID" ] && kill "$(cat "$GDB_PID")" 2>/dev/null; sleep 0.3
    rm -f "$GDB_FIFO"; mkfifo "$GDB_FIFO"
    setsid nohup bash -c "exec sleep 1000000 > $GDB_FIFO" > /dev/null 2>&1 &
    : > "$GDB_LOG"
    setsid nohup gdb-multiarch -q -nx < "$GDB_FIFO" >> "$GDB_LOG" 2>&1 &
    echo $! > "$GDB_PID"
    sleep 0.5
    printf 'set pagination off\nset confirm off\nset architecture armv4t\nset arm force-mode thumb\nset remotetimeout 20\ntarget remote 127.0.0.1:%s\n' "$GDB_PORT" > "$GDB_FIFO"
    sleep 2; tail -3 "$GDB_LOG" ;;
  cmd) printf '%s\n' "$2" > "$GDB_FIFO" ;;
  pause) [ -f "$GDB_PID" ] && kill -INT "$(cat "$GDB_PID")" ;;
  log) tail -n "${2:-20}" "$GDB_LOG" ;;
  stop) [ -f "$GDB_PID" ] && kill "$(cat "$GDB_PID")" 2>/dev/null; rm -f "$GDB_PID" ;;
esac
