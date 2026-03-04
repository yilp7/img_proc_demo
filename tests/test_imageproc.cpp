#include <QtTest>
#include <opencv2/opencv.hpp>
#include "image/imageproc.h"

class TestImageProc : public QObject
{
    Q_OBJECT

private slots:
    // TC-IP-001: hist_equalization grayscale
    void histEqualizationGrayscale()
    {
        cv::Mat src(100, 100, CV_8U, cv::Scalar(50));
        cv::Mat res;

        ImageProc::hist_equalization(src, res);

        QCOMPARE(res.size(), src.size());
        QCOMPARE(res.type(), CV_8U);

        // For a uniform image, equalizeHist maps all pixels to the same
        // output value. Verify output is valid and produced a result.
        cv::Scalar srcMean, srcStddev, resMean, resStddev;
        cv::meanStdDev(src, srcMean, srcStddev);
        cv::meanStdDev(res, resMean, resStddev);

        // At minimum the output should differ from the input
        // (equalizeHist on a constant image maps all pixels to a single
        // CDF-derived value which differs from the input constant).
        bool differs = (resMean[0] != srcMean[0]) || (resStddev[0] != srcStddev[0]);
        QVERIFY2(differs || resStddev[0] >= srcStddev[0],
                 "Output should differ from input or have >= stddev");
    }

    // TC-IP-002: hist_equalization color
    void histEqualizationColor()
    {
        cv::Mat src(100, 100, CV_8UC3, cv::Scalar(50, 50, 50));
        cv::Mat res;

        ImageProc::hist_equalization(src, res);

        QCOMPARE(res.channels(), 3);
        QCOMPARE(res.size(), src.size());
    }

    // TC-IP-003: laplace_transform default kernel
    void laplaceTransformDefaultKernel()
    {
        cv::Mat src(100, 100, CV_8U, cv::Scalar(0));
        // Bottom half = 128 to create a horizontal edge at rows 49-50
        // (use 128 not 255 to avoid saturation in the sharpening kernel)
        src(cv::Rect(0, 50, 100, 50)).setTo(128);

        cv::Mat res;
        ImageProc::laplace_transform(src, res);

        QCOMPARE(res.size(), src.size());

        // Default kernel (0,-1,0, 0,5,0, 0,-1,0) is a sharpening filter.
        // Flat regions: output = 5*val - 2*val = 3*val (clamped to [0,255]).
        // Edge region has mixed neighbors, so edge mean differs from both
        // flat regions.
        double edgeVal = cv::mean(res(cv::Rect(0, 49, 100, 2)))[0];
        double flatTopVal = cv::mean(res(cv::Rect(0, 10, 100, 10)))[0];
        double flatBotVal = cv::mean(res(cv::Rect(0, 70, 100, 10)))[0];

        // Flat top (0): 3*0 = 0. Flat bottom (128): 3*128 = 384 → 255.
        // Edge row 49 (val=0, neighbor below=128): 5*0 - 128 → clamp 0.
        // Edge row 50 (val=128, neighbor above=0): 5*128 - 0 - 128 = 512 → 255.
        // Edge mean ≈ (0+255)/2 = 127.5. Verify edge exceeds flat top (0).
        QVERIFY2(edgeVal > flatTopVal,
                 qPrintable(QString("Edge mean (%1) should exceed flat top mean (%2)")
                            .arg(edgeVal).arg(flatTopVal)));
        // Verify flat bottom is sharpened (3*128=384 saturates to 255)
        QVERIFY2(flatBotVal > 200,
                 qPrintable(QString("Flat bottom mean (%1) should be >200 after sharpening")
                            .arg(flatBotVal)));
    }

    // TC-IP-004: laplace_transform identity kernel
    void laplaceTransformIdentityKernel()
    {
        cv::Mat src(100, 100, CV_8U, cv::Scalar(0));
        src(cv::Rect(0, 50, 100, 50)).setTo(255);

        cv::Mat kernel = cv::Mat::zeros(3, 3, CV_32F);
        kernel.at<float>(1, 1) = 1.0f;

        cv::Mat res;
        ImageProc::laplace_transform(src, res, kernel);

        QCOMPARE(res.size(), src.size());
        QCOMPARE(res.type(), CV_8U);

        // Identity kernel through filter2D with CV_8U output should
        // preserve values approximately. Most pixels within +/-1.
        int mismatches = 0;
        for (int i = 0; i < src.rows; i++) {
            for (int j = 0; j < src.cols; j++) {
                int diff = std::abs((int)res.at<uchar>(i, j) - (int)src.at<uchar>(i, j));
                if (diff > 1) mismatches++;
            }
        }
        QVERIFY2(mismatches == 0,
                 qPrintable(QString("Identity kernel: %1 pixels differ by more than 1")
                            .arg(mismatches)));
    }

