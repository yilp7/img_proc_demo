#include <QtTest>
#include <QSignalSpy>
#include <QWidget>
#include <QPushButton>

#include "widgets/floatingpanel.h"

class TestFloatingPanel : public QObject
{
    Q_OBJECT

private slots:
    // TC-FP-001: clampToParent keeps panel fully inside parent rect.
    void clamp_inside_parent()
    {
        const QSize parent(800, 600);
        const QSize panel(200, 150);

        QCOMPARE(FloatingPanel::clampToParent(QPoint(100, 100), panel, parent), QPoint(100, 100));
        QCOMPARE(FloatingPanel::clampToParent(QPoint(-50, 100), panel, parent), QPoint(0, 100));
        QCOMPARE(FloatingPanel::clampToParent(QPoint(100, -50), panel, parent), QPoint(100, 0));
        QCOMPARE(FloatingPanel::clampToParent(QPoint(9999, 100), panel, parent),
                 QPoint(parent.width() - panel.width(), 100));
        QCOMPARE(FloatingPanel::clampToParent(QPoint(100, 9999), panel, parent),
                 QPoint(100, parent.height() - panel.height()));
    }

    // TC-FP-002: clampToParent handles degenerate parent (smaller than panel)
    // by pinning the panel to the origin on that axis.
    void clamp_parent_smaller_than_panel()
    {
        const QSize parent(100, 100);
        const QSize panel(200, 200);
        QCOMPARE(FloatingPanel::clampToParent(QPoint(50, 50), panel, parent), QPoint(0, 0));
    }

    // TC-FP-003: drag moves panel by press-move-release and stays inside parent.
    void drag_moves_panel()
    {
        QWidget parent;
        parent.resize(800, 600);

        FloatingPanel *panel = new FloatingPanel(&parent);
        panel->resize(200, 150);
        panel->move(100, 100);
        parent.show();
        QVERIFY(QTest::qWaitForWindowExposed(&parent));

        QSignalSpy spy(panel, &FloatingPanel::dragged);

        // Press on the header (y=10 is inside the 28px header).
        const QPoint press_pt(50, 10);
        QTest::mousePress(panel, Qt::LeftButton, Qt::NoModifier, press_pt);

        // Send a move event in global coords so the panel's handler can use it.
        const QPoint target_global = parent.mapToGlobal(QPoint(250, 210));
        QMouseEvent move_ev(QEvent::MouseMove,
                            QPoint(press_pt.x(), press_pt.y()),   // localPos (unused by us)
                            target_global,                        // globalPos
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(panel, &move_ev);

        QTest::mouseRelease(panel, Qt::LeftButton, Qt::NoModifier, press_pt);

        // Expected panel origin: target_global (in parent coords) - press_offset.
        const QPoint expected(250 - press_pt.x(), 210 - press_pt.y());
        QCOMPARE(panel->pos(), expected);
        QVERIFY(spy.count() >= 1);
    }

    // TC-FP-004: pin disables drag.
    void pin_disables_drag()
    {
        QWidget parent;
        parent.resize(800, 600);

        FloatingPanel *panel = new FloatingPanel(&parent);
        panel->resize(200, 150);
        panel->move(100, 100);
        panel->setPinned(true);
        parent.show();
        QVERIFY(QTest::qWaitForWindowExposed(&parent));

        const QPoint start = panel->pos();
        const QPoint press_pt(50, 10);
        QTest::mousePress(panel, Qt::LeftButton, Qt::NoModifier, press_pt);
        QMouseEvent move_ev(QEvent::MouseMove,
                            QPoint(press_pt.x(), press_pt.y()),
                            parent.mapToGlobal(QPoint(250, 210)),
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(panel, &move_ev);
        QTest::mouseRelease(panel, Qt::LeftButton, Qt::NoModifier, press_pt);

        QCOMPARE(panel->pos(), start);
    }

    // TC-FP-005: hide button emits hideRequested() and hides the widget.
    void hide_emits_signal()
    {
        QWidget parent;
        parent.resize(800, 600);

        FloatingPanel *panel = new FloatingPanel(&parent);
        panel->resize(200, 150);
        parent.show();
        QVERIFY(QTest::qWaitForWindowExposed(&parent));

        QSignalSpy spy(panel, &FloatingPanel::hideRequested);

        QPushButton *hide_btn = panel->findChild<QPushButton*>(QStringLiteral("FloatingPanelHide"));
        QVERIFY(hide_btn != nullptr);
        QTest::mouseClick(hide_btn, Qt::LeftButton);

        QCOMPARE(spy.count(), 1);
        QVERIFY(panel->isHidden());
    }

    // TC-FP-006: pin button toggles pin state and emits pinToggled.
    void pin_toggle_signal()
    {
        FloatingPanel panel;
        QSignalSpy spy(&panel, &FloatingPanel::pinToggled);

        QPushButton *pin_btn = panel.findChild<QPushButton*>(QStringLiteral("FloatingPanelPin"));
        QVERIFY(pin_btn != nullptr);

        QTest::mouseClick(pin_btn, Qt::LeftButton);
        QCOMPARE(panel.isPinned(), true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), true);

        QTest::mouseClick(pin_btn, Qt::LeftButton);
        QCOMPARE(panel.isPinned(), false);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.at(1).at(0).toBool(), false);
    }

    // TC-FP-007: clicking the panel raises its z-order above a sibling.
    void click_raises_z_order()
    {
        QWidget parent;
        parent.resize(800, 600);

        FloatingPanel *a = new FloatingPanel(&parent);
        FloatingPanel *b = new FloatingPanel(&parent);
        a->resize(200, 150); a->move(50, 50);
        b->resize(200, 150); b->move(120, 80);   // overlaps a
        parent.show();
        QVERIFY(QTest::qWaitForWindowExposed(&parent));

        // b was added after a, so b starts on top. Click a (in a non-overlap
        // region) and verify a comes to the front.
        QTest::mouseClick(a, Qt::LeftButton, Qt::NoModifier, QPoint(10, 40));

        const QList<QWidget*> children = parent.findChildren<QWidget*>(QString(),
                                                                       Qt::FindDirectChildrenOnly);
        // Last child in the list is topmost in the painter's stacking order.
        int idx_a = children.indexOf(a);
        int idx_b = children.indexOf(b);
        QVERIFY(idx_a > idx_b);
    }
};

QTEST_MAIN(TestFloatingPanel)
#include "test_floatingpanel.moc"
