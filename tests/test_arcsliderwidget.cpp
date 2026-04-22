#include <QtTest>
#include <QSignalSpy>

#include "widgets/arcsliderwidget.h"

class TestArcSliderWidget : public QObject
{
    Q_OBJECT

private slots:
    // TC-AS-001: value↔pixel round-trip across the range, within 1 step of
    // quantization error (integer value, integer pixel).
    void value_pixel_roundtrip()
    {
        const int h = 1000;
        const int lo = 0;
        const int hi = 100;
        for (int v = lo; v <= hi; ++v) {
            const int px = ArcSliderWidget::valueToPixel(v, h, lo, hi);
            const int rt = ArcSliderWidget::pixelToValue(px, h, lo, hi);
            QCOMPARE(rt, v);
        }
    }

    // TC-AS-002: top pixel = max; bottom pixel = min.
    void top_is_max_bottom_is_min()
    {
        QCOMPARE(ArcSliderWidget::pixelToValue(0,   100, 0, 50), 50);
        QCOMPARE(ArcSliderWidget::pixelToValue(100, 100, 0, 50), 0);
    }

    // TC-AS-003: pixel out of track bounds clamps.
    void pixel_out_of_bounds_clamps()
    {
        QCOMPARE(ArcSliderWidget::pixelToValue(-50,  100, 0, 50), 50);
        QCOMPARE(ArcSliderWidget::pixelToValue(5000, 100, 0, 50), 0);
    }

    // TC-AS-004: degenerate inputs return minimum / zero, no div-by-zero.
    void pixel_degenerate()
    {
        QCOMPARE(ArcSliderWidget::pixelToValue(10, 0, 0, 100), 0);
        QCOMPARE(ArcSliderWidget::valueToPixel(10, 0, 0, 100), 0);
        QCOMPARE(ArcSliderWidget::pixelToValue(10, 100, 5, 5), 5);
        QCOMPARE(ArcSliderWidget::valueToPixel(10, 100, 5, 5), 0);
    }

    // TC-AS-005: setValue below min clamps to min.
    void setValue_clamps_below_min()
    {
        ArcSliderWidget w;
        w.setRange(0, 100);
        w.setValue(-50);
        QCOMPARE(w.value(), 0);
    }

    // TC-AS-006: setValue above max clamps to max.
    void setValue_clamps_above_max()
    {
        ArcSliderWidget w;
        w.setRange(0, 100);
        w.setValue(9999);
        QCOMPARE(w.value(), 100);
    }

    // TC-AS-007: setValue emits valueChanged only when the stored value
    // actually changes.
    void setValue_signal_only_on_change()
    {
        ArcSliderWidget w;
        w.setRange(0, 100);
        QSignalSpy spy(&w, &ArcSliderWidget::valueChanged);

        w.setValue(42);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 42);

        w.setValue(42);
        QCOMPARE(spy.count(), 1);

        // Out-of-range request that resolves to current value: no emit.
        w.setValue(99999);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.at(1).at(0).toInt(), 100);
        w.setValue(99999);
        QCOMPARE(spy.count(), 2);
    }

    // TC-AS-008: setRange that excludes the current value re-clamps and emits.
    void setRange_reclamps_value()
    {
        ArcSliderWidget w;
        w.setRange(0, 100);
        w.setValue(80);
        QSignalSpy spy(&w, &ArcSliderWidget::valueChanged);

        w.setRange(0, 50);
        QCOMPARE(w.value(), 50);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 50);
    }
};

QTEST_MAIN(TestArcSliderWidget)
#include "test_arcsliderwidget.moc"
