#!/usr/bin/env python3
"""
Orin Nano performance simulator.

x86 üzerinde çalıştırılmış pipeline'ın CSV log dosyasını alır, OrinSimulator
formülleriyle Orin Nano 7W/15W/MAXN profillerine scale eder ve karşılaştırma
tablosu basar.  Gerçek hardware yoksa Project README'sinde "tahmini Orin Nano
performansı" göstermek için kullanılır.
"""
import argparse
import json
import sys
from pathlib import Path

try:
    import pandas as pd
except ImportError:
    sys.exit("Bu script pandas gerektirir.  pip install pandas")


# Aynı değerler include/monitoring/orin_simulator.h ile senkron tutulmalı.
ORIN_NANO = {
    "7W":   {"int8_tops": 20, "fp16_tflops": 5.0,  "mem_bw_gb": 68, "tdp": 7},
    "15W":  {"int8_tops": 40, "fp16_tflops": 10.0, "mem_bw_gb": 68, "tdp": 15},
    "MAXN": {"int8_tops": 50, "fp16_tflops": 12.5, "mem_bw_gb": 68, "tdp": 20},
}

# trtexec --help "Performance Characteristics" tablosu + NVIDIA whitepapers.
HOST_GPUS = {
    "RTX 4090": {"int8_tops": 1321, "fp16_tflops": 165, "mem_bw_gb": 1008, "tdp": 450},
    "RTX 4080": {"int8_tops": 780,  "fp16_tflops": 97,  "mem_bw_gb": 717,  "tdp": 320},
    "RTX 4070": {"int8_tops": 466,  "fp16_tflops": 58,  "mem_bw_gb": 504,  "tdp": 200},
    "RTX 3090": {"int8_tops": 568,  "fp16_tflops": 71,  "mem_bw_gb": 936,  "tdp": 350},
    "RTX 3080": {"int8_tops": 476,  "fp16_tflops": 59,  "mem_bw_gb": 760,  "tdp": 320},
    "RTX 3070": {"int8_tops": 326,  "fp16_tflops": 40,  "mem_bw_gb": 448,  "tdp": 220},
    "RTX 2080": {"int8_tops": 226,  "fp16_tflops": 28,  "mem_bw_gb": 448,  "tdp": 250},
    "A100":     {"int8_tops": 624,  "fp16_tflops": 156, "mem_bw_gb": 1555, "tdp": 400},
}


def scale_fps(host_fps: float, host_gpu: dict, orin_profile: dict,
              compute_bound: float = 0.7) -> float:
    """Mixed workload: 70% compute-bound + 30% memory-bound."""
    compute_scale = orin_profile["int8_tops"] / host_gpu["int8_tops"]
    memory_scale  = orin_profile["mem_bw_gb"] / host_gpu["mem_bw_gb"]
    return host_fps * (compute_bound * compute_scale +
                       (1 - compute_bound) * memory_scale)


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("csv", help="perf.csv from a pipeline run on x86")
    p.add_argument("--host-gpu", default="RTX 4090",
                   choices=list(HOST_GPUS.keys()),
                   help="Hangi GPU'da ölçüm yapıldı")
    p.add_argument("--output", default="orin_simulation.json")
    args = p.parse_args()

    if not Path(args.csv).exists():
        sys.exit(f"CSV yok: {args.csv}")

    df = pd.read_csv(args.csv)
    if df.empty:
        sys.exit("CSV boş")

    host_gpu = HOST_GPUS[args.host_gpu]
    host_fps = df["fps"].replace(0, pd.NA).dropna().mean()
    host_inf = df["inference_ms"].mean()

    print(f"\n=== Host ({args.host_gpu}) ölçüm ortalaması ===")
    print(f"  Mean FPS:           {host_fps:.1f}")
    print(f"  Mean inf time (ms): {host_inf:.2f}")
    print(f"  Frames:             {len(df)}")

    print(f"\n=== Tahmini Orin Nano performansı ===")
    print(f"{'Profile':<8}{'FPS':>10}{'Inf (ms)':>12}{'TDP (W)':>10}{'FPS/W':>10}")
    print("-" * 50)

    summary = {"host_gpu": args.host_gpu,
               "host_fps": float(host_fps),
               "host_inf_ms": float(host_inf),
               "estimates": {}}

    for name, prof in ORIN_NANO.items():
        fps_o = scale_fps(host_fps, host_gpu, prof)
        inf_o = 1000.0 / fps_o if fps_o > 0 else float("inf")
        fpw   = fps_o / prof["tdp"]
        print(f"{name:<8}{fps_o:>10.1f}{inf_o:>12.2f}{prof['tdp']:>10}{fpw:>10.2f}")
        summary["estimates"][name] = {
            "fps":          round(fps_o, 1),
            "inf_ms":       round(inf_o, 2),
            "tdp_w":        prof["tdp"],
            "fps_per_watt": round(fpw, 2),
        }

    Path(args.output).write_text(json.dumps(summary, indent=2))
    print(f"\n  ✓ Yazıldı: {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
