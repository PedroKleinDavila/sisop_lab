#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RESULTS="$ROOT/.lab/results.ext2"
LOG="$ROOT/.lab/sstf.log"
[[ -f "$RESULTS" ]] || { echo "Imagem de resultados não encontrada." >&2; exit 1; }
rm -f "$LOG"
debugfs -R "dump -p /sstf.log $LOG" "$RESULTS" >/dev/null
[[ -s "$LOG" ]] || { echo "Log SSTF vazio ou ausente na imagem." >&2; exit 1; }
python3 "$ROOT/entregas/sstf/report/generate.py" \
    --log "$LOG" --output "$ROOT/entregas/sstf/report/SSTF.pdf"
echo "Relatório atualizado: $ROOT/entregas/sstf/report/SSTF.pdf"
