#!/usr/bin/env python3
"""
İki perf.csv dosyasını karşılaştırır: biri x86 host'ta, diğeri Jetson'da
ölçülmüş.  Karşılaştırma tablosu + ASCII bar chart üretir.

Kullanım:
    python3 compare_x86_vs_jetson.py --x86 logs/x86_perf.csv --jetson logs/jetson_perf.csv
"""
import argparse
import sys
from pathlib import Path

try:
    import pandas as pd
except ImportError:
    sys.exit("pandas gerekli.  pip install pandas")


def bar(value: float, vmax: float, width: int = 30) -> str:
    if vmax <= 0: return ""
    n = int(value / vmax * width)
    return "█" * n + "░" * (width - n)


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--x86", required=True)
    p.add_argument("--jetson", required=True)
    args = p.parse_args()

    for path in (args.x86, args.jetson):
        if not Path(path).exists():
            sys.exit(f"Yok: {path}")

    x = pd.read_csv(args.x86)
    j = pd.read_csv(args.jetson)

    metrics = [
        ("Mean FPS",          "fps",            lambda d: d.replace(0, pd.NA).dropna().mean()),
        ("p95 inf (ms)",      "inference_ms",   lambda d: d.quantile(0.95)),
        ("Mean inf (ms)",     "inference_ms",   lambda d: d.mean()),
        ("Mean power (W)",    "power_total_mw", lambda d: d.mean() / 1000.0),
        ("Peak temp (°C)",    "thermal_temp_c", lambda d: d.max()),
        ("Mean GPU load (%)", "gpu_pct",        lambda d: d.mean()),
    ]

    print(f"\n{'Metric':<22}{'x86':>14}{'Jetson':>14}{'Δ':>10}")
    print("─" * 60)
    for label, col, fn in metrics:
        try:
            xv = fn(x[col])
            jv = fn(j[col])
        except KeyError:
            continue
        delta = (jv - xv) / xv * 100 if xv else 0
        print(f"{label:<22}{xv:>14.2f}{jv:>14.2f}{delta:>9.1f}%")

    # FPS/W bar
    x_fps = x["fps"].replace(0, pd.NA).dropna().mean()
    j_fps = j["fps"].replace(0, pd.NA).dropna().mean()
    x_pw  = x["power_total_mw"].mean() / 1000.0 or 1
    j_pw  = j["power_total_mw"].mean() / 1000.0 or 1
    x_fpw = x_fps / x_pw
    j_fpw = j_fps / j_pw

    print(f"\nFPS / Watt (verimlilik):")
    vmax = max(x_fpw, j_fpw, 1)
    print(f"  x86     {bar(x_fpw, vmax)} {x_fpw:6.2f}")
    print(f"  Jetson  {bar(j_fpw, vmax)} {j_fpw:6.2f}")
    print(f"\n  → Jetson {j_fpw/max(x_fpw,1e-6):.1f}x daha verimli (FPS/W)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
