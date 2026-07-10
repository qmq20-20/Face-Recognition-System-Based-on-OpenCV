import argparse
import shutil
from pathlib import Path

import cv2
from tqdm import tqdm


def parse_args():
    parser = argparse.ArgumentParser(
        description="Convert WIDER FACE annotations to YOLO detection format."
    )
    parser.add_argument("--images-root", required=True, help="Path to WIDER_train/images or WIDER_val/images.")
    parser.add_argument("--ann-file", required=True, help="Path to wider_face_*_bbx_gt.txt.")
    parser.add_argument("--out-root", required=True, help="Output dataset root, e.g. /workspace/datasets/face.")
    parser.add_argument("--split", required=True, choices=["train", "val", "test"], help="Output split name.")
    parser.add_argument("--min-size", type=int, default=3, help="Skip boxes smaller than this size.")
    parser.add_argument("--copy", action="store_true", help="Copy images instead of creating hard links when possible.")
    return parser.parse_args()


def safe_stem(relative_path):
    path = Path(relative_path)
    parts = list(path.parts)
    name = "_".join(parts)
    return Path(name).stem


def read_image_size(image_path):
    image = cv2.imread(str(image_path))
    if image is None:
        return None
    height, width = image.shape[:2]
    return width, height


def clamp_box(x, y, w, h, image_w, image_h):
    x1 = max(0.0, x)
    y1 = max(0.0, y)
    x2 = min(float(image_w), x + w)
    y2 = min(float(image_h), y + h)
    return x1, y1, max(0.0, x2 - x1), max(0.0, y2 - y1)


def convert_box_to_yolo(x, y, w, h, image_w, image_h):
    cx = (x + w / 2.0) / image_w
    cy = (y + h / 2.0) / image_h
    nw = w / image_w
    nh = h / image_h
    return cx, cy, nw, nh


def copy_or_link(src, dst, force_copy=False):
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists():
        return
    if force_copy:
        shutil.copy2(src, dst)
        return
    try:
        dst.hardlink_to(src)
    except OSError:
        shutil.copy2(src, dst)


def read_wider_records(annotation_file):
    lines = Path(annotation_file).read_text(encoding="utf-8").splitlines()
    index = 0
    records = []
    while index < len(lines):
        image_rel = lines[index].strip()
        index += 1
        if not image_rel:
            continue
        face_count = int(lines[index].strip())
        index += 1
        boxes = []
        for _ in range(face_count):
            values = lines[index].strip().split()
            index += 1
            if len(values) < 10:
                continue
            x, y, w, h = map(float, values[:4])
            invalid = int(values[7])
            boxes.append((x, y, w, h, invalid))
        records.append((image_rel, boxes))
    return records


def main():
    args = parse_args()
    images_root = Path(args.images_root)
    out_root = Path(args.out_root)
    out_images = out_root / "images" / args.split
    out_labels = out_root / "labels" / args.split
    out_images.mkdir(parents=True, exist_ok=True)
    out_labels.mkdir(parents=True, exist_ok=True)

    records = read_wider_records(args.ann_file)
    converted_images = 0
    converted_boxes = 0
    skipped_boxes = 0
    missing_images = 0

    for image_rel, boxes in tqdm(records, desc=f"Converting {args.split}"):
        source_image = images_root / image_rel
        if not source_image.exists():
            missing_images += 1
            continue

        size = read_image_size(source_image)
        if size is None:
            missing_images += 1
            continue
        image_w, image_h = size

        output_stem = safe_stem(image_rel)
        output_image = out_images / f"{output_stem}{source_image.suffix.lower()}"
        output_label = out_labels / f"{output_stem}.txt"

        yolo_lines = []
        for x, y, w, h, invalid in boxes:
            if invalid == 1:
                skipped_boxes += 1
                continue
            x, y, w, h = clamp_box(x, y, w, h, image_w, image_h)
            if w < args.min_size or h < args.min_size:
                skipped_boxes += 1
                continue
            cx, cy, nw, nh = convert_box_to_yolo(x, y, w, h, image_w, image_h)
            if not (0.0 <= cx <= 1.0 and 0.0 <= cy <= 1.0 and 0.0 < nw <= 1.0 and 0.0 < nh <= 1.0):
                skipped_boxes += 1
                continue
            yolo_lines.append(f"0 {cx:.6f} {cy:.6f} {nw:.6f} {nh:.6f}")

        copy_or_link(source_image, output_image, force_copy=args.copy)
        output_label.write_text("\n".join(yolo_lines) + ("\n" if yolo_lines else ""), encoding="utf-8")
        converted_images += 1
        converted_boxes += len(yolo_lines)

    print("Done.")
    print(f"Converted images: {converted_images}")
    print(f"Converted boxes: {converted_boxes}")
    print(f"Skipped boxes: {skipped_boxes}")
    print(f"Missing/bad images: {missing_images}")
    print(f"Output root: {out_root}")


if __name__ == "__main__":
    main()
