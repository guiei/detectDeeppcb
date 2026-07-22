#ifndef DETECTIONTYPES_H
#define DETECTIONTYPES_H

#include <QDateTime>
#include <QImage>
#include <QMetaType>
#include <QRect>
#include <QString>
#include <QVector>

struct DetectorParams
{
    int thresholdValue = 40;
    int blurKernel = 3;
    int morphKernel = 3;
    double minArea = 30.0;
    int minWidth = 3;
    int minHeight = 3;
    double scratchAspectRatio = 4.0;
    double maxAspectRatio = 25.0;
    bool invertThreshold = false;
    bool useOtsu = false;
    QString templatePath;
};

struct DefectInfo
{
    QRect boundingBox;
    double area = 0.0;
    double aspectRatio = 0.0;
    QString defectType;
};

struct DetectionResult
{
    bool success = false;
    QString errorMessage;
    QString sourcePath;
    QString conclusion;
    QDateTime detectionTime;
    int defectCount = 0;
    double elapsedMs = 0.0;
    QImage originalImage;
    QImage resultImage;
    QImage maskImage;
    QVector<DefectInfo> defects;
};

Q_DECLARE_METATYPE(DetectorParams)
Q_DECLARE_METATYPE(DefectInfo)
Q_DECLARE_METATYPE(DetectionResult)

#endif
