# Face-Recognition-System-Based-on-OpenCV

SCU 2025 级计算机科学与技术实训项目

## 项目简介

本项目是一个基于 Qt Widgets、OpenCV 和 SQLite 的桌面端人脸识别系统。系统提供人员信息管理、图片人脸识别、摄像头实时检测与识别、人员列表展示和识别日志记录等功能，适合作为计算机视觉与桌面应用开发的综合实训项目。

当前版本优先使用自训练的 YOLO11 ONNX 人脸检测模型，并在模型缺失或加载失败时自动回退到 OpenCV Haar 级联分类器。检测到的人脸会被裁剪并转换为 `64x64` 灰度图特征向量，再通过余弦相似度与数据库中已注册人员的人脸特征进行比对。该方案便于展示从模型训练、导出到 C++ 桌面端部署的完整流程。

## 已实现功能

- 人员信息管理：以学号作为唯一标识，提供管理界面，支持按姓名、学号、部门查询人员，并新增、编辑、删除人员信息。
- 人脸注册：新增人员时可以批量导入本地照片，也可以通过摄像头连续拍摄多张照片；编辑人员时也可以追加新的人脸照片。
- 多样本特征：系统会逐张检测人脸并提取特征，照片超过一张时会额外保存平均中心特征，提高识别鲁棒性。
- 图片识别：导入本地图片，检测图片中的人脸并显示识别结果。
- 摄像头识别：调用默认摄像头，实时检测画面中的人脸并进行身份识别。
- 人脸框绘制：在图片或视频帧上绘制检测框和识别结果。
- 人员列表展示：在主界面展示已注册人员信息。
- 识别日志记录：保存识别时间、识别姓名、相似度和来源路径。
- 本地数据持久化：使用 SQLite 保存人员信息、人脸特征和识别日志。

## 技术栈

- 编程语言：C++17
- 图形界面：Qt 5 Widgets
- 计算机视觉：OpenCV
- 数据存储：SQLite，基于 Qt SQL 模块访问
- 构建工具：CMake
- 人脸检测模型：自训练 YOLO11 ONNX，Haar 级联分类器作为回退方案

## 项目结构

```text
.
├── CMakeLists.txt
├── README.md
├── environment.md
├── resources
│   ├── haarcascade_frontalface_default.xml
│   └── models
│       └── face_detector_yolo.onnx (本地部署模型，不提交至 Git)
└── src
    ├── main.cpp
    ├── config
    │   └── AppConfig.h / AppConfig.cpp
    ├── ui
    │   ├── MainWindow.h / MainWindow.cpp
    │   ├── PersonManagementDialog.h / PersonManagementDialog.cpp
    │   └── RegisterDialog.h / RegisterDialog.cpp
    ├── utils
    │   └── ImageUtils.h / ImageUtils.cpp
    ├── vision
    │   ├── FaceDetector.h / FaceDetector.cpp
    │   └── FeatureExtractor.h / FeatureExtractor.cpp
    ├── service
    │   └── RecognitionService.h / RecognitionService.cpp
    └── storage
        └── FaceRepository.h / FaceRepository.cpp
```

核心模块职责：

- `config/AppConfig`：集中管理 YOLO/Haar 模型路径、YOLO 推理参数、数据库路径、识别阈值、摄像头刷新间隔、检测频率和日志写入间隔。
- `ui/MainWindow`：主窗口界面、按钮事件、摄像头控制和结果展示。
- `ui/PersonManagementDialog`：人员信息管理界面，支持查询、新增、编辑和删除人员。
- `ui/RegisterDialog`：人员信息编辑弹窗，收集姓名、学号、部门，并支持批量导入照片和摄像头拍照。
- `utils/ImageUtils`：负责图片读取、`cv::Mat` 到 `QImage` 的转换，以及人脸区域裁剪。
- `vision/FaceDetector`：优先加载 YOLO ONNX 模型，执行 letterbox 预处理、置信度过滤、NMS 和坐标还原；YOLO 不可用时自动回退 Haar。
- `vision/FeatureExtractor`：把裁剪后的人脸图片转换为固定长度的浮点特征向量。
- `service/RecognitionService`：计算余弦相似度，根据阈值判断是否匹配已注册人员。
- `storage/FaceRepository`：负责 SQLite 数据库连接、建表、人员信息、特征和日志读写，并保证学号唯一。

## 环境依赖

当前项目开发与测试环境如下：

- Windows 10/11
- CMake 4.3.4
- Qt 5.14.2 MinGW 64-bit
- OpenCV 构建与应用编译器：`D:\vscode\mingw64\bin`（GCC 15）
- OpenCV 5.0.0 MinGW build：`opencv5-deps/install/x64/mingw/lib`
- 旧版 Haar 回退环境：OpenCV 4.5.5 MinGW build

