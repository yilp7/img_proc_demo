#ifndef MAPBOXWIDGET_H
#define MAPBOXWIDGET_H

#include <QFrame>

class QLabel;

// Placeholder for the bottom-left map view. The actual Mapbox / map
// integration is deferred — this widget just renders a "pending
// integration" label so the layout reservation is visible during
// Sprint 17 development.
class MapboxWidget : public QFrame
{
    Q_OBJECT
public:
    explicit MapboxWidget(QWidget *parent = nullptr);
};

#endif // MAPBOXWIDGET_H
