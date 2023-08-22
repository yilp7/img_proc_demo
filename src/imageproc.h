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

    // enhance
    static void hist_equalization(cv::Mat &src, cv::Mat &res);
    static void laplace_transform(cv::Mat &src, cv::Mat &res, cv::Mat kernel = cv::Mat());
    static void plateau_equl_hist(cv::Mat &src, cv::Mat &res, int method);
    static void accumulative_enhance(cv::Mat &src, cv::Mat &res, float accu_base);
    static void adaptive_enhance(cv::Mat &src, cv::Mat &res, double low_in, double high_in, double low_out, double high_out, double gamma);

    static void haze_removal(cv::Mat &src, cv::Mat &res, int radius, float omega, float t0, int guided_radius = 60, float eps = 0.01);
    static void brightness_and_contrast(cv::Mat & src, cv::Mat &res, float alpha, float beta);
    // TODO: check float implementation
    static void brightness_and_contrast(cv::Mat & src, cv::Mat &res, float gamma);

    // smooth
    static void guided_image_filter(cv::Mat &img, cv::Mat &guidance, int r, float eps, int fast = 3);

    // 3D
    static void gated3D(cv::Mat &src1, cv::Mat &src2, cv::Mat &res, double delay, double gw, double *range, double range_thresh);
    static void gated3D_v2(cv::Mat &src1, cv::Mat &src2, cv::Mat &res, double delay, double gw,
                           int colormap = cv::COLORMAP_PARULA, double lower_thresh = 0, double upper_thresh = 0.981,
                           bool trim = false, cv::Mat *dist_mat = nullptr, double *d_min = nullptr, double *d_max = nullptr);
    // TODO rewrite 3d painting function
    static void paint_3d(cv::Mat &src, cv::Mat &res, double lower_thresh, double upper_thresh, int colormap = cv::COLORMAP_PARULA);

    // split image 1x(2x2) -> 4x(1x1)
    static void split_img(cv::Mat &src, cv::Mat &res);

private:
    static int get_median(uint *arr, int len);
    static void get_threshold(uint *nonzero_hist, int nonzero_size, uint &up, uint &down);
    static void equal_interval(uint *eq_hist, uchar *EI_hist, int num);
    static std::vector<uchar> estimate_atmospheric_light_avg(cv::Mat &src, cv::Mat &dark);
    static std::vector<uchar> estimate_atmospheric_light_brightest(cv::Mat &src, cv::Mat &dark);
    static void dark_channel(cv::Mat &src, cv::Mat &dark, cv::Mat &inter, int r);

private:
    int height;
    int width;
    int depth;
    int channels;
};

#endif // IMAGEPROC_H
