#ifndef TITLEBAR_H
#define TITLEBAR_H

#include "util/util.h"

class Preferences;
class ScanConfig;

class InfoLabel : public QLabel
{
    Q_OBJECT
public:
    explicit InfoLabel(QWidget *parent);

};

class TitleButton : public QPushButton
{
    Q_OBJECT
public:
    explicit TitleButton(QString icon, QWidget *parent);

protected:
    void mouseMoveEvent(QMouseEvent *event);

};

class TitleBar : public QFrame
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = 0);
    void setup(QObject *ptr);

public slots:
    void process_maximize();

protected:
    // @override
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

public:
    InfoLabel*   icon;
    InfoLabel*   title;
    TitleButton* url;
    TitleButton* settings;
    TitleButton* capture;
    TitleButton* cls;
    TitleButton* lang;
    TitleButton* theme;
    TitleButton* min;
    TitleButton* max;
    TitleButton* exit;

    bool         is_maximized;
    QPoint       prev_pos;
    QRect        normal_stat;

    bool         pressed;

    QObject      *signal_receiver;

//    ProgSettings *prog_settings;
    Preferences  *preferences;
    ScanConfig   *scan_config;

};

#endif // TITLEBAR_H
