#ifndef RIGHTSTACKCONTAINER_H
#define RIGHTSTACKCONTAINER_H

#include <QFrame>

class QVBoxLayout;

// Vertical stack of the right-side widgets: a thin QFrame around a QVBoxLayout
// with the margins and spacing they share.
class RightStackContainer : public QFrame
{
    Q_OBJECT
public:
    explicit RightStackContainer(QWidget *parent = nullptr);

    // Append a section to the stack. stretch=0 means fixed-height; >0 means
    // the section grows to fill remaining vertical space.
    void add_section(QWidget *w, int stretch = 0);

    QVBoxLayout *get_content_layout() const { return m_layout; }

private:
    QVBoxLayout *m_layout;
};

#endif // RIGHTSTACKCONTAINER_H
