#!/usr/bin/env python3
"""
Script to convert YOLOv8 ONNX models to TensorRT engines
Supports FP32, FP16, and INT8 precision modes
"""

import argparse
import tensorrt as trt
import numpy as np
from pathlib import Path
import logging
import sys
import os

# Setup logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class TensorRTLogger(trt.ILogger):
    """Custom TensorRT logger"""
    def __init__(self):
        trt.ILogger.__init__(self)

    def log(self, severity, msg):
        if severity <= trt.Logger.WARNING:
            logger.info(f"TensorRT: {msg}")

def check_onnx_model(onnx_path):
    """Validate ONNX model"""
    try:
        import onnx
        model = onnx.load(str(onnx_path))
        onnx.checker.check_model(model)
        logger.info(f"ONNX model validation passed: {onnx_path}")
        return True
    except ImportError:
        logger.warning("ONNX package not found, skipping model validation")
        return True
    except Exception as e:
        logger.error(f"ONNX model validation failed: {e}")
        return False

def build_engine(onnx_path, engine_path, precision='fp16', workspace_size=1<<30, 
                max_batch_size=1, verbose=False):
    """Build TensorRT engine from ONNX model"""
    
    TRT_LOGGER = TensorRTLogger()
    if verbose:
        TRT_LOGGER.min_severity = trt.Logger.VERBOSE
    
    with trt.Builder(TRT_LOGGER) as builder, \
         builder.create_network(1 << int(trt.NetworkDefinitionCreationFlag.EXPLICIT_BATCH)) as network, \
         trt.OnnxParser(network, TRT_LOGGER) as parser, \
         builder.create_builder_config() as config:
        
        # Parse ONNX model
        logger.info(f"Parsing ONNX model: {onnx_path}")
        with open(onnx_path, 'rb') as model:
            if not parser.parse(model.read()):
                logger.error('Failed to parse ONNX model')
                for error in range(parser.num_errors):
                    logger.error(f"Parser error: {parser.get_error(error)}")
                return None
        
        # Print network info
        logger.info(f"Network inputs: {network.num_inputs}")
        logger.info(f"Network outputs: {network.num_outputs}")
        
        for i in range(network.num_inputs):
            input_tensor = network.get_input(i)
            logger.info(f"Input {i}: {input_tensor.name} {input_tensor.shape} {input_tensor.dtype}")
        
        for i in range(network.num_outputs):
            output_tensor = network.get_output(i)
            logger.info(f"Output {i}: {output_tensor.name} {output_tensor.shape} {output_tensor.dtype}")
        
        # Configure builder
        config.max_workspace_size = workspace_size
        logger.info(f"Workspace size: {workspace_size // (1024*1024)} MB")
        
        # Set precision
        if precision == 'fp16':
            if builder.platform_has_fast_fp16:
                config.set_flag(trt.BuilderFlag.FP16)
                logger.info("FP16 mode enabled")
            else:
                logger.warning("FP16 not supported on this platform, falling back to FP32")
        elif precision == 'int8':
            if builder.platform_has_fast_int8:
                config.set_flag(trt.BuilderFlag.INT8)
                logger.info("INT8 mode enabled")
                logger.warning("INT8 requires calibration dataset - use calibration_tool for best results")
            else:
                logger.warning("INT8 not supported on this platform, falling back to FP32")
        else:
            logger.info("FP32 mode enabled")
        
        # Enable optimization profiles for dynamic shapes
        profile = builder.create_optimization_profile()
        
        # Assume YOLOv8 input format: [batch, 3, 640, 640]
        input_name = network.get_input(0).name
        input_shape = network.get_input(0).shape
        
        if input_shape[0] == -1:  # Dynamic batch dimension
            min_shape = (1, input_shape[1], input_shape[2], input_shape[3])
            opt_shape = (max_batch_size, input_shape[1], input_shape[2], input_shape[3])
            max_shape = (max_batch_size * 2, input_shape[1], input_shape[2], input_shape[3])
            
            profile.set_shape(input_name, min_shape, opt_shape, max_shape)
            logger.info(f"Dynamic shapes - Min: {min_shape}, Opt: {opt_shape}, Max: {max_shape}")
        
        config.add_optimization_profile(profile)
        
        # Build engine
        logger.info(f"Building {precision.upper()} engine. This may take several minutes...")
        engine = builder.build_engine(network, config)
        
        if engine is None:
            logger.error("Failed to build engine")
            return None
        
        # Save engine
        logger.info(f"Serializing engine to: {engine_path}")
        with open(engine_path, 'wb') as f:
            f.write(engine.serialize())
        
        # Print engine info
        logger.info(f"Engine built successfully!")
        logger.info(f"Engine file size: {os.path.getsize(engine_path) / (1024*1024):.2f} MB")
        
        return engine

