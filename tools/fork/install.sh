#!/bin/bash
# Copy the fork helpers (tools/fork/*.sh, *.gdb) into /root with CRs stripped and make them executable.
# usage (from Windows): wsl -d Ubuntu -u root -- bash -c "tr -d '\r' < /mnt/f/MeltyCreations/log2/tools/fork/install.sh | bash"
D=/mnt/f/MeltyCreations/log2/tools/fork
for f in "$D"/*.sh "$D"/*.gdb; do
  tr -d '\r' < "$f" > "/root/$(basename "$f")"
done
chmod +x /root/*.sh
echo "installed fork helpers"
