#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include "OpenCvCompat.h"

#include <QImage>

namespace ImageUtils
{
QImage cvMatToQImage(const cv::Mat &mat);
cv::Mat qImageToCvMat(const QImage &image);
}

#endif
