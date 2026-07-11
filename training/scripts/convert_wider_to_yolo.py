import argparse
import shutil
from pathlib import Path

import cv2
from tqdm import tqdm


def parse_args():
    parser = argparse.ArgumentParser(
        description="Convert WIDER FACE annotations to YOLO detection format."
    )

    parser.add_argument(
        "--images-root",
        required=True,
        help="Path to WIDER_train/images or WIDER_val/images.",
    )

    parser.add_argument(
        "--ann-file",
        required=True,
        help="Path to wider_face_*_bbx_gt.txt.",
    )

    parser.add_argument(
        "--out-root",
        required=True,
        help="Output dataset root, e.g. datasets/face.",
    )

    parser.add_argument(
        "--split",
        required=True,
        choices=["train", "val", "test"],
        help="Output split name.",
    )

    parser.add_argument(
        "--min-size",
        type=int,
        default=3,
        help="Skip boxes whose width or height is smaller than this value.",
    )

    parser.add_argument(
        "--copy",
        action="store_true",
        help="Copy images instead of creating hard links when possible.",
    )

    return parser.parse_args()


def read_image_size(image_path):
    image = cv2.imread(str(image_path))

    if image is None:
        return None

    height, width = image.shape[:2]
    return width, height


def clamp_box(x, y, w, h, image_w, image_h):
    """
    Clamp a bounding box into image boundaries.

    Input format:
        x, y, w, h

    Output format:
        clamped_x, clamped_y, clamped_w, clamped_h
    """

    x1 = max(0.0, float(x))
    y1 = max(0.0, float(y))

    x2 = min(float(image_w), float(x) + float(w))
    y2 = min(float(image_h), float(y) + float(h))

    clamped_w = max(0.0, x2 - x1)
    clamped_h = max(0.0, y2 - y1)

    return x1, y1, clamped_w, clamped_h


def convert_box_to_yolo(x, y, w, h, image_w, image_h):
    """
    Convert an XYWH bounding box to YOLO normalized format:

        center_x, center_y, normalized_width, normalized_height
    """

    center_x = (x + w / 2.0) / image_w
    center_y = (y + h / 2.0) / image_h

    normalized_w = w / image_w
    normalized_h = h / image_h

    return center_x, center_y, normalized_w, normalized_h


def copy_or_link(src, dst, force_copy=False):
    """
    Copy an image or create a hard link.

    Hard links save disk space. If hard-link creation fails,
    the function automatically falls back to copying.
    """

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


def is_zero_placeholder_line(parts):
    """
    Some WIDER FACE annotation files contain an all-zero placeholder
    line after a record whose face count is 0.
    """

    if not parts:
        return False

    try:
        values = [int(value) for value in parts]
    except ValueError:
        return False

    return all(value == 0 for value in values)


def read_wider_records(annotation_file):
    """
    Read WIDER FACE annotation records.

    WIDER FACE format:

        image_relative_path
        face_count
        x y w h blur expression illumination invalid occlusion pose
        ...

    Returns:
        [
            (image_path, [box1, box2, ...]),
            ...
        ]
    """

    annotation_file = Path(annotation_file)

    lines = annotation_file.read_text(
        encoding="utf-8"
    ).splitlines()

    records = []
    index = 0

    while index < len(lines):
        image_path = lines[index].strip()
        index += 1

        if not image_path:
            continue

        if index >= len(lines):
            raise ValueError(
                f"图片 {image_path} 后缺少人脸数量"
            )

        face_count_text = lines[index].strip()
        index += 1

        try:
            face_count = int(face_count_text)
        except ValueError as exc:
            raise ValueError(
                f"图片 {image_path} 后的人脸数量不是整数："
                f"{face_count_text!r}"
            ) from exc

        if face_count < 0:
            raise ValueError(
                f"图片 {image_path} 的人脸数量不能为负数："
                f"{face_count}"
            )

        boxes = []

        if face_count == 0:
            # 某些版本的 WIDER FACE 标注会在 0 后面
            # 再放一行全零占位数据，需要跳过。
            if index < len(lines):
                parts = lines[index].strip().split()

                if is_zero_placeholder_line(parts):
                    index += 1

        else:
            for box_index in range(face_count):
                if index >= len(lines):
                    raise ValueError(
                        f"图片 {image_path} 的人脸框数量不足："
                        f"声明 {face_count} 个，"
                        f"实际只读取到 {box_index} 个"
                    )

                raw_values = lines[index].strip().split()
                index += 1

                if len(raw_values) < 8:
                    raise ValueError(
                        f"图片 {image_path} 的第 {box_index + 1} 个框"
                        f"字段不足：至少需要 8 个字段，"
                        f"实际为 {len(raw_values)} 个，"
                        f"内容为 {raw_values}"
                    )

                try:
                    values = [int(value) for value in raw_values]
                except ValueError as exc:
                    raise ValueError(
                        f"图片 {image_path} 的第 {box_index + 1} 个框"
                        f"包含非整数内容：{raw_values}"
                    ) from exc

                boxes.append(values)

        records.append((image_path, boxes))

    return records


