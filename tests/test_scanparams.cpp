#include <QtTest>
#include "pipeline/scanparams.h"

class TestScanParams : public QObject
{
    Q_OBJECT

private slots:
    // TC-SP-001: ScanParams default-constructed values match the documented
    // defaults inherited from the legacy ScanConfig() constructor (Sprint 22).
    void defaultValues()
    {
        ScanParams p;

        QCOMPARE(p.dist_ns,         3e8f / 2e9f);
        QCOMPARE(p.hz_unit,         0);
        QCOMPARE(p.base_unit,       0);
        QCOMPARE(p.starting_delay,  0);
        QCOMPARE(p.ending_delay,    0);
        QCOMPARE(p.starting_gw,     0);
        QCOMPARE(p.ending_gw,       0);
        QCOMPARE(p.step_size_delay, 1.0f);
        QCOMPARE(p.step_size_gw,    1.0f);
        QCOMPARE(p.frame_count,     1);
        QCOMPARE(p.rep_freq,        10.0f);
        QCOMPARE(p.scan_delay_enabled, true);
        QCOMPARE(p.scan_gate_width_enabled, true);
        QCOMPARE(p.scan_ptz_enabled, true);

        QCOMPARE(p.capture_scan_ori, true);
        QCOMPARE(p.capture_scan_res, false);
        QCOMPARE(p.record_scan_ori,  false);
        QCOMPARE(p.record_scan_res,  true);

        QCOMPARE(p.starting_h, 0.0f);
        QCOMPARE(p.ending_h,   0.0f);
        QCOMPARE(p.starting_v, 0.0f);
        QCOMPARE(p.ending_v,   0.0f);
        QCOMPARE(p.step_h,     0.0f);
        QCOMPARE(p.step_v,     0.0f);
        QCOMPARE(p.count_h,    1);
        QCOMPARE(p.count_v,    1);
        QCOMPARE(p.ptz_direction, (unsigned int)0);
        QCOMPARE(p.ptz_wait_time, 4);
        QCOMPARE(p.num_single_pos, 8);

        QCOMPARE(p.ptz_scan_mode, static_cast<int>(ScanParams::PTZ_SCAN_PANORAMA));
        QCOMPARE(p.panorama_start_h, 0.0f);
        QCOMPARE(p.panorama_fixed_v, 0.0f);
        QCOMPARE(p.panorama_step_h, 30.0f);
        QCOMPARE(p.panorama_count, 12);
        QCOMPARE(p.panorama_direction, (unsigned int)0);
        QCOMPARE(p.panorama_wait_time, 2);
        QCOMPARE(p.panorama_use_end_h, false);
        QCOMPARE(p.panorama_end_h, 0.0f);
        QCOMPARE(p.panorama_zoom_level, 1.0f);
        QCOMPARE(p.panorama_fov_h, 38.4f);
        QCOMPARE(p.panorama_fov_v, 22.5f);
        QCOMPARE(p.panorama_fusion_mode, static_cast<int>(ScanParams::PANORAMA_FUSION_GAIN));
        QCOMPARE(static_cast<int>(p.panorama_zoom_fov.size()), 2);
        QCOMPARE(p.panorama_zoom_fov[0].focal_length_mm, 10.0f);
        QCOMPARE(p.panorama_zoom_fov[1].focal_length_mm, 860.0f);
    }

    // TC-SP-010: get_tcu_route produces frame_count delay values starting
    // from starting_delay with constant step_size_delay.
    void tcuRouteBasic()
    {
        ScanParams p;
        p.starting_delay = 100;
        p.step_size_delay = 10.0f;
        p.frame_count = 5;

        auto route = get_tcu_route(p);
        QCOMPARE(route.size(), (size_t)5);
        QCOMPARE(route[0], 100.0f);
        QCOMPARE(route[1], 110.0f);
        QCOMPARE(route[2], 120.0f);
        QCOMPARE(route[3], 130.0f);
        QCOMPARE(route[4], 140.0f);
    }

    // TC-SP-011: Single-frame degenerate case returns just starting_delay.
    void tcuRouteSingleFrame()
    {
        ScanParams p;  // defaults: starting_delay=0, frame_count=1
        auto route = get_tcu_route(p);
        QCOMPARE(route.size(), (size_t)1);
        QCOMPARE(route[0], 0.0f);
    }

    // TC-SP-012: frame_count == 0 yields an empty route (loop never runs).
    void tcuRouteZeroFrames()
    {
        ScanParams p;
        p.frame_count = 0;
        auto route = get_tcu_route(p);
        QCOMPARE(route.size(), (size_t)0);
    }