> 当前项目中，Haar 回退检测器可使用 OpenCV 4.5.5；但自训练的 YOLO11 ONNX 已验证需要 OpenCV `5.0.0` 或更新版本的 DNN 模块。OpenCV 5 已移除旧版 Haar `CascadeClassifier` API，因此升级后系统只使用 YOLO；若 YOLO 加载或推理失败，会直接显示原因而不是回退到 Haar。

`CMakeLists.txt` 中保留了上述本地默认路径。若本机 Qt 或 OpenCV 安装路径不同，可以在配置 CMake 时通过参数覆盖：

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="D:/QT/5.14.2/mingw73_64" `
  -DOpenCV_DIR="opencv5-deps/install/x64/mingw/lib" `
  -DCMAKE_C_COMPILER="D:/vscode/mingw64/bin/gcc.exe" `
  -DCMAKE_CXX_COMPILER="D:/vscode/mingw64/bin/g++.exe"
```

本项目内已构建 OpenCV 5 时，`OpenCV_DIR` 为 `opencv5-deps/install/x64/mingw/lib`。构建时 CMake 会把对应的 OpenCV DLL 自动复制到可执行文件目录，确保不会误用旧版 4.5.5 DLL。

## 构建与运行

在项目根目录执行：

```powershell
cmake -S . -B build-opencv5 -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="D:/QT/5.14.2/mingw73_64" `
  -DOpenCV_DIR="opencv5-deps/install/x64/mingw/lib" `
  -DCMAKE_C_COMPILER="D:/vscode/mingw64/bin/gcc.exe" `
  -DCMAKE_CXX_COMPILER="D:/vscode/mingw64/bin/g++.exe"
cmake --build build-opencv5
```

构建完成后运行：

```powershell
.\build-opencv5\FaceRecognitionSystem.exe
```

也可以使用 Qt Creator 打开 `CMakeLists.txt` 后直接配置、构建和运行。

构建后，CMake 会自动把 `resources` 目录复制到可执行文件所在目录。程序优先加载以下 YOLO 模型：

```text
<程序运行目录>/resources/models/face_detector_yolo.onnx
```

在 OpenCV 4.5.5 旧版构建中，YOLO 不可用时会加载以下 Haar 回退模型；OpenCV 5 构建不再包含此旧 API：

```text
<程序运行目录>/resources/haarcascade_frontalface_default.xml
```

程序首次启动时会自动创建数据目录和 SQLite 数据库：

```text
<程序运行目录>/data/face_recognition.db
```

## 使用流程

1. 启动程序，确认主界面提示数据库初始化成功。
2. 点击“人员管理”，进入人员信息管理界面。
3. 点击“新增”，填写姓名、学号和部门。
4. 通过“导入照片”选择一张或多张本地人脸照片，也可以点击“打开摄像头”后多次“拍照”采集不同角度或表情。
5. 确认新增后，系统会逐张检测人脸、提取特征；如果照片超过一张，会额外保存一条平均中心特征。
6. 在人员信息管理界面中，可以通过关键字查询人员，也可以选择人员后进行编辑或删除。
7. 关闭管理界面后，主界面的人员列表会刷新。
8. 点击“导入图片”，选择待识别图片。
9. 点击“开始识别”，系统会检测图片中的人脸并显示识别姓名和相似度。
10. 点击“开始摄像头”，系统会读取默认摄像头并进行实时检测和识别。
11. 识别记录会显示在右侧日志区域，并写入本地 SQLite 数据库。

## 核心实现说明

### 人脸检测

`FaceDetector` 会优先使用 OpenCV DNN 的 `cv::dnn::readNetFromONNX` 加载自训练 YOLO11 单类别人脸检测模型。输入图像会等比例缩放并补边到 `640x640`，模型输出经过置信度过滤和非极大值抑制（NMS）后，再映射回原图坐标。

默认参数位于 `AppConfig`：输入尺寸 `640`、置信度阈值 `0.35`、NMS 阈值 `0.45`、最多保留 `100` 张人脸。若 ONNX 文件缺失、无法加载或单帧推理出现异常，检测器会自动回退到 Haar；Haar 检测前会将图像灰度化并进行直方图均衡化。

摄像头模式将 ONNX 检测、特征提取和身份比对放入后台任务，界面线程只负责显示摄像头画面。上一个任务结束后，系统立即处理最新画面；同一时间只运行一个任务，避免 CPU 推理堆积导致界面卡顿。识别结果区域会显示该帧实际使用的检测器；显示 `YOLO ONNX` 表示训练模型已被使用，显示 Haar 则说明模型发生了回退，并会给出具体失败原因。

将训练产物部署到项目时，只需把导出的 `best.onnx` 复制为：

```text
resources/models/face_detector_yolo.onnx
```

该文件被 `.gitignore` 忽略，不会自动上传 GitHub。答辩或发布程序时，需要随可执行程序一并提供该模型文件。

当需要排查 ONNX 与本机 OpenCV DNN 的兼容性时，可以临时构建模型探测工具。它会加载模型并对一张空白图执行一次真实前向推理：

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DBUILD_YOLO_MODEL_PROBE=ON
cmake --build build --target YoloModelProbe
.\build\YoloModelProbe.exe .\resources\models\face_detector_yolo.onnx
```

