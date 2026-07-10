import argparse
from pathlib import Path

from ultralytics import YOLO


def parse_args():
    parser = argparse.ArgumentParser(description="Export an Ultralytics YOLO model to ONNX.")
    parser.add_argument("--model", required=True, help="Path to best.pt or another YOLO checkpoint.")
    parser.add_argument("--imgsz", type=int, default=640, help="Export image size.")
    parser.add_argument("--opset", type=int, default=12, help="ONNX opset version.")
    parser.add_argument("--no-simplify", action="store_true", help="Disable ONNX simplification.")
    return parser.parse_args()


def main():
    args = parse_args()
    model_path = Path(args.model)
    if not model_path.exists():
        raise SystemExit(f"Model not found: {model_path}")
    model = YOLO(str(model_path))
    exported = model.export(
        format="onnx",
        imgsz=args.imgsz,
        opset=args.opset,
        simplify=not args.no_simplify,
    )
    print(f"Exported model: {exported}")


if __name__ == "__main__":
    main()
