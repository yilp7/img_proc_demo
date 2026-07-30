#include "util.h"

QFont monaco, consolas, sans_heiti;

QTextCodec *locale_codec = QTextCodec::codecForLocale();

// 0: dark color mode, 1: light color mode
uchar app_color_mode = 0;

QByteArray stylesheet_dark, stylesheet_light;

QCursor cursor_dark_pointer, cursor_dark_resize_h, cursor_dark_resize_v, cursor_dark_resize_md, cursor_dark_resize_sd;
QCursor cursor_light_pointer, cursor_light_resize_h, cursor_light_resize_v, cursor_light_resize_md, cursor_light_resize_sd;
QCursor cursor_curr_pointer, cursor_curr_resize_h, cursor_curr_resize_v, cursor_curr_resize_md, cursor_curr_resize_sd;

QMutex cout_mutex;
