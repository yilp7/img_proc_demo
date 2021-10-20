#include "imageproc.h"
#include "opencv2/highgui.hpp"

ImageProc::ImageProc()
{

}

void ImageProc::plateau_equl_hist(cv::Mat *in, cv::Mat *out, int method) {
    int h = in->rows, w = in->cols;
    uint hist[256] = {0}, hist_ori[256] = {0}, af_hist[256] = {0}, nonzero_hist[256] = {0};
    int i, j, sum = 0, size = 0;
    uchar eq_hist[256] = {0}, EI_hist[256] = {0};
    uint nonzero_size = 0;
    uint t_up = 10000, t_down = 200;

    // calc # pixels of grayscale in[i][j]
    uchar *img = in->data;
    int step = in->step;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++)  hist_ori[(img + i * step)[j]]++;
    }

    for (i = 1; i < 255; i++) hist[i] = get_median(hist_ori + i - 1, 3); // to-do
    hist[0] = hist_ori[0];
    hist[255] = hist_ori[255];

    for (i = 0; i < 256; i++)  if (hist[i]) nonzero_hist[nonzero_size++] = hist[i];

    get_threshold(nonzero_hist, nonzero_size, t_up, t_down);

    // update size
    for (i = 0; i < 256; i++) {
        if      (hist[i] > t_up)              size += t_up;
        else if (hist[i] && hist[i] < t_down) size += t_down;
        else                                  size += hist[i];
    }

    // map new greyscale
    static float scale;
    for (i = 0; i < 256; i++) {
        // weight on level i, choose between (hist_sz-1.f) and (uzero_sz-1.f)
        scale = 255.0 / (size - hist[i]);

        if      (hist[i] > t_up)              sum += t_up;
        else if (hist[i] && hist[i] < t_down) sum += t_down;
        else                                  sum += hist[i];

        eq_hist[i] = cv::saturate_cast<uchar>(sum * scale);
    }

    // initialize new image
//    static cv::Mat temp(h, w, in->type());
//    uchar *img_temp = temp.data;
//    for (i = 0; i < h; i++) {
//        for (j = 0; j < w; j++) (img_temp + i * step)[j] = eq_hist[(img + i * step)[j]];
//    }

//    for (i = 0; i < h; i++) {
//        for (j = 0; j < w; j++) af_hist[(img_temp + i * step)[j]]++;
//    }
    for (i = 0; i < 256; i++) af_hist[eq_hist[i]] = hist[i];

    equal_interval(af_hist, EI_hist, method);

    uchar *res = out->data;
    for (i = 0; i < h; i++) {
//        for (j = 0; j < w; j++) (res + i * step)[j] = EI_hist[(img_temp + i * step)[j]];
        for (j = 0; j < w; j++) (res + i * step)[j] = EI_hist[eq_hist[(img + i * step)[j]]];
    }
}

inline int ImageProc::get_median(uint *arr, int len) {
    if (len != 3) return -1;
    if (arr[0] < arr[1]) return arr[1] > arr[2] ? (arr[0] > arr[2] ? arr[0] : arr[2]) : arr[1];
    else                 return arr[0] > arr[2] ? (arr[1] > arr[2] ? arr[1] : arr[2]) : arr[0];
}

void ImageProc::get_threshold(uint *nonzero_hist, int nonzero_size, uint &up, uint &down) {
    int m, local_size = 0, sum = 0;
    int f_hist[256] = {0}, f_prev, f_curr, f_next;

    f_curr = nonzero_hist[1] - nonzero_hist[0];
    f_next = nonzero_hist[2] - nonzero_hist[1];
    sum += nonzero_hist[0];
    local_size++;
    for (m = 2; m < nonzero_size - 1; m++) {
        // find local maximum
        f_prev = f_curr;
        f_curr = f_next;
        f_next = nonzero_hist[m+1] - nonzero_hist[m];

        if (f_prev > 0 && f_next < 0 && abs(f_prev) > abs(f_curr) && abs(f_next) > abs(f_curr)) {
            f_hist[local_size++] = nonzero_hist[m];
            sum += nonzero_hist[m];
        }
    }

    up = sum / local_size;
    down = up / sqrt(local_size);
}

