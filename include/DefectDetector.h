#ifndef DEFECTDETECTOR_H
#define DEFECTDETECTOR_H

#include "DetectionTypes.h"
#include "OpenCvCompat.h"

#include <QString>

class DefectDetector
{
public:
    DetectionResult detectFile(const QString &imagePath, const DetectorParams &params) const;
    DetectionResult detectMat(const cv::Mat &input, const QString &sourceName, const DetectorParams &params) const;

private:
    static int normalizedOddKernel(int value);
    static QString classifyDefect(double area, const cv::Rect &rect, double aspectRatio, const DetectorParams &params);
};

#endif
