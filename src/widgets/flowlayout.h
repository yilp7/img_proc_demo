#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QLayout>
#include <QList>
#include <QRect>
#include <QStyle>

// Wrapping horizontal layout: children keep their sizeHint width and wrap to a new
// line when the row is full. Adapted from the Qt FlowLayout example; used for the
// image processing card of the setup overlay.
class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent, int margin = -1, int h_spacing = -1, int v_spacing = -1);
    explicit FlowLayout(int margin = -1, int h_spacing = -1, int v_spacing = -1);
    ~FlowLayout() override;

    void addItem(QLayoutItem *item) override;
    int horizontal_spacing() const;
    int vertical_spacing() const;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;

private:
    int do_layout(const QRect &rect, bool test_only) const;
    int smart_spacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem *> m_items;
    int m_h_space;
    int m_v_space;
};

#endif // FLOWLAYOUT_H
