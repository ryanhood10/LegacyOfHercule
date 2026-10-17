"""Pair up two tools/mgba/netplay.lua clients and pipe bytes between them.

Run it on the host's PC (forward the port) or on any machine both players can reach:
    python tools/netplay_relay.py [--port 7778]
The first two connections form a session; when either side leaves, the other is dropped and
the relay waits for the next pair. Both directions use TCP_NODELAY (one small packet per frame).
"""
import argparse
import socket
import sys
import threading
import time


def forward(src, dst, counter):
    """Blocking copy src -> dst until src closes or either side fails."""
    try:
        while True:
            data = src.recv(65536)
            if not data:
                break
            counter[0] += len(data)
            dst.sendall(data)
    except OSError:
        pass
    finally:
        for s in (src, dst):
            try:
                s.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass


def session(a, b):
    """Shuttle data both ways; returns (bytes a->b, bytes b->a, seconds)."""
    t0 = time.time()
    ab, ba = [0], [0]
    t1 = threading.Thread(target=forward, args=(a, b, ab), daemon=True)
    t2 = threading.Thread(target=forward, args=(b, a, ba), daemon=True)
    t1.start()
    t2.start()
    t1.join()
    t2.join()
    return ab[0], ba[0], time.time() - t0


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--port", type=int, default=7778)
    ap.add_argument("--bind", default="0.0.0.0")
    a = ap.parse_args()
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((a.bind, a.port))
    srv.listen(2)
    print(f"relay listening on {a.bind}:{a.port}; waiting for two players", flush=True)
    while True:
        first, addr1 = srv.accept()
        first.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        print(f"player from {addr1[0]}:{addr1[1]} waiting for a partner", flush=True)
        second, addr2 = srv.accept()
        second.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        print(f"player from {addr2[0]}:{addr2[1]}; session started", flush=True)
        ab, ba, secs = session(first, second)
        for s in (first, second):
            try:
                s.close()
            except OSError:
                pass
        print(f"session over after {secs:.0f} s ({ab} / {ba} bytes); waiting again", flush=True)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(0)