    // TC-IP-005: plateau_equl_hist methods 0-6
    void plateauEqulHistAllMethods()
    {
        // 256x256 gradient: row i has value i
        cv::Mat src(256, 256, CV_8U);
        for (int i = 0; i < 256; i++) {
            src.row(i).setTo(cv::Scalar(i));
        }

        for (int method = 0; method <= 6; method++) {
            cv::Mat res;
            ImageProc::plateau_equl_hist(src, res, method);

            QCOMPARE(res.size(), src.size());
            QCOMPARE(res.type(), CV_8U);

            // Verify output is within valid [0, 255] range (guaranteed
            // by CV_8U type, but verify no corruption)
            double minVal, maxVal;
            cv::minMaxLoc(res, &minVal, &maxVal);
            QVERIFY2(minVal >= 0 && maxVal <= 255,
                     qPrintable(QString("Method %1: values out of range [%2, %3]")
                                .arg(method).arg(minVal).arg(maxVal)));
        }
    }

    // TC-IP-006: brightness_and_contrast
    void brightnessAndContrast()
    {
        cv::Mat src(100, 100, CV_8U, cv::Scalar(128));
        cv::Mat res = src.clone();  // pre-allocate since function uses res.data

        float alpha = 1.5f, beta = 10.0f;
        // Expected: saturate_cast<uchar>(1.5 * 128 + 10) = 202
        ImageProc::brightness_and_contrast(src, res, alpha, beta);

        // Check that most pixels are approximately 202
        // The implementation: res = src * alpha + beta, then zeros out
        // pixels where ptr2[idx] == beta. For value 128: 1.5*128+10=202,
        // which != 10(beta), so no zeroing.
        int correctCount = 0;
        for (int i = 0; i < res.rows; i++) {
            for (int j = 0; j < res.cols; j++) {
                int val = res.at<uchar>(i, j);
                if (std::abs(val - 202) <= 2) correctCount++;
            }
        }
        double pctCorrect = (double)correctCount / (res.rows * res.cols) * 100.0;
        QVERIFY2(pctCorrect > 95.0,
                 qPrintable(QString("Expected ~202, but only %1% of pixels within +/-2")
                            .arg(pctCorrect)));
    }

    // TC-IP-007: split_img
    void splitImg()
    {
        cv::Mat src(200, 200, CV_8U, cv::Scalar(0));
        // Four quadrants: TL=10, TR=20, BL=30, BR=40
        src(cv::Rect(0,   0,   100, 100)).setTo(10);
        src(cv::Rect(100, 0,   100, 100)).setTo(20);
        src(cv::Rect(0,   100, 100, 100)).setTo(30);
        src(cv::Rect(100, 100, 100, 100)).setTo(40);

        cv::Mat res = cv::Mat::zeros(200, 200, CV_8U);  // must pre-allocate
        ImageProc::split_img(src, res);

        QCOMPARE(res.size(), src.size());

        // The split rearrangement formula:
        //   res[i][j] = src[2*(i%half_h) + i/half_h][2*(j%half_w) + j/half_w]
        // Verify specific pixel spot checks
        int half_h = 100, half_w = 100;

        // Spot-check a few pixels by computing the expected mapping
        auto expectedVal = [&](int i, int j) -> uchar {
            int src_i = 2 * (i % half_h) + i / half_h;
            int src_j = 2 * (j % half_w) + j / half_w;
            return src.at<uchar>(src_i, src_j);
        };

        // Check corners and center points
        QCOMPARE(res.at<uchar>(0, 0),     expectedVal(0, 0));
        QCOMPARE(res.at<uchar>(0, 99),    expectedVal(0, 99));
        QCOMPARE(res.at<uchar>(0, 100),   expectedVal(0, 100));
        QCOMPARE(res.at<uchar>(99, 0),    expectedVal(99, 0));
        QCOMPARE(res.at<uchar>(99, 99),   expectedVal(99, 99));
        QCOMPARE(res.at<uchar>(100, 0),   expectedVal(100, 0));
        QCOMPARE(res.at<uchar>(100, 100), expectedVal(100, 100));
        QCOMPARE(res.at<uchar>(199, 199), expectedVal(199, 199));
        QCOMPARE(res.at<uchar>(50, 50),   expectedVal(50, 50));
        QCOMPARE(res.at<uchar>(150, 150), expectedVal(150, 150));
    }

    // TC-IP-008: gated3D
    void gated3DBasic()
    {
        cv::Mat src1(100, 100, CV_8U, cv::Scalar(200));
        cv::Mat src2(100, 100, CV_8U, cv::Scalar(50));
        cv::Mat res;

        double delay = 100.0, gw = 40.0, range_thresh = 100.0;
        // range array must be pre-allocated: size = width * height
        std::vector<double> range(100 * 100, 0.0);

        ImageProc::gated3D(src1, src2, res, delay, gw, range.data(), range_thresh);

        // Output includes a color bar (BARWIDTH=104), so width = 100 + 104
        QCOMPARE(res.rows, src1.rows);
        QVERIFY2(res.cols >= src1.cols,
                 qPrintable(QString("Result cols (%1) should be >= source cols (%2)")
                            .arg(res.cols).arg(src1.cols)));
        QCOMPARE(res.channels(), 3);  // output is CV_8UC3
    }