    // TC-SP-013: Fractional step preserved as float (starting_delay is int
    // but the result vector is float).
    void tcuRouteFractionalStep()
    {
        ScanParams p;
        p.starting_delay = 0;
        p.step_size_delay = 2.5f;
        p.frame_count = 4;

        auto route = get_tcu_route(p);
        QCOMPARE(route.size(), (size_t)4);
        QCOMPARE(route[0], 0.0f);
        QCOMPARE(route[1], 2.5f);
        QCOMPARE(route[2], 5.0f);
        QCOMPARE(route[3], 7.5f);
    }

    void gateWidthRouteBasic()
    {
        ScanParams p;
        p.starting_gw = 50;
        p.step_size_gw = 2.5f;
        p.frame_count = 4;

        auto route = get_gate_width_route(p);
        QCOMPARE(route.size(), (size_t)4);
        QCOMPARE(route[0], 50.0f);
        QCOMPARE(route[1], 52.5f);
        QCOMPARE(route[2], 55.0f);
        QCOMPARE(route[3], 57.5f);
    }

    // TC-SP-020: get_ptz_route with default ScanParams (1×1 grid) yields a
    // single point at (starting_h, starting_v).
    void ptzRouteDefaultSinglePoint()
    {
        ScanParams p;  // count_h=1, count_v=1
        auto route = get_ptz_route(p);
        QCOMPARE(route.size(), (size_t)1);
        QCOMPARE(route[0].first,  0.0f);
        QCOMPARE(route[0].second, 0.0f);
    }

    // TC-SP-021: ptz_direction == 0 — outer loop H, inner loop V with
    // serpentine V (V descends on odd-i columns from ending_v).
    void ptzRouteDirectionZero()
    {
        ScanParams p;
        p.ptz_direction = 0;
        p.starting_h = 0;  p.ending_h = 10;  p.step_h = 10;  p.count_h = 2;
        p.starting_v = 0;  p.ending_v = 20;  p.step_v = 10;  p.count_v = 3;

        auto route = get_ptz_route(p);
        QCOMPARE(route.size(), (size_t)6);

        // i=0 (even): V ascends from starting_v
        QCOMPARE(route[0], std::make_pair( 0.0f,  0.0f));
        QCOMPARE(route[1], std::make_pair( 0.0f, 10.0f));
        QCOMPARE(route[2], std::make_pair( 0.0f, 20.0f));
        // i=1 (odd): V descends from ending_v
        QCOMPARE(route[3], std::make_pair(10.0f, 20.0f));
        QCOMPARE(route[4], std::make_pair(10.0f, 10.0f));
        QCOMPARE(route[5], std::make_pair(10.0f,  0.0f));
    }

    // TC-SP-022: ptz_direction != 0 — outer loop V, inner loop H with
    // serpentine H (H descends on odd-j rows from ending_h).
    void ptzRouteDirectionNonZero()
    {
        ScanParams p;
        p.ptz_direction = 1;
        p.starting_h = 0;  p.ending_h = 20;  p.step_h = 10;  p.count_h = 3;
        p.starting_v = 0;  p.ending_v = 10;  p.step_v = 10;  p.count_v = 2;

        auto route = get_ptz_route(p);
        QCOMPARE(route.size(), (size_t)6);

        // j=0 (even): H ascends from starting_h
        QCOMPARE(route[0], std::make_pair( 0.0f, 0.0f));
        QCOMPARE(route[1], std::make_pair(10.0f, 0.0f));
        QCOMPARE(route[2], std::make_pair(20.0f, 0.0f));
        // j=1 (odd): H descends from ending_h
        QCOMPARE(route[3], std::make_pair(20.0f, 10.0f));
        QCOMPARE(route[4], std::make_pair(10.0f, 10.0f));
        QCOMPARE(route[5], std::make_pair( 0.0f, 10.0f));
    }

    // TC-SP-023: count_h == 0 or count_v == 0 yields an empty route.
    void ptzRouteZeroCount()
    {
        ScanParams p;
        p.count_h = 0;
        p.count_v = 3;
        QCOMPARE(get_ptz_route(p).size(), (size_t)0);

        p.count_h = 3;
        p.count_v = 0;
        QCOMPARE(get_ptz_route(p).size(), (size_t)0);
    }

    // TC-SP-024: Large grid size is correct (10x10 → 100 points).
    void ptzRouteLargeGrid()
    {
        ScanParams p;
        p.count_h = 10;
        p.count_v = 10;
        p.step_h = 1;
        p.step_v = 1;
        p.ending_h = 9;
        p.ending_v = 9;
        QCOMPARE(get_ptz_route(p).size(), (size_t)100);
    }

