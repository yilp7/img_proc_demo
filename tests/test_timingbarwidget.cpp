#include <QtTest>
#include <QSignalSpy>

#include "widgets/timingbarwidget.h"

class TestTimingBarWidget : public QObject
{
    Q_OBJECT

private slots:
    // TC-TB-001: pixel to value and back.
    void pixel_value_roundtrip()
    {
        const int width = 1000;
        const quint64 scale = 10000;

        for (quint64 v : {quint64(0), quint64(100), quint64(5000), quint64(9990), scale}) {
            const int px = TimingBarWidget::value_to_pixel(v, width, scale);
            const quint64 rt = TimingBarWidget::pixel_to_value(px, width, scale);
            const qint64 diff = qint64(rt) - qint64(v);
            QVERIFY2(qAbs(diff) <= scale / width,
                     qPrintable(QString("v=%1 px=%2 rt=%3").arg(v).arg(px).arg(rt)));
        }
    }

    // TC-TB-002: degenerate inputs return safe values.
    void pixel_value_degenerate()
    {
        QCOMPARE(TimingBarWidget::value_to_pixel(100, 0, 1000), 0);
        QCOMPARE(TimingBarWidget::pixel_to_value(100, 0, 1000), quint64(0));
        QCOMPARE(TimingBarWidget::value_to_pixel(100, 1000, 0), 0);
        QCOMPARE(TimingBarWidget::pixel_to_value(100, 1000, 0), quint64(0));
    }

    // TC-TB-003: value_to_pixel saturates at width_px when value >= scale.
    void pixel_saturates_at_scale()
    {
        QCOMPARE(TimingBarWidget::value_to_pixel(1000, 500, 1000), 500);
        QCOMPARE(TimingBarWidget::value_to_pixel(9999, 500, 1000), 500);
    }

    // TC-TB-004: set_bar_value clamps values that exceed the bar's scale.
    void set_bar_value_clamps_to_scale()
    {
        TimingBarWidget w;
        w.set_bar_scale(TimingBarWidget::LASER_WIDTH, 1000);
        w.set_bar_value(TimingBarWidget::LASER_WIDTH, 9999);
        QCOMPARE(w.get_bar_value(TimingBarWidget::LASER_WIDTH), quint64(1000));
    }

    // TC-TB-005: set_bar_value emits barValueChanged only when the value
    // actually changes.
    void set_bar_value_signal_only_on_change()
    {
        TimingBarWidget w;
        QSignalSpy spy(&w, &TimingBarWidget::bar_value_changed);

        w.set_bar_value(TimingBarWidget::DISTANCE, 1234);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), int(TimingBarWidget::DISTANCE));
        QCOMPARE(spy.at(0).at(1).toULongLong(), quint64(1234));

        w.set_bar_value(TimingBarWidget::DISTANCE, 1234);
        QCOMPARE(spy.count(), 1);
    }

    // TC-TB-006: changing one bar does not perturb the other three.
    void bars_are_independent()
    {
        TimingBarWidget w;
        w.set_bar_value(TimingBarWidget::LASER_WIDTH, 100);
        w.set_bar_value(TimingBarWidget::DISTANCE,    500);
        w.set_bar_value(TimingBarWidget::DOV,         200);
        w.set_bar_value(TimingBarWidget::MCP,         128);

        QSignalSpy spy(&w, &TimingBarWidget::bar_value_changed);
        w.set_bar_value(TimingBarWidget::LASER_WIDTH, 150);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(w.get_bar_value(TimingBarWidget::DISTANCE), quint64(500));
        QCOMPARE(w.get_bar_value(TimingBarWidget::DOV),      quint64(200));
        QCOMPARE(w.get_bar_value(TimingBarWidget::MCP),      quint64(128));
    }

    // TC-TB-007: reducing a bar's scale clamps its current value and emits.
    void set_bar_scale_clamps_existing_value()
    {
        TimingBarWidget w;
        w.set_bar_scale(TimingBarWidget::DISTANCE, 5000);
        w.set_bar_value(TimingBarWidget::DISTANCE, 4000);

        QSignalSpy spy(&w, &TimingBarWidget::bar_value_changed);
        w.set_bar_scale(TimingBarWidget::DISTANCE, 1000);

        QCOMPARE(w.get_bar_value(TimingBarWidget::DISTANCE), quint64(1000));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), int(TimingBarWidget::DISTANCE));
    }

    // TC-TB-008: the scale is per bar: setting one bar's scale does not affect the
    // others and emits nothing for them.
    void scales_are_independent()
    {
        TimingBarWidget w;
        w.set_bar_scale(TimingBarWidget::LASER_WIDTH, 1000);
        w.set_bar_scale(TimingBarWidget::MCP,         255);

        QSignalSpy spy(&w, &TimingBarWidget::bar_value_changed);
        w.set_bar_scale(TimingBarWidget::LASER_WIDTH, 2000);

        QCOMPARE(w.get_bar_scale(TimingBarWidget::LASER_WIDTH), quint64(2000));
        QCOMPARE(w.get_bar_scale(TimingBarWidget::MCP),         quint64(255));
        QCOMPARE(spy.count(), 0); // no clamp needed, no emit
    }
};

QTEST_MAIN(TestTimingBarWidget)
#include "test_timingbarwidget.moc"
