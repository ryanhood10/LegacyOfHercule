"""Send Lua lines to the mGBA command server (tools/mgba/cmd_server.lua) and print what they logged.

usage: python tools/mgba_cmd.py 'ents()' ['seq("DOWN:5 WAIT:30")' ...]
       python tools/mgba_cmd.py --wait            (block until the pad queues are empty)
       python tools/mgba_cmd.py --port=7781 ...   (a second mGBA instance whose server was loaded
                                                   with cmd_server_port = 7781)
Each argument is one line; a line's output arrives after the frame in which it ran.
"""
import socket
import sys
import time


def send(sock, line, timeout=30.0):
    sock.sendall((line + "\n").encode())
    sock.settimeout(timeout)
    buf = b""
    while b"\n<END>\n" not in buf:
        chunk = sock.recv(4096)
        if not chunk:
            raise SystemExit("server closed")
        buf += chunk
    out, _ = buf.split(b"\n<END>\n", 1)
    return out.decode(errors="replace")


def main():
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    port = 7779
    for a in list(args):
        if a.startswith("--port="):
            port = int(a[7:])
            args.remove(a)
    sock = socket.create_connection(("127.0.0.1", port), timeout=5)
    for line in args:
        if line == "--wait":
            while True:
                left = send(sock, "return busy()")
                if int(left.strip() or 0) <= 0:
                    break
                time.sleep(min(int(left) / 60.0, 1.0))
            continue
        if line.startswith("--sleep="):
            time.sleep(float(line[8:]))
            continue
        out = send(sock, line)
        if out:
            print(out)
    sock.close()


if __name__ == "__main__":
    main()