    // TC-SP-025: Single-row strip (count_v=1) under both directions yields
    // the same H sweep — V serpentine never engages.
    void ptzRouteSingleRow()
    {
        ScanParams p;
        p.starting_h = 0; p.ending_h = 20; p.step_h = 10; p.count_h = 3;
        p.starting_v = 5; p.ending_v = 5;  p.step_v = 0;  p.count_v = 1;

        p.ptz_direction = 0;
        auto r0 = get_ptz_route(p);
        QCOMPARE(r0.size(), (size_t)3);
        QCOMPARE(r0[0], std::make_pair( 0.0f, 5.0f));
        QCOMPARE(r0[1], std::make_pair(10.0f, 5.0f));
        QCOMPARE(r0[2], std::make_pair(20.0f, 5.0f));

        p.ptz_direction = 1;
        auto r1 = get_ptz_route(p);
        QCOMPARE(r1.size(), (size_t)3);
        QCOMPARE(r1[0], std::make_pair( 0.0f, 5.0f));
        QCOMPARE(r1[1], std::make_pair(10.0f, 5.0f));
        QCOMPARE(r1[2], std::make_pair(20.0f, 5.0f));
    }

    void panoramaRouteDefault()
    {
        ScanParams p;

        auto route = get_panorama_route(p);
        QCOMPARE(route.size(), (size_t)12);
        QCOMPARE(route[0], std::make_pair(0.0f, 0.0f));
        QCOMPARE(route[1], std::make_pair(30.0f, 0.0f));
        QCOMPARE(route[11], std::make_pair(330.0f, 0.0f));
    }

    void panoramaRouteNoDuplicateEndpoint()
    {
        ScanParams p;
        p.panorama_start_h = 15.0f;
        p.panorama_fixed_v = -5.0f;
        p.panorama_count = 4;

        auto route = get_panorama_route(p);
        QCOMPARE(route.size(), (size_t)4);
        QCOMPARE(route[0], std::make_pair(15.0f, -5.0f));
        QCOMPARE(route[1], std::make_pair(105.0f, -5.0f));
        QCOMPARE(route[2], std::make_pair(195.0f, -5.0f));
        QCOMPARE(route[3], std::make_pair(285.0f, -5.0f));
    }

    void panoramaRouteCounterClockwiseWrap()
    {
        ScanParams p;
        p.panorama_start_h = 10.0f;
        p.panorama_count = 4;
        p.panorama_direction = 1;

        auto route = get_panorama_route(p);
        QCOMPARE(route.size(), (size_t)4);
        QCOMPARE(route[0], std::make_pair(10.0f, 0.0f));
        QCOMPARE(route[1], std::make_pair(280.0f, 0.0f));
        QCOMPARE(route[2], std::make_pair(190.0f, 0.0f));
        QCOMPARE(route[3], std::make_pair(100.0f, 0.0f));
    }

    void panoramaRouteStepFallback()
    {
        ScanParams p;
        p.panorama_count = 0;
        p.panorama_step_h = 45.0f;

        auto route = get_panorama_route(p);
        QCOMPARE(route.size(), (size_t)8);
        QCOMPARE(route[0], std::make_pair(0.0f, 0.0f));
        QCOMPARE(route[7], std::make_pair(315.0f, 0.0f));
    }

    void panoramaRouteEndingHorizontal()
    {
        ScanParams p;
        p.panorama_start_h = 350.0f;
        p.panorama_end_h = 20.0f;
        p.panorama_use_end_h = true;
        p.panorama_count = 4;

        auto route = get_panorama_route(p);
        QCOMPARE(route.size(), (size_t)4);
        QCOMPARE(route[0], std::make_pair(350.0f, 0.0f));
        QCOMPARE(route[1], std::make_pair(0.0f, 0.0f));
        QCOMPARE(route[2], std::make_pair(10.0f, 0.0f));
        QCOMPARE(route[3], std::make_pair(20.0f, 0.0f));
    }

    void panoramaZoomFovInterpolation()
    {
        ScanParams p;
        p.panorama_zoom_level = 86.0f;
        update_panorama_fov_from_zoom(p);

        QVERIFY(qAbs(p.panorama_fov_h - 0.49f) < 0.0001f);
        QVERIFY(qAbs(p.panorama_fov_v - 0.28f) < 0.0001f);

        p.panorama_zoom_level = 20.0f;
        update_panorama_fov_from_zoom(p);
        QVERIFY(p.panorama_fov_h < 38.4f);
        QVERIFY(p.panorama_fov_h > 0.49f);
        QVERIFY(p.panorama_fov_v < 22.5f);
        QVERIFY(p.panorama_fov_v > 0.28f);
    }
};

QTEST_MAIN(TestScanParams)
#include "test_scanparams.moc"
