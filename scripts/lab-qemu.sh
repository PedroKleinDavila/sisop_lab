#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="$ROOT/.lab/sdb.bin"
[[ -f "$ROOT/output-lab/images/bzImage" ]] || { echo "Kernel não compilado." >&2; exit 1; }
[[ -f "$ROOT/output-lab/images/rootfs.ext2" ]] || { echo "Rootfs não compilado." >&2; exit 1; }
mkdir -p "$ROOT/.lab"
[[ -f "$IMAGE" ]] || truncate -s 1G "$IMAGE"
exec qemu-system-i386 -accel tcg -M pc -m 512M -nographic +    -kernel "$ROOT/output-lab/images/bzImage" +    -drive "file=$ROOT/output-lab/images/rootfs.ext2,if=ide,index=0,format=raw" +    -drive "file=$IMAGE,if=ide,index=1,format=raw" +    -append 'console=ttyS0 root=/dev/sda rootwait'
