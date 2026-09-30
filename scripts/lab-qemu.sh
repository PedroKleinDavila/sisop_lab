#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="$ROOT/.lab/sdb.bin"
RESULTS="$ROOT/.lab/results.ext2"
[[ -f "$ROOT/output-lab/images/bzImage" ]] || { echo "Kernel não compilado." >&2; exit 1; }
[[ -f "$ROOT/output-lab/images/rootfs.ext2" ]] || { echo "Rootfs não compilado." >&2; exit 1; }
mkdir -p "$ROOT/.lab"
[[ -f "$IMAGE" ]] || truncate -s 1G "$IMAGE"
if [[ ! -f "$RESULTS" ]]; then
    truncate -s 128M "$RESULTS"
    mkfs.ext2 -F -q "$RESULTS"
fi
exec qemu-system-i386 -accel tcg -M pc -m 512M -nographic \
    -kernel "$ROOT/output-lab/images/bzImage" \
    -drive "file=$ROOT/output-lab/images/rootfs.ext2,if=ide,index=0,format=raw" \
    -drive "file=$IMAGE,if=ide,index=1,format=raw" \
    -drive "file=$RESULTS,if=ide,index=2,format=raw" \
    -append 'console=ttyS0 root=/dev/sda rootwait'
