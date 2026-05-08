#include "mapboxwidget.h"

#include <QLabel>
#include <QVBoxLayout>

MapboxWidget::MapboxWidget(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("MAPBOX_WIDGET");
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);

    auto *label = new QLabel("Map view — pending integration", this);
    label->setObjectName("MAPBOX_PLACEHOLDER");
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    l->addWidget(label);
}
