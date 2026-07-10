import argparse
import random
from pathlib import Path

import cv2


IMAGE_SUFFIXES = {".jpg", ".jpeg", ".png", ".bmp"}


def parse_args():
    parser = argparse.ArgumentParser(description="Draw YOLO labels on random images for visual verification.")
    parser.add_argument("--dataset-root", required=True, help="Dataset root containing images/ and labels/.")
    parser.add_argument("--split", default="train", choices=["train", "val", "test"], help="Dataset split.")
    parser.add_argument("--output-dir", required=True, help="Directory to save preview images.")
    parser.add_argument("--count", type=int, default=100, help="How many images to sample.")
    parser.add_argument("--seed", type=int, default=42, help="Random seed.")
    return parser.parse_args()


def read_label(label_path):
    boxes = []
    if not label_path.exists():
        return boxes
    for line_no, line in enumerate(label_path.read_text(encoding="utf-8").splitlines(), start=1):
        line = line.strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) != 5:
            raise ValueError(f"{label_path}:{line_no} should have 5 fields, got {len(parts)}")
        cls, cx, cy, w, h = parts
        cls = int(cls)
        values = [float(cx), float(cy), float(w), float(h)]
        if cls != 0:
            raise ValueError(f"{label_path}:{line_no} class id should be 0, got {cls}")
        if not all(0.0 <= v <= 1.0 for v in values):
            raise ValueError(f"{label_path}:{line_no} box values must be in [0, 1], got {values}")
        if values[2] <= 0.0 or values[3] <= 0.0:
            raise ValueError(f"{label_path}:{line_no} width and height must be > 0, got {values}")
        boxes.append(values)
    return boxes


def yolo_to_xyxy(box, image_w, image_h):
    cx, cy, w, h = box
    x1 = int((cx - w / 2.0) * image_w)
    y1 = int((cy - h / 2.0) * image_h)
    x2 = int((cx + w / 2.0) * image_w)
    y2 = int((cy + h / 2.0) * image_h)
    return x1, y1, x2, y2


def main():
    args = parse_args()
    dataset_root = Path(args.dataset_root)
    image_dir = dataset_root / "images" / args.split
    label_dir = dataset_root / "labels" / args.split
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    images = sorted(p for p in image_dir.rglob("*") if p.suffix.lower() in IMAGE_SUFFIXES)
    if not images:
        raise SystemExit(f"No images found in {image_dir}")

    random.seed(args.seed)
    sample = random.sample(images, min(args.count, len(images)))

    total_boxes = 0
    for image_path in sample:
        image = cv2.imread(str(image_path))
        if image is None:
            print(f"Skip unreadable image: {image_path}")
            continue
        image_h, image_w = image.shape[:2]
        label_path = label_dir / f"{image_path.stem}.txt"
        boxes = read_label(label_path)
        total_boxes += len(boxes)
        for box in boxes:
            x1, y1, x2, y2 = yolo_to_xyxy(box, image_w, image_h)
            cv2.rectangle(image, (x1, y1), (x2, y2), (0, 0, 255), 2)
            cv2.putText(image, "face", (x1, max(0, y1 - 5)), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)
        cv2.imwrite(str(output_dir / image_path.name), image)

    print(f"Checked images: {len(sample)}")
    print(f"Total boxes drawn: {total_boxes}")
    print(f"Preview dir: {output_dir}")


if __name__ == "__main__":
    main()
