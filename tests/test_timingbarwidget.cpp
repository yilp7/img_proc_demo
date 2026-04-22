#include <QtTest>
#include <QSignalSpy>

#include "widgets/timingbarwidget.h"

class TestTimingBarWidget : public QObject
{
    Q_OBJECT

private slots:
    // TC-TB-001: pixel↔value conversion round-trip.
    void pixel_value_roundtrip()
    {
        const int width = 1000;
        const quint64 scale = 10000;

        for (quint64 v : {quint64(0), quint64(100), quint64(5000), quint64(9990), scale}) {
            const int px = TimingBarWidget::valueToPixel(v, width, scale);
            const quint64 rt = TimingBarWidget::pixelToValue(px, width, scale);
            const qint64 diff = qint64(rt) - qint64(v);
            QVERIFY2(qAbs(diff) <= scale / width,
                     qPrintable(QString("v=%1 px=%2 rt=%3").arg(v).arg(px).arg(rt)));
        }
    }

    // TC-TB-002: degenerate inputs return safe values.
    void pixel_value_degenerate()
    {
        QCOMPARE(TimingBarWidget::valueToPixel(100, 0, 1000), 0);
        QCOMPARE(TimingBarWidget::pixelToValue(100, 0, 1000), quint64(0));
        QCOMPARE(TimingBarWidget::valueToPixel(100, 1000, 0), 0);
        QCOMPARE(TimingBarWidget::pixelToValue(100, 1000, 0), quint64(0));
    }

    // TC-TB-003: valueToPixel saturates at widthPx when value ≥ scale.
    void pixel_saturates_at_scale()
    {
        QCOMPARE(TimingBarWidget::valueToPixel(1000, 500, 1000), 500);
        QCOMPARE(TimingBarWidget::valueToPixel(9999, 500, 1000), 500);
    }

    // TC-TB-004: setBarValue clamps values that exceed the bar's scale.
    void setBarValue_clamps_to_scale()
    {
        TimingBarWidget w;
        w.setBarScale(TimingBarWidget::LaserWidth, 1000);
        w.setBarValue(TimingBarWidget::LaserWidth, 9999);
        QCOMPARE(w.barValue(TimingBarWidget::LaserWidth), quint64(1000));
    }

    // TC-TB-005: setBarValue emits barValueChanged only when the value
    // actually changes.
    void setBarValue_signal_only_on_change()
    {
        TimingBarWidget w;
        QSignalSpy spy(&w, &TimingBarWidget::barValueChanged);

        w.setBarValue(TimingBarWidget::Distance, 1234);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), int(TimingBarWidget::Distance));
        QCOMPARE(spy.at(0).at(1).toULongLong(), quint64(1234));

        w.setBarValue(TimingBarWidget::Distance, 1234);
        QCOMPARE(spy.count(), 1);
    }

    // TC-TB-006: changing one bar does not perturb the other three.
    void bars_are_independent()
    {
        TimingBarWidget w;
        w.setBarValue(TimingBarWidget::LaserWidth, 100);
        w.setBarValue(TimingBarWidget::Distance,   500);
        w.setBarValue(TimingBarWidget::Dov,        200);
        w.setBarValue(TimingBarWidget::Mcp,        128);

        QSignalSpy spy(&w, &TimingBarWidget::barValueChanged);
        w.setBarValue(TimingBarWidget::LaserWidth, 150);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(w.barValue(TimingBarWidget::Distance), quint64(500));
        QCOMPARE(w.barValue(TimingBarWidget::Dov),      quint64(200));
        QCOMPARE(w.barValue(TimingBarWidget::Mcp),      quint64(128));
    }

    // TC-TB-007: reducing a bar's scale clamps its current value and emits.
    void setBarScale_clamps_existing_value()
    {
        TimingBarWidget w;
        w.setBarScale(TimingBarWidget::Distance, 5000);
        w.setBarValue(TimingBarWidget::Distance, 4000);

        QSignalSpy spy(&w, &TimingBarWidget::barValueChanged);
        w.setBarScale(TimingBarWidget::Distance, 1000);

        QCOMPARE(w.barValue(TimingBarWidget::Distance), quint64(1000));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), int(TimingBarWidget::Distance));
    }

    // TC-TB-008: per-bar scale is independent — setting one bar's scale
    // does not affect others and does not emit for them.
    void scales_are_independent()
    {
        TimingBarWidget w;
        w.setBarScale(TimingBarWidget::LaserWidth, 1000);
        w.setBarScale(TimingBarWidget::Mcp,         255);

        QSignalSpy spy(&w, &TimingBarWidget::barValueChanged);
        w.setBarScale(TimingBarWidget::LaserWidth, 2000);

        QCOMPARE(w.barScale(TimingBarWidget::LaserWidth), quint64(2000));
        QCOMPARE(w.barScale(TimingBarWidget::Mcp),        quint64(255));
        QCOMPARE(spy.count(), 0); // no clamp needed, no emit
    }
};

QTEST_MAIN(TestTimingBarWidget)
#include "test_timingbarwidget.moc"
