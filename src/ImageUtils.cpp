#include "ImageUtils.h"

namespace ImageUtils
{

QImage cvMatToQImage(const cv::Mat &mat)
{
    if (mat.empty()) {
        return QImage();
    }

    if (mat.type() == CV_8UC1) {
        QImage image(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8);
        return image.copy();
    }

    if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, DI_CV_COLOR_BGR2RGB);
        QImage image(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);
        return image.copy();
    }

    if (mat.type() == CV_8UC4) {
        cv::Mat rgba;
        cv::cvtColor(mat, rgba, DI_CV_COLOR_BGRA2RGBA);
        QImage image(rgba.data, rgba.cols, rgba.rows, static_cast<int>(rgba.step), QImage::Format_RGBA8888);
        return image.copy();
    }

    cv::Mat normalized;
    cv::normalize(mat, normalized, 0, 255, DI_CV_NORM_MINMAX);
    normalized.convertTo(normalized, CV_8U);
    QImage image(normalized.data, normalized.cols, normalized.rows, static_cast<int>(normalized.step), QImage::Format_Grayscale8);
    return image.copy();
}

cv::Mat qImageToCvMat(const QImage &image)
{
    if (image.isNull()) {
        return cv::Mat();
    }

    QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);
    cv::Mat rgb(rgbImage.height(),
                rgbImage.width(),
                CV_8UC3,
                const_cast<uchar *>(rgbImage.bits()),
                static_cast<size_t>(rgbImage.bytesPerLine()));
    cv::Mat bgr;
    cv::cvtColor(rgb, bgr, DI_CV_COLOR_RGB2BGR);
    return bgr.clone();
}

}
