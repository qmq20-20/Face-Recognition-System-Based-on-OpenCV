# Face Detector Training Tools

这个目录放训练人脸检测模型需要的配置和脚本。

推荐先阅读：

```text
docs/beginner_yolo_face_training.md
docs/train_face_detector_plan.md
```

## Quick Start

```bash
python3 -m venv ~/venvs/face-yolo
source ~/venvs/face-yolo/bin/activate
pip install -r training/requirements.txt
```

转换 WIDER FACE：

```bash
python training/scripts/convert_wider_to_yolo.py \
  --images-root /workspace/raw/WIDER_train/images \
  --ann-file /workspace/raw/wider_face_split/wider_face_train_bbx_gt.txt \
  --out-root /workspace/datasets/face \
  --split train
```

如果 WIDER FACE 旧下载地址返回 Forbidden，不要继续卡在旧地址。先阅读：

```text
docs/beginner_yolo_face_training.md
```

里面有 Kaggle、Roboflow、Hugging Face、Open Images 等替代路线。

可视化检查标签：

```bash
python training/scripts/verify_labels.py \
  --dataset-root /workspace/datasets/face \
  --split train \
  --output-dir /workspace/datasets/face/preview_train \
  --count 100
```

训练：

```bash
yolo detect train model=yolo26n.pt data=/workspace/datasets/face/face.yaml imgsz=640 epochs=3 batch=-1 device=0
```

导出：

```bash
python training/scripts/export_onnx.py \
  --model /workspace/runs/face_yolo26s_640/weights/best.pt \
  --imgsz 640 \
  --opset 12
```
