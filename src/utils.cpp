#include "utils.h"

QFont monaco, consolas, yozai;

QTextCodec *locale_codec = QTextCodec::codecForLocale();

// 0: dark theme, 1: light theme
uchar app_theme = 0;

QByteArray theme_dark, theme_light;

QCursor cursor_dark_pointer, cursor_dark_resize_h, cursor_dark_resize_v, cursor_dark_resize_md, cursor_dark_resize_sd;
QCursor cursor_light_pointer, cursor_light_resize_h, cursor_light_resize_v, cursor_light_resize_md, cursor_light_resize_sd;
QCursor cursor_curr_pointer, cursor_curr_resize_h, cursor_curr_resize_v, cursor_curr_resize_md, cursor_curr_resize_sd;
