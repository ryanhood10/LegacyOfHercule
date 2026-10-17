"""Send player 2's buttons to the mGBA script (tools/mgba/p2_input.lua) over TCP.

Reads the keyboard globally (works while mGBA has focus), so two people can share one keyboard,
or run it on another PC with --host <ip> to play remotely.

Default keys:  I J K L = up left down right   O = A   U = B   [ ] = L R   N = Select   M = Start
usage: p2_keys.py [--host 127.0.0.1] [--port 7777]
"""
import argparse
import ctypes
import socket
import struct
import sys
import time

GetAsyncKeyState = ctypes.windll.user32.GetAsyncKeyState

# GBA bit -> virtual-key code
KEYMAP = {
    0: 0x4F,  # A      -> O
    1: 0x55,  # B      -> U
    2: 0x4E,  # Select -> N
    3: 0x4D,  # Start  -> M
    4: 0x4C,  # Right  -> L
    5: 0x4A,  # Left   -> J
    6: 0x49,  # Up     -> I
    7: 0x4B,  # Down   -> K
    8: 0xDD,  # R      -> ]
    9: 0xDB,  # L      -> [
}


def read_word():
    word = 0
    for bit, vk in KEYMAP.items():
        if GetAsyncKeyState(vk) & 0x8000:
            word |= 1 << bit
    return word


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=7777)
    a = ap.parse_args()
    while True:
        try:
            with socket.create_connection((a.host, a.port), timeout=5) as s:
                s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                print(f"connected to {a.host}:{a.port}; IJKL move, O=A, U=B, [ ]=L R, N=Select, M=Start")
                last, ticks = None, 0
                while True:
                    word = read_word()
                    ticks += 1
                    if word != last or ticks % 6 == 0:   # on change, and a 20 Hz heartbeat
                        s.sendall(struct.pack("<H", word))
                        last = word
                    time.sleep(1 / 120)
        except (ConnectionRefusedError, socket.timeout, ConnectionResetError, OSError) as e:
            print(f"not connected ({e}); retrying...", file=sys.stderr)
            time.sleep(1)


if __name__ == "__main__":
    main()
