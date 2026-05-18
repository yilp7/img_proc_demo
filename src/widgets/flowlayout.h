#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QLayout>
#include <QList>
#include <QRect>
#include <QStyle>

// Wrapping horizontal layout: lays children left-to-right at their sizeHint
// widths, wrapping to a new line when the next item would exceed the
// container width. Adapted from the standard Qt FlowLayout example.
//
// Used for the SETUP overlay's Image Processing card so the 6 sub-groups
// (ACCUMULATIVE / ADAPTIVE / DCP / IMAGE_3D / FISHNET / ECC_GRP) tile
// horizontally and pop to the next row when the total fixed widths exceed
// the card's available width.
class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1);
    explicit FlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1);
    ~FlowLayout() override;

    void addItem(QLayoutItem *item) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;
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
    int doLayout(const QRect &rect, bool testOnly) const;
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem *> m_items;
    int m_h_space;
    int m_v_space;
};

#endif // FLOWLAYOUT_H
