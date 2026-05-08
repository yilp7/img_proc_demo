#ifndef RIGHTSTACKCONTAINER_H
#define RIGHTSTACKCONTAINER_H

#include <QFrame>

class QVBoxLayout;

// Vertical stack of right-side widgets (IMG PROC / Save Options / Vision).
// Subclasses are added section-by-section in 17.2 / 17.3 / 17.4. The container
// itself is dumb — a thin QFrame wrapper around a QVBoxLayout with a default
// content margin and spacing convention that all right-side widgets share.
class RightStackContainer : public QFrame
{
    Q_OBJECT
public:
    explicit RightStackContainer(QWidget *parent = nullptr);

    // Append a section to the stack. stretch=0 means fixed-height; >0 means
    // the section grows to fill remaining vertical space.
    void addSection(QWidget *w, int stretch = 0);

    QVBoxLayout *contentLayout() const { return m_layout; }

private:
    QVBoxLayout *m_layout;
};

#endif // RIGHTSTACKCONTAINER_H
