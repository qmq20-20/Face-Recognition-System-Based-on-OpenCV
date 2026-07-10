import argparse
import random
import shutil
from pathlib import Path


IMAGE_SUFFIXES = {".jpg", ".jpeg", ".png", ".bmp"}


def parse_args():
    parser = argparse.ArgumentParser(description="Split custom YOLO images/labels into train/val/test.")
    parser.add_argument("--images-dir", required=True, help="Directory containing custom images.")
    parser.add_argument("--labels-dir", required=True, help="Directory containing YOLO label txt files.")
    parser.add_argument("--out-root", required=True, help="Output dataset root.")
    parser.add_argument("--train-ratio", type=float, default=0.7)
    parser.add_argument("--val-ratio", type=float, default=0.2)
    parser.add_argument("--test-ratio", type=float, default=0.1)
    parser.add_argument("--seed", type=int, default=42)
    return parser.parse_args()


def ensure_ratios(train_ratio, val_ratio, test_ratio):
    total = train_ratio + val_ratio + test_ratio
    if abs(total - 1.0) > 1e-6:
        raise ValueError(f"Ratios must sum to 1.0, got {total}")


def copy_pair(image_path, label_path, out_root, split):
    out_image = out_root / "images" / split / image_path.name
    out_label = out_root / "labels" / split / f"{image_path.stem}.txt"
    out_image.parent.mkdir(parents=True, exist_ok=True)
    out_label.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(image_path, out_image)
    if label_path.exists():
        shutil.copy2(label_path, out_label)
    else:
        out_label.write_text("", encoding="utf-8")


def main():
    args = parse_args()
    ensure_ratios(args.train_ratio, args.val_ratio, args.test_ratio)
    images_dir = Path(args.images_dir)
    labels_dir = Path(args.labels_dir)
    out_root = Path(args.out_root)

    images = sorted(p for p in images_dir.rglob("*") if p.suffix.lower() in IMAGE_SUFFIXES)
    if not images:
        raise SystemExit(f"No images found in {images_dir}")

    random.seed(args.seed)
    random.shuffle(images)

    train_end = int(len(images) * args.train_ratio)
    val_end = train_end + int(len(images) * args.val_ratio)
    splits = {
        "train": images[:train_end],
        "val": images[train_end:val_end],
        "test": images[val_end:],
    }

    for split, split_images in splits.items():
        for image_path in split_images:
            label_path = labels_dir / f"{image_path.stem}.txt"
            copy_pair(image_path, label_path, out_root, split)
        print(f"{split}: {len(split_images)} images")

    print(f"Output root: {out_root}")


if __name__ == "__main__":
    main()