### 特征提取

`FeatureExtractor` 将裁剪得到的人脸区域转换为灰度图，缩放到 `64x64`，再进行直方图均衡化和归一化，最终展开为长度为 `4096` 的浮点向量。

需要注意的是，当前特征不是 FaceNet、ArcFace 等深度学习模型生成的 embedding，而是一个便于课程实训理解和实现的基础图像特征。

新增人员或追加人脸照片时，系统会对每张照片分别检测最大人脸并提取特征；当同一人员一次录入多张照片时，会对这些特征按维度求平均，并把平均后的中心特征一并保存到数据库。

### 身份识别

`RecognitionService` 会将待识别人脸特征与数据库中所有已注册特征计算余弦相似度，并选择最高相似度作为候选结果。默认识别阈值在 `AppConfig` 中配置为 `0.80`：

- 最高相似度大于或等于 `0.80`：判定为对应注册人员。
- 最高相似度低于 `0.80`：判定为“陌生人”。

### 数据存储

`FaceRepository` 使用 SQLite 保存三类数据：

- `persons`：人员基本信息，包括学号、姓名、部门和创建时间；学号是主键，不再使用界面可见的自增 ID。
- `face_features`：人员对应的人脸特征，通过学号关联人员，样本特征和平均中心特征都以 JSON 数组形式保存。
- `recognition_logs`：识别日志，包括识别时间、人员姓名、相似度和图片来源。

删除人员时，SQLite 外键会级联删除其对应的人脸特征；识别日志保留历史姓名文本，不随人员删除而清空。

## 已完成的结构优化

- 按职责拆分源码目录，形成 `ui`、`vision`、`service`、`storage`、`config`、`utils` 等模块。
- 新增 `AppConfig`，集中管理模型路径、数据库路径、识别阈值和摄像头相关参数。
- 新增 `ImageUtils`，从 `MainWindow` 中拆出图片读取、图片格式转换和人脸裁剪逻辑。
- 新增 `PersonManagementDialog`，将原来的直接注册按钮升级为人员信息管理入口。
- 扩展 `RegisterDialog`，支持多张本地照片导入、摄像头预览和连续拍照。
- 扩展 `FaceRepository`，支持人员查询、更新和删除，并以学号作为人员唯一标识。
- 清理 CMake 配置，去掉重复的 Qt 查找语句，并支持通过 CMake 参数覆盖 Qt/OpenCV 路径。
- `MainWindow` 更聚焦于界面交互、摄像头控制和结果展示，底层工具逻辑由独立模块承担。

## 当前限制与后续优化

当前版本已经实现了完整的人脸注册、检测、识别和日志记录流程，但仍有进一步优化空间：

- 引入 FaceNet、ArcFace、DeepFace 等深度学习模型，替换当前基础灰度图特征，提高识别准确率和鲁棒性。
- 增加人脸照片质量评分、低质量照片过滤和更精细的样本管理。
- 增加单人详情页、日志清空、批量导入等更完整的数据管理功能。
- 继续拆分摄像头处理和识别流程协调逻辑，让 `MainWindow` 只负责 UI 展示和用户事件。
- 增加活体检测，降低照片或视频攻击带来的误识别风险。
- 当人员数量较多时，可引入 FAISS、Annoy 等近似最近邻检索库，提升大规模特征比对速度。
- 对摄像头画面增加目标跟踪或帧间平滑，减少人脸框闪烁。

## 验证建议

- 构建项目并确认程序可以正常启动。
- 确认 `resources/haarcascade_frontalface_default.xml` 被复制到可执行文件目录。
- 首次启动后确认 `data/face_recognition.db` 自动生成。
- 在人员管理界面新增、查询、编辑、删除人员后，确认人员列表刷新。
- 新增人员时导入多张照片或连续拍照，确认特征保存成功。
- 导入包含人脸的图片，确认可以绘制人脸框并输出识别结果。
- 启动摄像头后确认画面实时刷新，且识别日志可以正常写入。
