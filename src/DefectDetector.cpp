#include "DefectDetector.h"

#include "ImageUtils.h"

#include <QElapsedTimer>
#include <QFile>

#include <algorithm>
#include <string>

namespace
{
std::string defectLabelForImage(const QString &defectType)
{
    if (defectType == QStringLiteral("划痕")) {
        return "Scratch";
    }
    if (defectType == QStringLiteral("缺口")) {
        return "Notch";
    }
    if (defectType == QStringLiteral("污点")) {
        return "Stain";
    }
    return "Defect";
}
}

DetectionResult DefectDetector::detectFile(const QString &imagePath, const DetectorParams &params) const
{
    DetectionResult result;
    result.sourcePath = imagePath;
    result.detectionTime = QDateTime::currentDateTime();

    const QByteArray imageFileName = QFile::encodeName(imagePath);
    const cv::Mat input = cv::imread(imageFileName.constData(), DI_CV_IMREAD_COLOR);
    if (input.empty()) {
        result.success = false;
        result.errorMessage = QStringLiteral("图片读取失败：%1").arg(imagePath);
        return result;
    }

    return detectMat(input, imagePath, params);
}

DetectionResult DefectDetector::detectMat(const cv::Mat &input, const QString &sourceName, const DetectorParams &params) const
{
    DetectionResult result;
    result.sourcePath = sourceName;
    result.detectionTime = QDateTime::currentDateTime();

    if (input.empty()) {
        result.success = false;
        result.errorMessage = QStringLiteral("输入图像为空");
        return result;
    }

    QElapsedTimer timer;
    timer.start();

    cv::Mat color;
    if (input.channels() == 1) {
        cv::cvtColor(input, color, DI_CV_COLOR_GRAY2BGR);
    } else {
        color = input.clone();
    }

    cv::Mat gray;
    cv::cvtColor(color, gray, DI_CV_COLOR_BGR2GRAY);

    const int blurKernel = normalizedOddKernel(params.blurKernel);
    if (blurKernel > 1) {
        cv::GaussianBlur(gray, gray, cv::Size(blurKernel, blurKernel), 0);
    }

    cv::Mat inspectImage = gray;
    bool usingTemplate = false;
    if (!params.templatePath.trimmed().isEmpty()) {
        const QByteArray templateFileName = QFile::encodeName(params.templatePath);
        cv::Mat templ = cv::imread(templateFileName.constData(), DI_CV_IMREAD_GRAYSCALE);
        if (!templ.empty()) {
            if (templ.size() != gray.size()) {
                cv::resize(templ, templ, gray.size());
            }
            if (blurKernel > 1) {
                cv::GaussianBlur(templ, templ, cv::Size(blurKernel, blurKernel), 0);
            }
            cv::absdiff(gray, templ, inspectImage);
            usingTemplate = true;
        }
    }

    cv::Mat binary;
    int thresholdType = DI_CV_THRESH_BINARY;
    if (!usingTemplate && params.invertThreshold) {
        thresholdType = DI_CV_THRESH_BINARY_INV;
    }
    if (params.useOtsu) {
        thresholdType |= DI_CV_THRESH_OTSU;
    }
    cv::threshold(inspectImage, binary, params.thresholdValue, 255, thresholdType);

    const int morphKernel = normalizedOddKernel(params.morphKernel);
    if (morphKernel > 1) {
        cv::Mat kernel = cv::getStructuringElement(DI_CV_MORPH_RECT, cv::Size(morphKernel, morphKernel));
        cv::morphologyEx(binary, binary, DI_CV_MORPH_OPEN, kernel);
        cv::morphologyEx(binary, binary, DI_CV_MORPH_CLOSE, kernel);
    }

    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(binary, contours, DI_CV_RETR_EXTERNAL, DI_CV_CHAIN_APPROX_SIMPLE);

    cv::Mat annotated = color.clone();
    for (const std::vector<cv::Point> &contour : contours) {
        const double area = cv::contourArea(contour);
        if (area < params.minArea) {
            continue;
        }

        const cv::Rect rect = cv::boundingRect(contour);
        if (rect.width < params.minWidth || rect.height < params.minHeight) {
            continue;
        }

        const double aspectRatio = static_cast<double>(std::max(rect.width, rect.height)) /
                                   static_cast<double>(std::max(1, std::min(rect.width, rect.height)));
        if (aspectRatio > params.maxAspectRatio) {
            continue;
        }

        DefectInfo defect;
        defect.boundingBox = QRect(rect.x, rect.y, rect.width, rect.height);
        defect.area = area;
        defect.aspectRatio = aspectRatio;
        defect.defectType = classifyDefect(area, rect, aspectRatio, params);
        result.defects.push_back(defect);

        cv::rectangle(annotated, rect, cv::Scalar(0, 0, 255), 2);
        const std::string label = defectLabelForImage(defect.defectType);
        int baseline = 0;
        cv::Size textSize = cv::getTextSize(label, DI_CV_FONT_HERSHEY_SIMPLEX, 0.45, 1, &baseline);
        int textY = std::max(0, rect.y - textSize.height - 4);
        cv::rectangle(annotated,
                      cv::Rect(rect.x, textY, textSize.width + 8, textSize.height + baseline + 6),
                      cv::Scalar(0, 0, 255),
                      -1);
        cv::putText(annotated,
                    label,
                    cv::Point(rect.x + 4, textY + textSize.height + 1),
                    DI_CV_FONT_HERSHEY_SIMPLEX,
                    0.45,
                    cv::Scalar(255, 255, 255),
                    1);
    }

    result.defectCount = result.defects.size();
    result.conclusion = result.defectCount > 0 ? QStringLiteral("NG") : QStringLiteral("OK");
    result.originalImage = ImageUtils::cvMatToQImage(color);
    result.resultImage = ImageUtils::cvMatToQImage(annotated);
    result.maskImage = ImageUtils::cvMatToQImage(binary);
    result.elapsedMs = static_cast<double>(timer.elapsed());
    result.success = true;
    return result;
}

int DefectDetector::normalizedOddKernel(int value)
{
    if (value <= 1) {
        return 1;
    }
    if (value % 2 == 0) {
        ++value;
    }
    return value;
}

QString DefectDetector::classifyDefect(double area, const cv::Rect &rect, double aspectRatio, const DetectorParams &params)
{
    const double fillRatio = area / static_cast<double>(std::max(1, rect.width * rect.height));

    if (aspectRatio >= params.scratchAspectRatio) {
        return QStringLiteral("划痕");
    }

    if (rect.width >= params.minWidth * 4 && rect.height >= params.minHeight * 4 && fillRatio < 0.45) {
        return QStringLiteral("缺口");
    }

    if (area >= params.minArea * 6.0 && fillRatio > 0.55) {
        return QStringLiteral("污点");
    }

    return QStringLiteral("异常");
}
