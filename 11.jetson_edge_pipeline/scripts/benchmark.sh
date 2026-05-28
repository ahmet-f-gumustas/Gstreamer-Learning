#!/usr/bin/env bash
# 1000-frame benchmark çalıştırır ve sonuçları CSV/JSON olarak yazar.
# Hem x86 hem aarch64 üzerinde aynı şekilde çalışır.
set -euo pipefail

BIN="${BIN:-./build/jetson_edge}"
CFG="${CFG:-config/pipeline.yaml}"
OUT="${OUT:-logs/bench_$(date +%Y%m%d_%H%M%S)}"

mkdir -p "$OUT"
mkdir -p "$(dirname "$OUT/perf.csv")"

echo "=== Benchmark başlıyor ==="
echo "Binary: $BIN"
echo "Config: $CFG"
echo "Out:    $OUT"

PRECISIONS=(fp16 int8)
POWER_MODES=(7w 15w maxn)

for prec in "${PRECISIONS[@]}"; do
  for pm in "${POWER_MODES[@]}"; do
    label="${prec}_${pm}"
    echo
    echo "--- $label ---"
    "$BIN" \
      --config "$CFG" \
      --precision "$prec" \
      --power "$pm" \
      --headless \
      --benchmark \
      --record "" 2>&1 | tee "$OUT/run_$label.log"
    # main.cpp varsayılan olarak perf.csv ve perf_summary.json yazar
    mv -f logs/perf.csv          "$OUT/perf_$label.csv"          || true
    mv -f logs/perf_summary.json "$OUT/perf_summary_$label.json" || true
  done
done

echo
echo "=== Tüm benchmark koşumları tamam ==="
echo "Sonuçlar: $OUT/"
ls -1 "$OUT/"
