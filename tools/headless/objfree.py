"""Parse 'peek8 0x03001120 8' + 'peek 0x030017B4 330' blocks of a headless run: OBJ VRAM free list
(entity list + 0x694, 10-byte nodes {+4 u16 size in 64-byte units, +8 u8 next}, head = list byte 0)."""
import re, struct, sys
lines = open(sys.argv[1]).read().splitlines()
i = 0
while i < len(lines):
    if lines[i].startswith("03001120:"):
        head = int(lines[i].split()[1], 16)
        words = [int(x, 16) for x in lines[i + 1].split()[1:]]
        blob = b"".join(struct.pack("<I", w) for w in words)
        n, sizes, k = head, [], 0
        while n != 0xFF and k < 130:
            node = blob[n * 10:n * 10 + 10]
            sizes.append(struct.unpack_from("<H", node, 4)[0] * 64)
            n = node[8]
            k += 1
        print(f"free blocks {len(sizes)}: largest {max(sizes) if sizes else 0} bytes, total {sum(sizes)} bytes")
        i += 2
    else:
        i += 1