void ImageProc::equal_interval(uint *eq_hist, uchar *EI_hist, int num) {
    int t = 0;

    for (int k = 0; k < 255; k++) {
        if (!eq_hist[k]) continue;
        if (t++ <= 1) continue;

        switch (num) {
        // 1. circle
        case 0:
            EI_hist[k] = sqrt(255 * 255 - sqrt(255 - k));          break;
        // 2. square root
        case 1:
            EI_hist[k] = sqrt(255 * k);                            break;
        // 3. 4th root
        case 2:
            EI_hist[k] = sqrt(255 * sqrt(255 * k));                break;
        // 4. 8th root
        case 3:
            EI_hist[k] = sqrt(255 * sqrt(255 * sqrt(255 * k)));    break;
        // 5. x = 2y; x = 2 * (y - 64) / 3 + 127
        case 4:
            EI_hist[k] = k < 64 ? 2 * k : 127 + (k - 64) * 2 / 3;  break;
        // 6. x = 4y; x = 4 * (y - 32) / 7 + 127
        case 5:
            EI_hist[k] = k < 32 ? 4 * k : 127 + (k - 32) * 4 / 7;  break;
        // 7. x = 8y; x = 8 * (y - 16) / 15 + 127
        case 6:
            EI_hist[k] = k < 16 ? 8 * k : 127 + (k - 16) * 8 / 15; break;
        default: break;
        }
    }
    EI_hist[255] = 255;
}

