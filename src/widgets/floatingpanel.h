#ifndef FLOATINGPANEL_H
#define FLOATINGPANEL_H

#include <QWidget>
#include <QPoint>

class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QMouseEvent;

// Base class for draggable / pinnable / hideable panels that float inside a
// parent widget (OpticsPanel, TCUPanel, ConsolePanel subclass this).
//
// Transparency: enabled via QSS `background-color: rgba(...)` on the
// FloatingPanel class. Avoids Qt::WA_TranslucentBackground so behavior is the
// same across platforms. If transparency proves unreliable at integration
// time, callers can paint an opaque background via QSS without changing this
// class — transparency is a nice-to-have, not a contract.
class FloatingPanel : public QWidget
{
    Q_OBJECT
public:
    explicit FloatingPanel(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    QString title() const;

    void setPinned(bool pinned);
    bool isPinned() const { return m_pinned; }

    // Subclasses add child widgets into this layout; the header is never
    // inside it, so subclasses cannot accidentally break the drag surface.
    QVBoxLayout* contentLayout() const { return m_content_layout; }

    // Clamp helper exposed for unit testing the drag math in isolation.
    static QPoint clampToParent(const QPoint &desiredTopLeft,
                                const QSize &panelSize,
                                const QSize &parentSize);

signals:
    void pinToggled(bool pinned);
    void hideRequested();
    void dragged(QPoint newTopLeft);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onPinClicked();
    void onHideClicked();

private:
    QWidget      *m_header;
    QLabel       *m_title_label;
    QPushButton  *m_pin_button;
    QPushButton  *m_hide_button;
    QVBoxLayout  *m_root_layout;
    QVBoxLayout  *m_content_layout;

    bool   m_pinned;
    bool   m_dragging;
    QPoint m_press_offset;  // offset from panel top-left to press point
};

#endif // FLOATINGPANEL_H
