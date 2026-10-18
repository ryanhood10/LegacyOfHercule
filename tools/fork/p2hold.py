"""Scripted player-2 pad for the WSL fork (p2pad.c protocol: 2-byte little-endian GBA key words,
active-high, A=1 B=2 Select=4 Start=8 Right=0x10 Left=0x20 Up=0x40 Down=0x80 R=0x100 L=0x200).
Plays MASK:SECONDS steps, then keeps player 2 present by sending 0 every 100 ms until killed; a file
named by --cmd (default build/p2cmd.txt) is polled: write "MASK:SECONDS ..." into it to play more steps.
usage: python tools/fork/p2hold.py [--port 7778] [--cmd FILE] [MASK:SECONDS ...]"""
import os
import socket
import struct
import sys
import time

args = sys.argv[1:]
port = 7778
cmd = "build/p2cmd.txt"
if "--port" in args:
    i = args.index("--port"); port = int(args[i + 1]); del args[i:i + 2]
if "--cmd" in args:
    i = args.index("--cmd"); cmd = args[i + 1]; del args[i:i + 2]


def play(sock, steps):
    for st in steps:
        mask, secs = st.split(":")
        mask = int(mask, 0)
        end = time.time() + float(secs)
        while time.time() < end:
            sock.sendall(struct.pack("<H", mask))
            time.sleep(0.05)


while True:
    try:
        s = socket.create_connection(("127.0.0.1", port), timeout=5)
        print("connected", flush=True)
        play(s, args)
        args = []
        while True:
            if os.path.exists(cmd):
                steps = open(cmd).read().split()
                os.remove(cmd)
                play(s, steps)
            s.sendall(struct.pack("<H", 0))
            time.sleep(0.1)
    except OSError as e:
        print("retry", e, flush=True)
        time.sleep(1)
