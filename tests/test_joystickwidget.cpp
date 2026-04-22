#include <QtTest>
#include <QSignalSpy>

#include "widgets/joystickwidget.h"

namespace {
bool nearly(double a, double b, double eps = 1e-6)
{
    return qAbs(a - b) < eps;
}
}

class TestJoystickWidget : public QObject
{
    Q_OBJECT

private slots:
    // TC-JW-001: clampToCircle leaves vectors inside the radius alone.
    void clamp_passthrough()
    {
        const QPointF v(30.0, 40.0); // length 50
        const QPointF out = JoystickWidget::clampToCircle(v, 100.0);
        QCOMPARE(out, v);
    }

    // TC-JW-002: clampToCircle reduces vectors longer than the radius to the
    // radius on the same ray.
    void clamp_shrinks_to_radius()
    {
        const QPointF v(30.0, 40.0); // length 50
        const QPointF out = JoystickWidget::clampToCircle(v, 10.0);
        QVERIFY(nearly(std::sqrt(out.x() * out.x() + out.y() * out.y()), 10.0));
        // Direction preserved: out is parallel to v.
        QVERIFY(nearly(out.x() / v.x(), out.y() / v.y()));
    }

    // TC-JW-003: zero radius yields zero vector.
    void clamp_zero_radius()
    {
        QCOMPARE(JoystickWidget::clampToCircle(QPointF(5.0, 5.0), 0.0), QPointF(0.0, 0.0));
    }

    // TC-JW-004: stick at center reports (0, 0).
    void direction_center_is_zero()
    {
        const QPointF d = JoystickWidget::computeDirection(QPointF(50, 50),
                                                           QPointF(50, 50),
                                                           40.0, 0.1);
        QCOMPARE(d, QPointF(0.0, 0.0));
    }

    // TC-JW-005: stick at right edge reports (+1, 0) within tolerance.
    void direction_right_edge_is_plus_x()
    {
        const QPointF d = JoystickWidget::computeDirection(QPointF(90, 50),
                                                           QPointF(50, 50),
                                                           40.0, 0.1);
        QVERIFY(nearly(d.x(), 1.0));
        QVERIFY(nearly(d.y(), 0.0));
    }

    // TC-JW-006: stick dragged beyond the base radius saturates at magnitude 1.
    void direction_clamped_to_unit_magnitude()
    {
        const QPointF d = JoystickWidget::computeDirection(QPointF(500, 50),
                                                           QPointF(50, 50),
                                                           40.0, 0.1);
        const double mag = std::sqrt(d.x() * d.x() + d.y() * d.y());
        QVERIFY(mag <= 1.0 + 1e-9);
        QVERIFY(nearly(mag, 1.0));
    }

    // TC-JW-007: inside deadzone → (0, 0).
    void direction_deadzone_reports_zero()
    {
        // Radius 100, deadzone 0.1 → deadzone radius = 10 px.
        // Stick 5 px off center → inside deadzone.
        const QPointF d = JoystickWidget::computeDirection(QPointF(55, 50),
                                                           QPointF(50, 50),
                                                           100.0, 0.1);
        QCOMPARE(d, QPointF(0.0, 0.0));
    }

    // TC-JW-008: just outside deadzone → non-zero, continuous.
    void direction_just_outside_deadzone()
    {
        // Deadzone radius 10. Stick 11 px off center.
        const QPointF d = JoystickWidget::computeDirection(QPointF(61, 50),
                                                           QPointF(50, 50),
                                                           100.0, 0.1);
        QVERIFY(d.x() > 0.0);
        QVERIFY(nearly(d.y(), 0.0));
    }

    // TC-JW-009: release after drag snaps back to center and emits (0, 0).
    void release_snaps_center_and_emits_zero()
    {
        JoystickWidget w;
        w.resize(200, 200);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));

        QSignalSpy spy(&w, &JoystickWidget::directionChanged);

        // Drag from center to right edge.
        QTest::mousePress(&w, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
        QMouseEvent move_ev(QEvent::MouseMove, QPoint(180, 100),
                            w.mapToGlobal(QPoint(180, 100)),
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(&w, &move_ev);
        QTest::mouseRelease(&w, Qt::LeftButton, Qt::NoModifier, QPoint(180, 100));

        // The last emitted direction must be (0, 0) (snap-back).
        QVERIFY(spy.count() >= 1);
        const QList<QVariant> last = spy.last();
        QCOMPARE(last.at(0).toDouble(), 0.0);
        QCOMPARE(last.at(1).toDouble(), 0.0);
        QCOMPARE(w.direction(), QPointF(0.0, 0.0));
    }
};

QTEST_MAIN(TestJoystickWidget)
#include "test_joystickwidget.moc"
