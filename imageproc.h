#ifndef IMAGEPROC_H
#define IMAGEPROC_H

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include <QString>

class ImageProc
{
public:
    ImageProc();
    static void plateau_equl_hist(cv::Mat *in, cv::Mat *out, int method);
    static cv::Mat gated3D(cv::Mat &img1, cv::Mat &img2, double delay, double gw, double range_thresh);

private:
    static int get_median(uint *arr, int len);
    static void get_threshold(uint *nonzero_hist, int nonzero_size, uint &up, uint &down);
    static void equal_interval(uint *eq_hist, uchar *EI_hist, int num);

};

#endif // IMAGEPROC_H