def main():
    args = parse_args()

    images_root = Path(args.images_root)
    annotation_file = Path(args.ann_file)
    out_root = Path(args.out_root)

    if not images_root.exists():
        raise FileNotFoundError(
            f"图片根目录不存在：{images_root.resolve()}"
        )

    if not images_root.is_dir():
        raise NotADirectoryError(
            f"图片根路径不是目录：{images_root.resolve()}"
        )

    if not annotation_file.exists():
        raise FileNotFoundError(
            f"标注文件不存在：{annotation_file.resolve()}"
        )

    if not annotation_file.is_file():
        raise ValueError(
            f"标注文件路径不是文件：{annotation_file.resolve()}"
        )

    if args.min_size < 1:
        raise ValueError("--min-size 必须大于等于 1")

    out_images = out_root / "images" / args.split
    out_labels = out_root / "labels" / args.split

    out_images.mkdir(parents=True, exist_ok=True)
    out_labels.mkdir(parents=True, exist_ok=True)

    records = read_wider_records(annotation_file)

    converted_images = 0
    converted_boxes = 0
    skipped_boxes = 0
    missing_images = 0
    empty_label_images = 0

    for image_rel, boxes in tqdm(
        records,
        desc=f"Converting {args.split}"
    ):
        relative_path = Path(image_rel)
        source_image = images_root / relative_path

        if not source_image.exists():
            missing_images += 1
            continue

        image_size = read_image_size(source_image)

        if image_size is None:
            missing_images += 1
            continue

        image_w, image_h = image_size

        if image_w <= 0 or image_h <= 0:
            missing_images += 1
            continue

        # 保留 WIDER FACE 的原始子目录结构。
        output_image = out_images / relative_path
        output_label = (
            out_labels / relative_path.with_suffix(".txt")
        )

        yolo_lines = []

        for box in boxes:
            if len(box) < 8:
                skipped_boxes += 1
                continue

            # WIDER FACE:
            # x y w h blur expression illumination invalid occlusion pose
            x, y, w, h = box[:4]
            invalid = box[7]

            if invalid == 1:
                skipped_boxes += 1
                continue

            if w <= 0 or h <= 0:
                skipped_boxes += 1
                continue

            x, y, w, h = clamp_box(
                x=x,
                y=y,
                w=w,
                h=h,
                image_w=image_w,
                image_h=image_h,
            )

            if w < args.min_size or h < args.min_size:
                skipped_boxes += 1
                continue

            cx, cy, normalized_w, normalized_h = (
                convert_box_to_yolo(
                    x=x,
                    y=y,
                    w=w,
                    h=h,
                    image_w=image_w,
                    image_h=image_h,
                )
            )

            if not (
                0.0 <= cx <= 1.0
                and 0.0 <= cy <= 1.0
                and 0.0 < normalized_w <= 1.0
                and 0.0 < normalized_h <= 1.0
            ):
                skipped_boxes += 1
                continue

            yolo_lines.append(
                f"0 "
                f"{cx:.6f} "
                f"{cy:.6f} "
                f"{normalized_w:.6f} "
                f"{normalized_h:.6f}"
            )

        copy_or_link(
            src=source_image,
            dst=output_image,
            force_copy=args.copy,
        )

        output_label.parent.mkdir(
            parents=True,
            exist_ok=True
        )

        label_content = "\n".join(yolo_lines)

        if label_content:
            label_content += "\n"
        else:
            empty_label_images += 1

        output_label.write_text(
            label_content,
            encoding="utf-8"
        )

        converted_images += 1
        converted_boxes += len(yolo_lines)

    print()
    print("Done.")
    print(f"Total annotation records: {len(records)}")
    print(f"Converted images: {converted_images}")
    print(f"Converted boxes: {converted_boxes}")
    print(f"Skipped boxes: {skipped_boxes}")
    print(f"Images with empty labels: {empty_label_images}")
    print(f"Missing/bad images: {missing_images}")
    print(f"Output root: {out_root.resolve()}")

    if converted_images == 0:
        print()
        print(
            "警告：没有成功转换任何图片，请检查 --images-root。"
        )

    if missing_images == len(records):
        print(
            "警告：所有图片均缺失，图片根目录很可能设置错误。"
        )


if __name__ == "__main__":
    main()