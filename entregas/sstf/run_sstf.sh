#!/bin/sh
set -eu

size="$(cat /sys/block/sdb/size 2>/dev/null || true)"
[ "$size" = 2097152 ] || {
    echo "Disco de testes /dev/sdb ausente ou com tamanho incorreto: $size" >&2
    exit 1
}
[ -b /dev/sdb ] || { echo "/dev/sdb não é dispositivo de bloco" >&2; exit 1; }
mkdir -p /mnt/results
grep -q ' /mnt/results ' /proc/mounts || mount /dev/sdc /mnt/results
modprobe sstf_iosched
grep -q 'sstf' /sys/block/sdb/queue/scheduler || {
    echo "Escalonador sstf indisponível para /dev/sdb" >&2
    exit 1
}
echo sstf > /sys/block/sdb/queue/scheduler
cat /sys/block/sdb/queue/scheduler
echo 2 > /sys/block/sdb/queue/nomerges
echo 0 > /sys/block/sdb/queue/read_ahead_kb
sync
echo 3 > /proc/sys/vm/drop_caches
dmesg -c >/dev/null
sector_read "${1:-16}" "${2:-16}"
dmesg | grep '\[SSTF\]' > /mnt/results/sstf.log
sync
echo "Log salvo em /mnt/results/sstf.log"
