# 本地 YOLO 人脸检测训练工具

完整新手教程：

```text
docs/beginner_yolo_face_training.md
```

当前推荐环境：

```text
Windows 10/11
Python 3.11 64-bit
NVIDIA RTX 5060 Laptop GPU
PyTorch CUDA build
Ultralytics YOLO
```

## 1. 创建环境

在项目根目录运行：

```powershell
py -3.11 -m venv .venv-training
.\.venv-training\Scripts\Activate.ps1
python -m pip install --upgrade pip setuptools wheel
pip install torch torchvision
pip install -r training\requirements.txt
```

## 2. 检查 GPU

```powershell
nvidia-smi
python training\scripts\check_gpu.py
```

必须看到：

```text
CUDA available: True
```

## 3. 转换 WIDER FACE

训练集：

```powershell
python training\scripts\convert_wider_to_yolo.py `
  --images-root raw\WIDER_train\images `
  --ann-file raw\wider_face_split\wider_face_train_bbx_gt.txt `
  --out-root datasets\face `
  --split train
```

验证集：

```powershell
python training\scripts\convert_wider_to_yolo.py `
  --images-root raw\WIDER_val\images `
  --ann-file raw\wider_face_split\wider_face_val_bbx_gt.txt `
  --out-root datasets\face `
  --split val
```

## 4. 创建本地 YAML

```powershell
python training\scripts\create_face_yaml.py --dataset-root datasets\face
```

## 5. 检查标签

```powershell
python training\scripts\verify_labels.py `
  --dataset-root datasets\face `
  --split train `
  --output-dir datasets\face\preview_train `
  --count 100
```

## 6. Smoke Test

```powershell
yolo detect train `
  model=yolo11n.pt `
  data=datasets\face\face.yaml `
  imgsz=640 `
  epochs=1 `
  batch=2 `
  workers=0 `
  device=0 `
  amp=True `
  cache=False `
  project=training\runs `
  name=smoke
```

## 7. 正式训练

```powershell
yolo detect train `
  model=yolo11n.pt `
  data=datasets\face\face.yaml `
  imgsz=640 `
  epochs=80 `
  batch=4 `
  workers=2 `
  device=0 `
  amp=True `
  cache=False `
  patience=20 `
  save_period=5 `
  project=training\runs `
  name=face_yolo11n_640
```

显存不足时，按顺序调整：

```text
batch=4 → 2 → 1
workers=2 → 0
yolo11s → yolo11n
imgsz=640 → 512
```

## 8. 导出 ONNX

```powershell
python training\scripts\export_onnx.py `
  --model training\runs\face_yolo11n_640\weights\best.pt `
  --imgsz 640 `
  --opset 12
```

## 脚本说明

```text
check_gpu.py                 检查 PyTorch 是否真正使用 NVIDIA GPU
convert_wider_to_yolo.py    WIDER FACE 标注转 YOLO
create_face_yaml.py         自动生成本机绝对路径 YAML
verify_labels.py            随机画框检查标签
split_custom_data.py        划分自采集数据 train/val/test
export_onnx.py              导出 ONNX
```