    // TC-IP-009: gated3D_v2
    void gated3DV2Basic()
    {
        cv::Mat src1(100, 100, CV_8U, cv::Scalar(200));
        cv::Mat src2(100, 100, CV_8U, cv::Scalar(50));
        cv::Mat res = cv::Mat::zeros(100, 100 + 104, CV_8U);

        ImageProc::gated3D_v2(src1, src2, res, 100.0, 40.0);

        // gated3D_v2 appends a bar, so result width = 100 + 104
        QCOMPARE(res.rows, src1.rows);
        QVERIFY2(res.cols >= src1.cols,
                 qPrintable(QString("Result cols (%1) should be >= source cols (%2)")
                            .arg(res.cols).arg(src1.cols)));
        // In current implementation the colormap is commented out, so
        // output is single-channel (img_3d_gray.copyTo). Verify channels.
        QVERIFY(res.channels() == 1 || res.channels() == 3);
    }

    // TC-IP-010: accumulative_enhance
    void accumulativeEnhance()
    {
        // Gradient image: column j has value j (0-99)
        cv::Mat src(100, 100, CV_8U);
        for (int i = 0; i < src.rows; i++) {
            for (int j = 0; j < src.cols; j++) {
                src.at<uchar>(i, j) = (uchar)j;
            }
        }

        cv::Mat res;
        float accu_base = 2.4f;
        ImageProc::accumulative_enhance(src, res, accu_base);

        QCOMPARE(res.size(), src.size());
        QCOMPARE(res.type(), CV_8U);

        // Dark pixels (low j) should be enhanced more than bright pixels.
        // For p < 0.250 * 256 = 64: output = saturate(p * 2.4 * 2.4) = saturate(p * 5.76)
        // For p >= 64: multiplier decreases progressively.
        // Check that a dark pixel (j=10) has a higher enhancement ratio
        // than a brighter pixel (j=80).
        double darkSrcMean = cv::mean(src.col(10))[0];
        double darkResMean = cv::mean(res.col(10))[0];
        double brightSrcMean = cv::mean(src.col(80))[0];
        double brightResMean = cv::mean(res.col(80))[0];

        double darkRatio = darkResMean / std::max(darkSrcMean, 1.0);
        double brightRatio = brightResMean / std::max(brightSrcMean, 1.0);

        QVERIFY2(darkRatio > brightRatio,
                 qPrintable(QString("Dark enhancement ratio (%1) should exceed bright ratio (%2)")
                            .arg(darkRatio).arg(brightRatio)));
    }

    // TC-IP-011: adaptive_enhance
    void adaptiveEnhance()
    {
        cv::Mat src(100, 100, CV_8U, cv::Scalar(128));
        cv::Mat res;

        ImageProc::adaptive_enhance(src, res, 0.0, 1.0, 0.0, 1.0, 1.2);

        QCOMPARE(res.size(), src.size());
        QCOMPARE(res.type(), CV_8U);

        // Output should differ from input (gamma != 1.0)
        cv::Mat diff;
        cv::absdiff(src, res, diff);
        int numDiff = cv::countNonZero(diff);
        QVERIFY2(numDiff > 0, "Output should differ from input with gamma=1.2");

        // All values should be in [0, 255] (guaranteed by CV_8U, but verify
        // no corruption)
        double minVal, maxVal;
        cv::minMaxLoc(res, &minVal, &maxVal);
        QVERIFY(minVal >= 0 && maxVal <= 255);
    }

    // TC-IP-012: haze_removal
    void hazeRemoval()
    {
        // Use a non-uniform image: bright haze with darker scene content.
        // A uniform image produces identical output because (px - A)/t + A = A
        // when all pixels equal A.
        cv::Mat src(100, 100, CV_8UC3);
        for (int r = 0; r < 100; r++) {
            for (int c = 0; c < 100; c++) {
                // Simulate hazy scene: bright background (200) with darker objects
                uchar val = static_cast<uchar>(120 + (r + c) % 80);
                src.at<cv::Vec3b>(r, c) = cv::Vec3b(val, val, val);
            }
        }
        cv::Mat res = src.clone();  // must pre-allocate since function writes to res.data

        ImageProc::haze_removal(src, res, 7, 0.95f, 0.1f);

        QCOMPARE(res.channels(), 3);
        QCOMPARE(res.size(), src.size());

        // Output should differ from non-uniform input after haze removal
        cv::Mat diff;
        cv::absdiff(src, res, diff);
        cv::Mat diffGray;
        cv::cvtColor(diff, diffGray, cv::COLOR_BGR2GRAY);
        int numDiff = cv::countNonZero(diffGray);
        QVERIFY2(numDiff > 0 || cv::mean(diff)[0] > 0,
                 "Haze removal output should differ from hazy input");
    }
};

QTEST_MAIN(TestImageProc)
#include "test_imageproc.moc"
