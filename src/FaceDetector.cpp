// FaceDetector.cpp
// 人脸检测模块实现文件。
// 负责加载 haarcascade_frontalface_default.xml 模型，
// 将输入图片转成灰度图并调用 OpenCV detectMultiScale 检测人脸，
// 最后返回所有检测到的人脸矩形框。