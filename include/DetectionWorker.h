#ifndef DETECTIONWORKER_H
#define DETECTIONWORKER_H

#include "DetectionTypes.h"
#include "DefectDetector.h"

#include <QObject>
#include <QImage>

class DetectionWorker : public QObject
{
    Q_OBJECT

public:
    explicit DetectionWorker(QObject *parent = nullptr);

public slots:
    void detectFile(const QString &imagePath, const DetectorParams &params);
    void detectImage(const QImage &image, const QString &sourceName, const DetectorParams &params);

signals:
    void detectionFinished(const DetectionResult &result);
};

#endif
