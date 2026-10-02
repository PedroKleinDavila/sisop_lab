#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LAB="$ROOT/.lab"
ARCHIVE="$LAB/cache/linux-4.13.9.tar.xz"
SOURCE="$LAB/src/linux-4.13.9"
PATCH="$ROOT/entregas/tutorial-2.2/kernel-integration.patch"
EXPECTED_SHA="a1dc15e4dba9a385c4112903da81c01f412b36420feefa7771797073f5352a8f"

[[ "$(uname -s)" == Linux ]] || { echo "Execute este script no Codespace Linux." >&2; exit 1; }
[[ "$(uname -m)" == x86_64 ]] || { echo "O Codespace precisa ser x86_64." >&2; exit 1; }
source /etc/os-release
case "$ID" in ubuntu|debian) ;; *) echo "Distribuição não suportada: $ID" >&2; exit 1 ;; esac

printf 'Ambiente: %s %s, %s CPUs\n' "$ID" "$VERSION_ID" "$(nproc)"
free -h
df -h "$ROOT"
available_kib="$(df -Pk "$ROOT" | awk 'NR==2 {print $4}')"
(( available_kib >= 12 * 1024 * 1024 )) || {
    echo "São necessários ao menos 12 GiB livres para o Buildroot e o kernel." >&2
    exit 1
}

packages=(build-essential bc bison flex libssl-dev libelf-dev libncurses-dev
          rsync cpio unzip wget curl xz-utils qemu-system-x86 zip patch
          python3 python3-reportlab e2fsprogs gawk file git perl)
missing=()
for package in "${packages[@]}"; do
    dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -qx 'install ok installed' ||
        missing+=("$package")
done
if (("${#missing[@]}" > 0)); then
    echo "Instalando dependências: ${missing[*]}"
    sudo apt-get update
    sudo apt-get install -y "${missing[@]}"
fi

mkdir -p "$LAB/cache" "$LAB/src" "$ROOT/output-lab"
if [[ ! -f "$ARCHIVE" ]]; then
    curl --fail --location --retry 3 \
        'https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.13.9.tar.xz' \
        --output "$ARCHIVE"
fi
echo "$EXPECTED_SHA  $ARCHIVE" | sha256sum --check --status || {
    echo "Checksum incorreto para o kernel 4.13.9; remova o arquivo e tente novamente." >&2
    exit 1
}

patch_sha="none"
if [[ -f "$PATCH" ]]; then
    patch_sha="$(sha256sum "$PATCH" | awk '{print $1}')"
fi
if [[ ! -f "$SOURCE/.sisop-patch-sha" ]] ||
   [[ "$(cat "$SOURCE/.sisop-patch-sha")" != "$patch_sha" ]]; then
    rm -rf "$SOURCE"
    tar -xJf "$ARCHIVE" -C "$LAB/src"
    if [[ -f "$PATCH" ]]; then
        patch --directory "$SOURCE" -p1 --forward < "$PATCH"
    fi
    echo "$patch_sha" > "$SOURCE/.sisop-patch-sha"
fi
if [[ -f "$PATCH" ]]; then
    cp "$ROOT/entregas/tutorial-2.2/exemplo/process_info.c" "$SOURCE/syscall/"
    cp "$ROOT/entregas/tutorial-2.2/desafio1/list_sleeping.c" "$SOURCE/syscall/"
    cp "$ROOT/entregas/tutorial-2.2/desafio2/log_message.c" "$SOURCE/syscall/"
fi
echo "LINUX_OVERRIDE_SRCDIR = $SOURCE" > "$ROOT/output-lab/local.mk"
printf 'Kernel preparado em %s\n' "$SOURCE"