cv::Mat ImageProc::gated3D(cv::Mat &img1, cv::Mat &img2, double delay, double gw, double range_thresh)
{
    const int BARWIDTH = 104, BARHEIGHT = img1.rows;
    const int IMAGEWIDTH = img1.cols, IMAGEHEIGHT = img1.rows;
    cv::Mat gray_bar(BARHEIGHT, BARWIDTH, CV_8UC1);
    cv::Mat color_bar(BARHEIGHT, BARWIDTH, CV_8UC3);
    static double *range = (double*)calloc(IMAGEWIDTH * IMAGEHEIGHT, sizeof(double));

    cv::Mat img_3d(IMAGEHEIGHT, IMAGEWIDTH, CV_8UC3);

    const double c = 3e8 / 1.33;
    double max = 0, min = 10255;
    int threshold = 0;
    int i, j, idx1, idx2;
    int step = img1.step, step_3d = img_3d.step;
    int step_gray = gray_bar.step, step_color = color_bar.step;
    double R = delay * 1e-9 * c / 2;
    uchar *ptr1, *ptr2;

    threshold = cv::sum(img1)[0] / img1.total();
    img_3d = cv::Scalar(0, 0, 0);

    ptr1 = img1.data;
    ptr2 = img2.data;
    for (i = 0; i < IMAGEHEIGHT; i++) {
        idx1 = i * step;
        for (j = 0; j < IMAGEWIDTH; j++) {
            uchar c1 = ptr1[idx1], c2 = ptr2[idx1];
            if (c1 < 250 && c1 > range_thresh && c2 < 250 && c2 > range_thresh) {
                range[idx1] = R + gw * 1e-9 * c / 2 / ((double)c1 / c2 + 1);

                if (range[idx1] > max) max = range[idx1];
                if (range[idx1] < min) min = range[idx1];
            }
            else {
                range[idx1] = 0;
            }
            idx1++;
        }
    }

    for (i = 0; i < IMAGEHEIGHT; i++) {
        idx1 = i * step;
        for (j = 0; j < IMAGEWIDTH; j++) {
            if (range[idx1]) range[idx1] = (range[idx1] - min) / (max - min) * 255;
            idx1++;
        }
    }

    ptr1 = img_3d.data;
    for (i = 0; i < IMAGEHEIGHT; i++) {
        idx1 = i * step_3d, idx2 = i * step;
        for (j = 0; j < IMAGEWIDTH; j++) {
            uchar c0 = range[idx2++];
            if (c0 <= range_thresh) {
                ptr1[idx1++] = 0;
                ptr1[idx1++] = 0;
                ptr1[idx1++] = 0;
            }
            else if (c0 <= 64) {
                ptr1[idx1++] = 255;
                ptr1[idx1++] = 4 * c0;
                ptr1[idx1++] = 0;
            }
            else if(c0 <= 128) {
                ptr1[idx1++] = 4 * (128 - c0);
                ptr1[idx1++] = 255;
                ptr1[idx1++] = 0;
            }
            else if(c0 <= 192) {
                ptr1[idx1++] = 0;
                ptr1[idx1++] = 255;
                ptr1[idx1++] = 4 * (c0 - 128);
            }
            else if(c0 <= 256) {
                ptr1[idx1++] = 0;
                ptr1[idx1++] = 4 * (256 - c0);
                ptr1[idx1++] = 255;
            }
        }
    }

    int bar_gray = 255;
    ptr1 = gray_bar.data;
    int gap = (BARHEIGHT - 1024) / 2;
    for (i = 0; i < gap; i++) for (j = 0; j < BARWIDTH; j++) ptr1[i * step_gray + j] = 255;
    for (i = gap; i < BARHEIGHT - gap; i += 4) {
        for (j = 0; j < BARWIDTH; j++) {
            ptr1[i     * step_gray + j] = bar_gray;
            ptr1[(i+1) * step_gray + j] = bar_gray;
            ptr1[(i+2) * step_gray + j] = bar_gray;
            ptr1[(i+3) * step_gray + j] = bar_gray;
        }
        bar_gray--;
    }
    for (i = BARHEIGHT - gap; i < BARHEIGHT; i++) for (j = 0; j < BARWIDTH; j++) ptr1[i * step_gray + j] = 1;

    ptr1 = color_bar.data, ptr2 = gray_bar.data;
    for(i = 0; i < BARHEIGHT; i++) {
        idx1 = i * step_color, idx2 = i * step_gray;
        for(j = 0; j < BARWIDTH; j++) {
            uchar c0 = ptr2[idx2];
            if(c0 == 0) {
                ptr1[idx1++] = 255;
                ptr1[idx1++] = 4 * ptr2[idx2++];
                ptr1[idx1++] = 0;
            }
            else if(c0 <= 64) {
                ptr1[idx1++] = 255;
                ptr1[idx1++] = 4 * ptr2[idx2++] - 1;
                ptr1[idx1++] = 0;
            }
            else if(c0 <= 128) {
                ptr1[idx1++] = 4 * (128 - ptr2[idx2++]);
                ptr1[idx1++] = 255;
                ptr1[idx1++] = 0;
            }
            else if(c0 <= 192) {
                ptr1[idx1++] = 0;
                ptr1[idx1++] = 255;
                ptr1[idx1++] = 4 * (ptr2[idx2++] - 128) - 1;
            }
            else if(c0 <= 256) {
                ptr1[idx1++] = 0;
                ptr1[idx1++] = 4 * (256 - ptr2[idx2++]);
                ptr1[idx1++] = 255;
            }
        }
    }
    cv::hconcat(img_3d, color_bar, img_3d);

    cv::putText(img_3d, QString::asprintf("%.2f", min).toLatin1().data(), cv::Point(IMAGEWIDTH, 20), cv::FONT_HERSHEY_SIMPLEX, 0.77, cv::Scalar(0, 0, 0));
    cv::putText(img_3d, QString::asprintf("%.2f", (max + min) / 2).toLatin1().data(), cv::Point(IMAGEWIDTH, IMAGEHEIGHT / 2 - 15), cv::FONT_HERSHEY_SIMPLEX, 0.77, cv::Scalar(0, 0, 0));
    cv::putText(img_3d, QString::asprintf("%.2f", max).toLatin1().data(), cv::Point(IMAGEWIDTH, IMAGEHEIGHT - 10), cv::FONT_HERSHEY_SIMPLEX, 0.77, cv::Scalar(0, 0, 0));

    return img_3d;
}

void ImageProc::adaptive_enhance(cv::Mat *in, cv::Mat *out, double low_in, double high_in, double low_out, double high_out, double gamma)
{
    double low = low_in * 255, high = high_in * 255;
    double bottom = low_out * 255, top = high_out * 255;
    double err_in = high - low, err_out = top - bottom;

    // intensity transform
    for(int y = 0; y < in->rows; y++) {
        for (int x = 0; x < in->cols; x++) {
            out->data[x + y * out->step] = cv::saturate_cast<uchar>(pow((in->data[x + y * in->step] - low) / err_in, gamma) * err_out + bottom);
        }
    }
}