def download_yolov8_model(model_name='yolov8n.onnx'):
    """Download YOLOv8 ONNX model if not present"""
    model_path = Path(model_name)
    if model_path.exists():
        logger.info(f"Model already exists: {model_path}")
        return str(model_path)
    
    logger.info(f"Downloading {model_name}...")
    try:
        import ultralytics
        from ultralytics import YOLO
        
        # Load model and export to ONNX
        model_variant = model_name.replace('.onnx', '.pt')
        model = YOLO(model_variant)
        
        # Export to ONNX with dynamic batch size
        onnx_path = model.export(
            format='onnx',
            dynamic=True,
            simplify=True,
            opset=11
        )
        
        logger.info(f"Model exported to: {onnx_path}")
        return onnx_path
        
    except ImportError:
        logger.error("ultralytics package not found. Please install with: pip install ultralytics")
        logger.error("Or manually download the ONNX model and provide the path")
        return None
    except Exception as e:
        logger.error(f"Failed to download/export model: {e}")
        return None

def main():
    parser = argparse.ArgumentParser(description='Build YOLOv8 TensorRT engine from ONNX model')
    parser.add_argument('onnx_path', nargs='?', help='Path to ONNX model (or model name to download)')
    parser.add_argument('engine_path', nargs='?', help='Output path for TensorRT engine')
    parser.add_argument('--precision', choices=['fp32', 'fp16', 'int8'], 
                       default='fp16', help='Precision mode (default: fp16)')
    parser.add_argument('--workspace', type=int, default=1024, 
                       help='Workspace size in MB (default: 1024)')
    parser.add_argument('--batch-size', type=int, default=1,
                       help='Maximum batch size (default: 1)')
    parser.add_argument('--download', action='store_true',
                       help='Download YOLOv8 model if not found')
    parser.add_argument('--verbose', action='store_true',
                       help='Enable verbose TensorRT logging')
    parser.add_argument('--validate', action='store_true',
                       help='Validate ONNX model before conversion')
    
    args = parser.parse_args()
    
    # Handle model download or path resolution
    if not args.onnx_path:
        if args.download:
            args.onnx_path = download_yolov8_model('yolov8n.onnx')
            if not args.onnx_path:
                return 1
        else:
            parser.print_help()
            return 1
    
    # Handle auto-download for common model names
    if args.onnx_path in ['yolov8n', 'yolov8s', 'yolov8m', 'yolov8l', 'yolov8x']:
        onnx_name = f"{args.onnx_path}.onnx"
        if not Path(onnx_name).exists() and args.download:
            args.onnx_path = download_yolov8_model(onnx_name)
            if not args.onnx_path:
                return 1
        else:
            args.onnx_path = onnx_name
    
    # Generate engine path if not provided
    if not args.engine_path:
        onnx_path = Path(args.onnx_path)
        engine_name = f"{onnx_path.stem}_{args.precision}.trt"
        args.engine_path = str(onnx_path.parent / engine_name)
    
    # Validate inputs
    onnx_path = Path(args.onnx_path)
    if not onnx_path.exists():
        logger.error(f"ONNX model not found: {onnx_path}")
        logger.info("Use --download flag to automatically download YOLOv8 models")
        return 1
    
    if args.validate:
        if not check_onnx_model(onnx_path):
            return 1
    
    engine_path = Path(args.engine_path)
    engine_path.parent.mkdir(parents=True, exist_ok=True)
    
    workspace_size = args.workspace * 1024 * 1024  # Convert MB to bytes
    
    # Build engine
    logger.info("="*60)
    logger.info(f"Building TensorRT engine with following settings:")
    logger.info(f"  ONNX model: {onnx_path}")
    logger.info(f"  Engine output: {engine_path}")
    logger.info(f"  Precision: {args.precision.upper()}")
    logger.info(f"  Workspace: {args.workspace} MB")
    logger.info(f"  Max batch size: {args.batch_size}")
    logger.info("="*60)
    
    engine = build_engine(
        str(onnx_path), 
        str(engine_path), 
        args.precision, 
        workspace_size,
        args.batch_size,
        args.verbose
    )
    
    if engine is None:
        logger.error("Engine build failed!")
        return 1
    
    logger.info("="*60)
    logger.info("Engine build completed successfully!")
    logger.info(f"Engine saved to: {engine_path}")
    logger.info(f"You can now use this engine with the DeepDetect plugin:")
    logger.info(f"  deepdetect engine-path={engine_path}")
    logger.info("="*60)
    
    return 0

if __name__ == '__main__':
    try:
        exit_code = main()
        sys.exit(exit_code)
    except KeyboardInterrupt:
        logger.info("Build interrupted by user")
        sys.exit(1)
    except Exception as e:
        logger.error(f"Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)