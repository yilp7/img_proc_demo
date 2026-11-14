#include <QtTest>
#include <QSignalSpy>

#include "widgets/arcsliderwidget.h"

class TestArcSliderWidget : public QObject
{
    Q_OBJECT

private slots:
    // TC-AS-001: value to pixel and back across the range, within one step of
    // quantization (integer value, integer pixel).
    void value_pixel_roundtrip()
    {
        const int h = 1000;
        const int lo = 0;
        const int hi = 100;
        for (int v = lo; v <= hi; ++v) {
            const int px = ArcSliderWidget::value_to_pixel(v, h, lo, hi);
            const int rt = ArcSliderWidget::pixel_to_value(px, h, lo, hi);
            QCOMPARE(rt, v);
        }
    }

    // TC-AS-002: top pixel = max; bottom pixel = min.
    void top_is_max_bottom_is_min()
    {
        QCOMPARE(ArcSliderWidget::pixel_to_value(0,   100, 0, 50), 50);
        QCOMPARE(ArcSliderWidget::pixel_to_value(100, 100, 0, 50), 0);
    }

    // TC-AS-003: pixel out of track bounds clamps.
    void pixel_out_of_bounds_clamps()
    {
        QCOMPARE(ArcSliderWidget::pixel_to_value(-50,  100, 0, 50), 50);
        QCOMPARE(ArcSliderWidget::pixel_to_value(5000, 100, 0, 50), 0);
    }

    // TC-AS-004: degenerate inputs return minimum / zero, no div-by-zero.
    void pixel_degenerate()
    {
        QCOMPARE(ArcSliderWidget::pixel_to_value(10, 0, 0, 100), 0);
        QCOMPARE(ArcSliderWidget::value_to_pixel(10, 0, 0, 100), 0);
        QCOMPARE(ArcSliderWidget::pixel_to_value(10, 100, 5, 5), 5);
        QCOMPARE(ArcSliderWidget::value_to_pixel(10, 100, 5, 5), 0);
    }

    // TC-AS-005: set_value below min clamps to min.
    void set_value_clamps_below_min()
    {
        ArcSliderWidget w;
        w.set_range(0, 100);
        w.set_value(-50);
        QCOMPARE(w.get_value(), 0);
    }

    // TC-AS-006: set_value above max clamps to max.
    void set_value_clamps_above_max()
    {
        ArcSliderWidget w;
        w.set_range(0, 100);
        w.set_value(9999);
        QCOMPARE(w.get_value(), 100);
    }

    // TC-AS-007: set_value emits value_changed only when the stored value changes.
    void set_value_signal_only_on_change()
    {
        ArcSliderWidget w;
        w.set_range(0, 100);
        QSignalSpy spy(&w, &ArcSliderWidget::value_changed);

        w.set_value(42);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 42);

        w.set_value(42);
        QCOMPARE(spy.count(), 1);

        // Out-of-range request that resolves to current value: no emit.
        w.set_value(99999);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.at(1).at(0).toInt(), 100);
        w.set_value(99999);
        QCOMPARE(spy.count(), 2);
    }

    // TC-AS-008: set_range that excludes the current value re-clamps and emits.
    void set_range_reclamps_value()
    {
        ArcSliderWidget w;
        w.set_range(0, 100);
        w.set_value(80);
        QSignalSpy spy(&w, &ArcSliderWidget::value_changed);

        w.set_range(0, 50);
        QCOMPARE(w.get_value(), 50);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 50);
    }
};

QTEST_MAIN(TestArcSliderWidget)
#include "test_arcsliderwidget.moc"
