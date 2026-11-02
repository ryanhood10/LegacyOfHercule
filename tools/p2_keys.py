"""Send player 2's buttons to the mGBA script (tools/mgba/p2_input.lua) over TCP.

Reads the keyboard globally (works while mGBA has focus), so two people can share one keyboard,
or run it on another PC with --host <ip> to play remotely. Also reads one XInput controller (an Xbox
pad, or a remote player's pad through Parsec, which appears on the host as a virtual Xbox 360 pad):
the first controller to press a button after the start becomes player 2's (--pad N picks one by
number, --pad none turns controllers off). Player 1's own controller is the one chosen in mGBA's
Settings > Controllers; mGBA reads only that one, so the two never mix.

Default keys:  I J K L = up left down right   O = A   U = B   [ ] = L R   N = Select   M = Start
Controller:    d-pad or left stick = move   B / Y = A   A / X = B   LB / LT = L   RB / RT = R
               Start = Start   Back (View) = Select   (--swap-ab: A / X = A, B / Y = B)
usage: p2_keys.py [--host 127.0.0.1] [--port 7777] [--pad auto|none|0-3] [--swap-ab]
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


class XINPUT_GAMEPAD(ctypes.Structure):
    _fields_ = [("wButtons", ctypes.c_ushort), ("bLeftTrigger", ctypes.c_ubyte),
                ("bRightTrigger", ctypes.c_ubyte), ("sThumbLX", ctypes.c_short),
                ("sThumbLY", ctypes.c_short), ("sThumbRX", ctypes.c_short), ("sThumbRY", ctypes.c_short)]


class XINPUT_STATE(ctypes.Structure):
    _fields_ = [("dwPacketNumber", ctypes.c_uint), ("Gamepad", XINPUT_GAMEPAD)]


def load_xinput():
    for name in ("xinput1_4", "xinput1_3", "xinput9_1_0"):
        try:
            return getattr(ctypes.windll, name).XInputGetState
        except OSError:
            pass
    return None


XInputGetState = load_xinput()
STICK = 12000      # left-stick dead zone (of 32767)
TRIGGER = 100      # trigger threshold (of 255)

# XInput button mask -> GBA bit (A and B follow the GBA's layout: right face button = A)
PADMAP = {
    0x0001: 6,  # d-pad up
    0x0002: 7,  # d-pad down
    0x0004: 5,  # d-pad left
    0x0008: 4,  # d-pad right
    0x0010: 3,  # Start
    0x0020: 2,  # Back / View -> Select
    0x0100: 9,  # LB -> L
    0x0200: 8,  # RB -> R
    0x2000: 0,  # B -> A
    0x8000: 0,  # Y -> A
    0x1000: 1,  # A -> B
    0x4000: 1,  # X -> B
}


def pad_state(i):
    """The XINPUT_GAMEPAD of controller i, or None when it is not connected."""
    if XInputGetState is None:
        return None
    st = XINPUT_STATE()
    if XInputGetState(i, ctypes.byref(st)) != 0:
        return None
    return st.Gamepad


def pad_word(g, swap_ab):
    word = 0
    for mask, bit in PADMAP.items():
        if g.wButtons & mask:
            word |= 1 << bit
    if swap_ab and word & 3 in (1, 2):
        word ^= 3
    if g.sThumbLX > STICK: word |= 1 << 4
    if g.sThumbLX < -STICK: word |= 1 << 5
    if g.sThumbLY > STICK: word |= 1 << 6
    if g.sThumbLY < -STICK: word |= 1 << 7
    if g.bLeftTrigger > TRIGGER: word |= 1 << 9
    if g.bRightTrigger > TRIGGER: word |= 1 << 8
    return word


def pick_pad():
    """auto: the first connected controller with a button held (checked every poll until one is)."""
    for i in range(4):
        g = pad_state(i)
        if g is not None and (g.wButtons or g.bLeftTrigger > TRIGGER or g.bRightTrigger > TRIGGER):
            return i
    return None


def read_word(pad, swap_ab):
    word = 0
    for bit, vk in KEYMAP.items():
        if GetAsyncKeyState(vk) & 0x8000:
            word |= 1 << bit
    if pad is not None:
        g = pad_state(pad)
        if g is not None:
            word |= pad_word(g, swap_ab)
    return word


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=7777)
    ap.add_argument("--pad", default="auto", help="auto (first controller to press a button), none, or 0-3")
    ap.add_argument("--swap-ab", action="store_true", help="controller A / X = GBA A, B / Y = GBA B")
    a = ap.parse_args()
    pad = None if a.pad in ("auto", "none") else int(a.pad)
    auto = a.pad == "auto"
    if XInputGetState is None:
        print("no XInput on this PC: keyboard only", file=sys.stderr)
        auto = False
    else:
        connected = [i for i in range(4) if pad_state(i) is not None]
        print(f"controllers connected: {', '.join(str(i) for i in connected) or 'none yet'}")
        if auto:
            print("player 2: press any button on your controller to take it (keyboard works meanwhile)")
    while True:
        try:
            with socket.create_connection((a.host, a.port), timeout=5) as s:
                s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                print(f"connected to {a.host}:{a.port}; IJKL move, O=A, U=B, [ ]=L R, N=Select, M=Start")
                last, ticks = None, 0
                while True:
                    if auto and pad is None:
                        pad = pick_pad()
                        if pad is not None:
                            print(f"player 2's controller: number {pad}")
                    word = read_word(pad, a.swap_ab)
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
