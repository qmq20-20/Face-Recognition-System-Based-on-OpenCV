import argparse
import random
from pathlib import Path

import cv2


IMAGE_SUFFIXES = {".jpg", ".jpeg", ".png", ".bmp"}


def parse_args():
    parser = argparse.ArgumentParser(
        description="Draw YOLO labels on random images for visual verification."
    )

    parser.add_argument(
        "--dataset-root",
        required=True,
        help="Dataset root containing images/ and labels/.",
    )

    parser.add_argument(
        "--split",
        default="train",
        choices=["train", "val", "test"],
        help="Dataset split.",
    )

    parser.add_argument(
        "--output-dir",
        required=True,
        help="Directory to save preview images.",
    )

    parser.add_argument(
        "--count",
        type=int,
        default=100,
        help="How many images to sample.",
    )

    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Random seed.",
    )

    return parser.parse_args()


def read_label(label_path):
    boxes = []

    if not label_path.exists():
        return boxes

    lines = label_path.read_text(
        encoding="utf-8"
    ).splitlines()

    for line_no, line in enumerate(lines, start=1):
        line = line.strip()

        if not line:
            continue

        parts = line.split()

        if len(parts) != 5:
            raise ValueError(
                f"{label_path}:{line_no} should have 5 fields, "
                f"got {len(parts)}"
            )

        cls_text, cx_text, cy_text, w_text, h_text = parts

        try:
            cls = int(cls_text)
            cx = float(cx_text)
            cy = float(cy_text)
            w = float(w_text)
            h = float(h_text)
        except ValueError as exc:
            raise ValueError(
                f"{label_path}:{line_no} contains invalid numbers: {parts}"
            ) from exc

        if cls != 0:
            raise ValueError(
                f"{label_path}:{line_no} class id should be 0, "
                f"got {cls}"
            )

        values = [cx, cy, w, h]

        if not all(0.0 <= value <= 1.0 for value in values):
            raise ValueError(
                f"{label_path}:{line_no} box values must be in [0, 1], "
                f"got {values}"
            )

        if w <= 0.0 or h <= 0.0:
            raise ValueError(
                f"{label_path}:{line_no} width and height must be > 0, "
                f"got {values}"
            )

        boxes.append(values)

    return boxes


def yolo_to_xyxy(box, image_w, image_h):
    cx, cy, w, h = box

    x1 = round((cx - w / 2.0) * image_w)
    y1 = round((cy - h / 2.0) * image_h)
    x2 = round((cx + w / 2.0) * image_w)
    y2 = round((cy + h / 2.0) * image_h)

    x1 = max(0, min(image_w - 1, x1))
    y1 = max(0, min(image_h - 1, y1))
    x2 = max(0, min(image_w - 1, x2))
    y2 = max(0, min(image_h - 1, y2))

    return x1, y1, x2, y2


def main():
    args = parse_args()

    if args.count <= 0:
        raise ValueError("--count must be greater than 0")

    dataset_root = Path(args.dataset_root)
    image_dir = dataset_root / "images" / args.split
    label_dir = dataset_root / "labels" / args.split
    output_dir = Path(args.output_dir)

    if not image_dir.exists():
        raise FileNotFoundError(
            f"Image directory does not exist: {image_dir.resolve()}"
        )

    if not label_dir.exists():
        raise FileNotFoundError(
            f"Label directory does not exist: {label_dir.resolve()}"
        )

    output_dir.mkdir(parents=True, exist_ok=True)

    images = sorted(
        path
        for path in image_dir.rglob("*")
        if path.is_file()
        and path.suffix.lower() in IMAGE_SUFFIXES
    )

    if not images:
        raise SystemExit(
            f"No images found in {image_dir.resolve()}"
        )

    random_generator = random.Random(args.seed)

    sample_count = min(args.count, len(images))
    sampled_images = random_generator.sample(
        images,
        sample_count,
    )

    checked_images = 0
    unreadable_images = 0
    missing_labels = 0
    empty_labels = 0
    total_boxes = 0

    for image_path in sampled_images:
        image = cv2.imread(str(image_path))

        if image is None:
            unreadable_images += 1
            print(f"Skip unreadable image: {image_path}")
            continue

        image_h, image_w = image.shape[:2]

        # 获取图片相对于 images/train 的路径。
        # 例如：
        # 0--Parade/example.jpg
        relative_image_path = image_path.relative_to(image_dir)

        # 标签保留相同的子目录结构。
        # 例如：
        # labels/train/0--Parade/example.txt
        label_path = (
            label_dir
            / relative_image_path.with_suffix(".txt")
        )

        if not label_path.exists():
            missing_labels += 1
            print(f"Missing label: {label_path}")
            boxes = []
        else:
            boxes = read_label(label_path)

            if not boxes:
                empty_labels += 1

        total_boxes += len(boxes)

        for box in boxes:
            x1, y1, x2, y2 = yolo_to_xyxy(
                box,
                image_w,
                image_h,
            )

            if x2 <= x1 or y2 <= y1:
                print(
                    f"Skip invalid converted box in {label_path}: "
                    f"{box}"
                )
                continue

            cv2.rectangle(
                image,
                (x1, y1),
                (x2, y2),
                (0, 0, 255),
                2,
            )

            text_y = max(20, y1 - 5)

            cv2.putText(
                image,
                "face",
                (x1, text_y),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.6,
                (0, 0, 255),
                2,
                cv2.LINE_AA,
            )

        # 输出预览也保留相对目录，避免同名图片覆盖。
        output_image = output_dir / relative_image_path
        output_image.parent.mkdir(
            parents=True,
            exist_ok=True,
        )

        success = cv2.imwrite(
            str(output_image),
            image,
        )

        if not success:
            print(f"Failed to write preview: {output_image}")
            continue

        checked_images += 1

    print()
    print("Verification completed.")
    print(f"Sampled images: {len(sampled_images)}")
    print(f"Checked images: {checked_images}")
    print(f"Unreadable images: {unreadable_images}")
    print(f"Missing label files: {missing_labels}")
    print(f"Empty label files: {empty_labels}")
    print(f"Total boxes drawn: {total_boxes}")
    print(f"Preview dir: {output_dir.resolve()}")

    if missing_labels == len(sampled_images):
        print()
        print(
            "Warning: every sampled image is missing its label file. "
            "Check whether images and labels use the same subdirectory structure."
        )

    if total_boxes == 0:
        print()
        print(
            "Warning: no boxes were drawn. Check missing-label and "
            "empty-label counts above."
        )


if __name__ == "__main__":
    main()