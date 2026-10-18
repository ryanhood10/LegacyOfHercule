"""Pad sender for STORY_TEST builds in the WSL fork: the fork's TCP pad (p2pad.c, port 7778 + slot)
drives player 1 there. Keeps one connection open and plays steps from a command file, so test scripts
can press buttons without the debugger. Masks: A=1 B=2 Select=4 Start=8 Right=0x10 Left=0x20 Up=0x40
Down=0x80 R=0x100 L=0x200.
usage: python tools/fork/st_pad.py [--port 7779] [--cmd build/st_pad.txt]      (runs until killed)
       python tools/fork/st_pad.py --send "1:0.2 0:1 0x80:0.3" [--cmd build/st_pad.txt]
            append steps (MASK:SECONDS) for the running sender and wait until it has played them"""
import os
import socket
import struct
import sys
import time

args = sys.argv[1:]
port, cmd = 7779, "build/st_pad.txt"
if "--port" in args:
    i = args.index("--port"); port = int(args[i + 1]); del args[i:i + 2]
if "--cmd" in args:
    i = args.index("--cmd"); cmd = args[i + 1]; del args[i:i + 2]

if "--send" in args:
    steps = args[args.index("--send") + 1]
    with open(cmd + ".tmp", "w") as f:
        f.write(steps)
    while os.path.exists(cmd):          # the previous batch is still queued
        time.sleep(0.05)
    os.replace(cmd + ".tmp", cmd)
    total = sum(float(s.split(":")[1]) for s in steps.split())
    while os.path.exists(cmd):
        time.sleep(0.05)
    time.sleep(total + 0.1)
    sys.exit(0)


def play(sock, steps):
    for st in steps:
        mask, secs = st.split(":")
        mask = int(mask, 0)
        end = time.time() + float(secs)
        while time.time() < end:
            sock.sendall(struct.pack("<H", mask))
            time.sleep(0.03)


while True:
    try:
        s = socket.create_connection(("127.0.0.1", port), timeout=5)
        print("connected", flush=True)
        while True:
            if os.path.exists(cmd):
                steps = open(cmd).read().split()
                os.remove(cmd)
                play(s, steps)
            s.sendall(struct.pack("<H", 0))
            time.sleep(0.05)
    except OSError as e:
        print("retry", e, flush=True)
        time.sleep(1)
