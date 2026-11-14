#ifndef MAPBOXWIDGET_H
#define MAPBOXWIDGET_H

#include <QFrame>

class QLabel;

// Placeholder for the bottom-left map view: it only shows a label until a map is integrated.
class MapboxWidget : public QFrame
{
    Q_OBJECT
public:
    explicit MapboxWidget(QWidget *parent = nullptr);
};

#endif // MAPBOXWIDGET_H
