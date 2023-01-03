#ifndef UTILS_H
#define UTILS_H

#include <QtCore>
#include <QtWidgets>
#include <QtSerialPort>
#include <QtNetwork>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/videoio.hpp>
#include "PixelType.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
}

extern QFont monaco, consolas;

extern QTextCodec *locale_codec;

extern uchar app_theme;

extern QByteArray theme_dark, theme_light;

extern QCursor cursor_dark_pointer, cursor_dark_resize_h, cursor_dark_resize_v, cursor_dark_resize_md, cursor_dark_resize_sd;
extern QCursor cursor_light_pointer, cursor_light_resize_h, cursor_light_resize_v, cursor_light_resize_md, cursor_light_resize_sd;
extern QCursor cursor_curr_pointer, cursor_curr_resize_h, cursor_curr_resize_v, cursor_curr_resize_md, cursor_curr_resize_sd;

#endif // UTILS_H
