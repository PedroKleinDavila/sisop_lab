#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
"$ROOT/scripts/lab-setup.sh"
cd "$ROOT"
MAKE_ARGS=(O="$ROOT/output-lab" BR2_EXTERNAL="$ROOT/lab-external")
make "${MAKE_ARGS[@]}" lab_x86_defconfig
available_kib="$(awk '/MemAvailable:/ {print $2}' /proc/meminfo)"
jobs=$((available_kib / (2 * 1024 * 1024)))
((jobs >= 1)) || jobs=1
((jobs <= $(nproc))) || jobs="$(nproc)"
((jobs <= 4)) || jobs=4
echo "Compilando com $jobs job(s) por pacote."
if [[ -f "$ROOT/output-lab/build/linux-custom/.stamp_built" ]]; then
    for package in lab-syscalls lab-xtea lab-sstf; do
        symbol="BR2_PACKAGE_${package^^}"
        symbol="${symbol//-/_}"
        if grep -q "^${symbol}=y" "$ROOT/output-lab/.config"; then
            make "${MAKE_ARGS[@]}" "$package-dirclean"
        fi
    done
    make "${MAKE_ARGS[@]}" BR2_JLEVEL="$jobs" linux-rebuild all
else
    make "${MAKE_ARGS[@]}" BR2_JLEVEL="$jobs"
fi
echo "Imagens: $ROOT/output-lab/images"
