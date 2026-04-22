#include <QtTest>
#include <QSignalSpy>

#include "widgets/unlockbar.h"

namespace {
// Drive the bar with synthesized press/move/release events to a target x
// (widget-local). Targets the middle of the thumb so the press hits it.
void dragTo(UnlockBar *w, int start_x, int target_x, int y)
{
    QTest::mousePress(w, Qt::LeftButton, Qt::NoModifier, QPoint(start_x, y));
    QMouseEvent move_ev(QEvent::MouseMove, QPoint(target_x, y),
                        w->mapToGlobal(QPoint(target_x, y)),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(w, &move_ev);
    QTest::mouseRelease(w, Qt::LeftButton, Qt::NoModifier, QPoint(target_x, y));
}
}

class TestUnlockBar : public QObject
{
    Q_OBJECT

private slots:
    // TC-UB-001: drag past threshold → unlocked() fires once.
    void drag_past_threshold_unlocks()
    {
        UnlockBar w;
        w.setRelockMs(0); // disable auto-relock for this test
        w.setUnlockThreshold(0.9);
        w.resize(400, 40);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));

        QSignalSpy spy(&w, &UnlockBar::unlocked);
        // Press inside thumb (near x=8, thumb starts at trackLeft=4), drag
        // to x=400 (far right).
        dragTo(&w, 20, 400, 20);

        QCOMPARE(spy.count(), 1);
        QVERIFY(w.isUnlocked());
    }

    // TC-UB-002: short drag → no signal, thumb springs back.
    void drag_short_springs_back()
    {
        UnlockBar w;
        w.setRelockMs(0);
        w.resize(400, 40);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));

        QSignalSpy unlocked_spy(&w, &UnlockBar::unlocked);
        // Drag to roughly 40% — well below the 90% threshold.
        dragTo(&w, 20, 160, 20);

        QCOMPARE(unlocked_spy.count(), 0);
        QVERIFY(!w.isUnlocked());
    }

    // TC-UB-003: auto-relock timer fires locked() and re-arms the bar.
    void auto_relock()
    {
        UnlockBar w;
        w.setRelockMs(50);
        w.resize(400, 40);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));

        QSignalSpy locked_spy(&w, &UnlockBar::locked);
        dragTo(&w, 20, 400, 20);
        QVERIFY(w.isUnlocked());

        QVERIFY(QTest::qWaitFor([&](){ return !w.isUnlocked(); }, 1000));
        QCOMPARE(locked_spy.count(), 1);
    }

    // TC-UB-004: repeated unlock gestures while already unlocked do not
    // re-emit unlocked().
    void unlocked_signal_single_fire()
    {
        UnlockBar w;
        w.setRelockMs(0);
        w.resize(400, 40);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));

        QSignalSpy spy(&w, &UnlockBar::unlocked);
        dragTo(&w, 20, 400, 20);
        QCOMPARE(spy.count(), 1);

        // Thumb is now at right end; pressing there and releasing at right
        // is still past threshold, but we're already unlocked.
        dragTo(&w, 380, 400, 20);
        QCOMPARE(spy.count(), 1);
    }

    // TC-UB-005: manual relock() slot emits locked() and cancels the pending
    // auto-relock timer.
    void manual_relock_slot()
    {
        UnlockBar w;
        w.setRelockMs(200); // long enough we can verify it doesn't fire twice
        w.resize(400, 40);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));

        QSignalSpy locked_spy(&w, &UnlockBar::locked);
        dragTo(&w, 20, 400, 20);
        QVERIFY(w.isUnlocked());

        w.relock();
        QVERIFY(!w.isUnlocked());
        QCOMPARE(locked_spy.count(), 1);

        // Wait longer than the auto-relock window to make sure the pending
        // timer did not fire a second locked() signal.
        QTest::qWait(300);
        QCOMPARE(locked_spy.count(), 1);
    }
};

QTEST_MAIN(TestUnlockBar)
#include "test_unlockbar.moc"
