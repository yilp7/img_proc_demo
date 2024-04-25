#ifndef IMAGEIO_H
#define IMAGEIO_H

#include "util/util.h"

class ImageIO
{
public:
    ImageIO();

    static void save_image_bmp(cv::Mat img, QString filename);
    static void save_image_tif(cv::Mat img, QString filename);
    static bool load_image_tif(cv::Mat &img, QString filename);
};

#endif // IMAGEIO_H
