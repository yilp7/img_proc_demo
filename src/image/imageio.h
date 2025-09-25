#ifndef IMAGEIO_H
#define IMAGEIO_H

#include "util/util.h"

class ImageIO
{
public:
    ImageIO();

    static void save_image_bmp(cv::Mat img, QString filename);
    static void save_image_bmp(cv::Mat img, QString filename, QString note);  // Overload with note parameter
    static void save_image_tif(cv::Mat img, QString filename);
    static bool load_image_tif(cv::Mat &img, QString filename);
    static void save_image_jpg(cv::Mat img, QString filename);

    // Custom BMP note block functions
    static void append_note_to_bmp(QString filename, QString note);
    static QString read_note_from_bmp(QString filename);  // Optional: for reading notes back

};

#endif // IMAGEIO_H
