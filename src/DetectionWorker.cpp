#include "DetectionWorker.h"

#include "ImageUtils.h"

DetectionWorker::DetectionWorker(QObject *parent)
    : QObject(parent)
{
}

void DetectionWorker::detectFile(const QString &imagePath, const DetectorParams &params)
{
    DefectDetector detector;
    emit detectionFinished(detector.detectFile(imagePath, params));
}

void DetectionWorker::detectImage(const QImage &image, const QString &sourceName, const DetectorParams &params)
{
    DefectDetector detector;
    emit detectionFinished(detector.detectMat(ImageUtils::qImageToCvMat(image), sourceName, params));
}
