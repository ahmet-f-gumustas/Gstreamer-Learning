#!/usr/bin/env python3
"""
YOLOv8 → ONNX → TensorRT INT8 engine.

Usage:
    python3 build_int8_engine.py \
        --model yolov8n.pt \
        --calib-dir models/coco_calib_subset \
        --precision int8 \
        --output models/yolov8n_int8.engine

Notes:
    * Bu script x86 host'ta çalışır; engine dosyası Jetson'a kopyalanabilir AMA
      TensorRT engine'leri GPU'ya özgüdür. Jetson'da kullanmak için aynı
      JetPack sürümünde TensorRT ile yeniden build edilmelidir.
    * INT8 için 500-1000 calibration imajı önerilir (COCO val subset).
"""
import argparse
import os
import sys
import subprocess
from pathlib import Path


def export_onnx(pt_path: str, onnx_path: str, imgsz: int) -> None:
    """Use Ultralytics to export YOLOv8 .pt to ONNX with proper opset."""
    print(f"[1/2] Exporting {pt_path} → {onnx_path}")
    try:
        from ultralytics import YOLO
    except ImportError:
        sys.exit("ultralytics paketi yok.  pip install ultralytics")

    model = YOLO(pt_path)
    model.export(format="onnx", imgsz=imgsz, opset=12, simplify=True, dynamic=False)
    # Ultralytics writes alongside the .pt file
    produced = Path(pt_path).with_suffix(".onnx")
    if produced != Path(onnx_path):
        produced.rename(onnx_path)
    print(f"  ✓ ONNX written: {onnx_path}")


def build_engine(onnx_path: str, engine_path: str, precision: str,
                 calib_dir: str, calib_cache: str, imgsz: int,
                 workspace_mib: int = 4096) -> None:
    """Invoke trtexec — simplest path that handles INT8 calibration."""
    print(f"[2/2] Building {precision.upper()} engine → {engine_path}")
    cmd = [
        "trtexec",
        f"--onnx={onnx_path}",
        f"--saveEngine={engine_path}",
        f"--workspace={workspace_mib}",
        f"--shapes=images:1x3x{imgsz}x{imgsz}",
    ]
    if precision == "fp16":
        cmd.append("--fp16")
    elif precision == "int8":
        cmd.append("--int8")
        cmd.append(f"--calib={calib_cache}")
        # trtexec doesn't itself walk the image directory; we supply the cache.
        # If cache is missing the engine will be implicit-INT8 (poor accuracy).
        # The pipeline binary builds a proper IInt8EntropyCalibrator2 the first
        # time it sees a fresh calib_dir — recommended path.
        if not Path(calib_cache).exists():
            print("  [!] calib cache yok — pipeline ilk açılışta yapılacak.")
            print("      Daha doğru sonuç için scripts/run_calibration.cpp kullan.")

    print("  $ " + " ".join(cmd))
    ret = subprocess.call(cmd)
    if ret != 0:
        sys.exit(f"trtexec başarısız oldu (rc={ret})")
    print(f"  ✓ Engine written: {engine_path}")


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--model", default="yolov8n.pt",
                   help="Path to YOLOv8 .pt or skip if --onnx provided")
    p.add_argument("--onnx", default="",
                   help="Existing ONNX; if set, skip export step")
    p.add_argument("--output", default="models/yolov8n.engine")
    p.add_argument("--precision", choices=["fp32", "fp16", "int8"],
                   default="fp16")
    p.add_argument("--calib-dir",   default="models/coco_calib_subset")
    p.add_argument("--calib-cache", default="models/yolov8n_int8.cache")
    p.add_argument("--imgsz", type=int, default=640)
    args = p.parse_args()

    onnx_path = args.onnx or args.output.replace(".engine", ".onnx")
    if not args.onnx:
        export_onnx(args.model, onnx_path, args.imgsz)

    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    build_engine(onnx_path, args.output, args.precision,
                 args.calib_dir, args.calib_cache, args.imgsz)
    return 0


if __name__ == "__main__":
    sys.exit(main())
