#ifndef RECOGNITIONSERVICE_H
#define RECOGNITIONSERVICE_H

// RecognitionService.h
// 人脸识别业务逻辑声明文件。
// 负责比较待识别人脸特征和数据库中的已知特征，返回最相似人员或“陌生人”。

#include "storage/FaceRepository.h"

#include <QString>

#include <vector>

struct RecognitionResult
{
    bool matched;
    int personId;
    QString personName;
    double similarity;
};

class RecognitionService
{
public:
    explicit RecognitionService(double threshold = 0.80);

    // 使用数据库中的人员和特征记录，对输入特征进行身份识别。
    RecognitionResult recognize(const std::vector<float> &inputFeature,
                                const QList<Person> &persons,
                                const QList<FaceFeatureRecord> &knownFeatures) const;

    double threshold() const;
    void setThreshold(double value);

private:
    double cosineSimilarity(const std::vector<float> &a,
                            const std::vector<float> &b) const;

    QString findPersonNameById(const QList<Person> &persons, int personId) const;

private:
    double similarityThreshold;
};

#endif
