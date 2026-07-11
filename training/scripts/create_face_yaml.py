import argparse
from pathlib import Path


def parse_args():
    parser = argparse.ArgumentParser(description="Create a local Ultralytics face dataset YAML file.")
    parser.add_argument(
        "--dataset-root",
        default="datasets/face",
        help="Dataset root containing images/ and labels/.",
    )
    parser.add_argument(
        "--output",
        default="datasets/face/face.yaml",
        help="Output YAML path.",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    dataset_root = Path(args.dataset_root).resolve()
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    yaml_text = "\n".join(
        [
            f"path: {dataset_root.as_posix()}",
            "train: images/train",
            "val: images/val",
            "test: images/test",
            "names:",
            "  0: face",
            "",
        ]
    )
    output.write_text(yaml_text, encoding="utf-8")
    print(f"Created: {output.resolve()}")
    print(yaml_text)


if __name__ == "__main__":
    main()
