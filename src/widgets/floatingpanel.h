#ifndef FLOATINGPANEL_H
#define FLOATINGPANEL_H

#include <QWidget>
#include <QPoint>

class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QMouseEvent;

// Base class of the draggable, pinnable, hideable panels that float inside a parent
// widget. Transparency comes from the stylesheet (rgba background), not from
// Qt::WA_TranslucentBackground, so it behaves the same on every platform.
class FloatingPanel : public QWidget
{
    Q_OBJECT
public:
    explicit FloatingPanel(QWidget *parent = nullptr);

    void set_title(const QString &title);
    QString get_title() const;

    void set_pinned(bool pinned);
    bool is_pinned() const { return m_pinned; }

    // Subclasses add child widgets into this layout; the header is never
    // inside it, so subclasses cannot accidentally break the drag surface.
    QVBoxLayout *get_content_layout() const { return m_content_layout; }

    // Clamp helper exposed for unit testing the drag math in isolation.
    static QPoint clamp_to_parent(const QPoint &desired_top_left,
                                  const QSize &panel_size,
                                  const QSize &parent_size);

signals:
    void pin_toggled(bool pinned);
    void hide_requested();
    void dragged(QPoint new_top_left);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void on_pin_clicked();
    void on_hide_clicked();

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
