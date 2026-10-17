"""Print GBA header info and hashes for one or more ROMs, and validate the header checksum."""
import hashlib
import sys
import zlib


def check(path):
    data = open(path, "rb").read()
    title = data[0xA0:0xAC].rstrip(b"\0").decode("ascii", "replace")
    code = data[0xAC:0xB0].decode("ascii", "replace")
    maker = data[0xB0:0xB2].decode("ascii", "replace")
    version = data[0xBC]
    stored = data[0xBD]
    calc = (-(sum(data[0xA0:0xBD]) + 0x19)) & 0xFF

    print(f"{path}")
    print(f"  size      {len(data)} bytes ({len(data) // 1048576} MiB)")
    print(f"  title     {title!r}")
    print(f"  code      {code}  maker {maker}  version {version}")
    print(f"  header    checksum {'OK' if stored == calc else f'BAD (stored {stored:02X}, calc {calc:02X})'}")
    print(f"  crc32     {zlib.crc32(data):08x}")
    print(f"  sha1      {hashlib.sha1(data).hexdigest()}")
    print(f"  sha256    {hashlib.sha256(data).hexdigest()}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("usage: romcheck.py ROM [ROM ...]")
    for p in sys.argv[1:]:
        check(p)
        print()
