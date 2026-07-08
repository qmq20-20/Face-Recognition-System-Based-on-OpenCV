#include "service/RecognitionService.h"

#include <cmath>

RecognitionService::RecognitionService(double threshold)
    : similarityThreshold(threshold)
{
}

RecognitionResult RecognitionService::recognize(
    const std::vector<float> &inputFeature,
    const QList<Person> &persons,
    const QList<FaceFeatureRecord> &knownFeatures) const
{
    RecognitionResult result;
    result.matched = false;
    result.studentId.clear();
    result.personName = "陌生人";
    result.similarity = 0.0;

    if (inputFeature.empty() || knownFeatures.isEmpty())
    {
        return result;
    }

    double bestSimilarity = -1.0;
    QString bestStudentId;

    for (const FaceFeatureRecord &record : knownFeatures)
    {
        double similarity = cosineSimilarity(inputFeature, record.feature);

        if (similarity > bestSimilarity)
        {
            bestSimilarity = similarity;
            bestStudentId = record.studentId;
        }
    }

    result.similarity = bestSimilarity;

    if (bestSimilarity >= similarityThreshold)
    {
        result.matched = true;
        result.studentId = bestStudentId;
        result.personName = findPersonNameByStudentId(persons, bestStudentId);
    }

    return result;
}

double RecognitionService::threshold() const
{
    return similarityThreshold;
}

void RecognitionService::setThreshold(double value)
{
    similarityThreshold = value;
}

double RecognitionService::cosineSimilarity(const std::vector<float> &a,
                                            const std::vector<float> &b) const
{
    if (a.empty() || b.empty() || a.size() != b.size())
    {
        return 0.0;
    }

    double dot = 0.0;
    double normA = 0.0;
    double normB = 0.0;

    for (size_t i = 0; i < a.size(); ++i)
    {
        dot += static_cast<double>(a[i]) * static_cast<double>(b[i]);
        normA += static_cast<double>(a[i]) * static_cast<double>(a[i]);
        normB += static_cast<double>(b[i]) * static_cast<double>(b[i]);
    }

    if (normA == 0.0 || normB == 0.0)
    {
        return 0.0;
    }

    return dot / (std::sqrt(normA) * std::sqrt(normB));
}

QString RecognitionService::findPersonNameByStudentId(const QList<Person> &persons, const QString &studentId) const
{
    for (const Person &person : persons)
    {
        if (person.studentId == studentId)
        {
            return person.name;
        }
    }

    return "未知人员";
}
