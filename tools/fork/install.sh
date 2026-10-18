#!/bin/bash
# Copy the fork helpers (tools/fork/*.sh, *.gdb) into /root with CRs stripped and make them executable.
# usage (from Windows): wsl -d Ubuntu -u root -- bash -c "tr -d '\r' < /mnt/f/MeltyCreations/log2/tools/fork/install.sh | bash"
# FORK_TOOLS = the tools/fork directory to copy from (default: the main checkout; runfork.ps1 passes its own)
# FORK_DEST = where to install (default /root; runfork.ps1 gives slots >= 1 their own /root/fork-sN, so a
# session reinstalling the shared /root copies never swaps the helpers under another slot's scripts)
D=${FORK_TOOLS:-/mnt/f/MeltyCreations/log2/tools/fork}
O=${FORK_DEST:-/root}
mkdir -p "$O"
for f in "$D"/*.sh "$D"/*.gdb; do
  tr -d '\r' < "$f" > "$O/$(basename "$f")"
done
chmod +x "$O"/*.sh
echo "installed fork helpers"
