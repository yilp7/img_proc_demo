#ifndef IMAGEPROC_H
#define IMAGEPROC_H

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include <vector>
#include <algorithm>
#include <numeric>

class ImageProc
{
public:
    // use CV_8U, CV_16U only
    ImageProc();
    static void plateau_equl_hist(cv::Mat *in, cv::Mat *out, int method);
    static void gated3D(cv::Mat &src1, cv::Mat &src2, cv::Mat &res, double delay, double gw, double *range, double range_thresh);
    //TODO rewrite 3d painting function
    static void paint_3d(cv::Mat &src, cv::Mat &res, double range_thresh, double min, double max);
    static void accumulative_enhance(cv::Mat &src, cv::Mat &res, float accu_base);
    static void adaptive_enhance(cv::Mat &src, cv::Mat &res, double low_in, double high_in, double low_out, double high_out, double gamma);
    static void haze_removal(cv::Mat &src, cv::Mat &res, int radius, float omega, float t0, int guided_radius = 60, float eps = 0.01);
    static void brightness_and_contrast(cv::Mat & src, cv::Mat &res, float alpha, float beta);
    static void brightness_and_contrast(cv::Mat & src, cv::Mat &res, float gamma);

private:
    static int get_median(uint *arr, int len);
    static void get_threshold(uint *nonzero_hist, int nonzero_size, uint &up, uint &down);
    static void equal_interval(uint *eq_hist, uchar *EI_hist, int num);
    static std::vector<uchar> estimate_atmospheric_light_avg(cv::Mat &src, cv::Mat &dark);
    static std::vector<uchar> estimate_atmospheric_light_brightest(cv::Mat &src, cv::Mat &dark);
    static void dark_channel(cv::Mat &src, cv::Mat &dark, cv::Mat &inter, int r);
    static void guided_filter(cv::Mat &p, cv::Mat &I, int r, float eps, int fast);

};

#endif // IMAGEPROC_H
