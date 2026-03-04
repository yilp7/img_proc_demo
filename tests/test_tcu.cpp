#include <QtTest>
#include "port/tcu.h"
#include "port/tcu_protocol.h"
#include "util/constants.h"

// ---------------------------------------------------------------------------
// Test subclass: intercepts communicate() so no real serial/TCP I/O happens.
// The `using` declaration exposes the protected set_user_param overloads.
// ---------------------------------------------------------------------------
class TestTCU : public TCU
{
    Q_OBJECT
public:
    explicit TestTCU(int type = 0) : TCU(-1, type) {}

    QList<QByteArray> frames;

    void communicate(QByteArray write, uint, uint, bool) override {
        frames.append(write);
    }

    using TCU::set_user_param;   // make both overloads callable from tests
};

// ---------------------------------------------------------------------------
// QtTest test class
// ---------------------------------------------------------------------------
class TestTCUSuite : public QObject
{
    Q_OBJECT

private slots:

    // ------------------------------------------------------------------
    // TC-T-001: Default values after construction
    // ------------------------------------------------------------------
    void defaultValues()
    {
        TestTCU tcu(0);

        QCOMPARE(tcu.get(TCU::REPEATED_FREQ), 30.0);
        QCOMPARE(tcu.get(TCU::LASER_WIDTH),   40.0);
        QCOMPARE(tcu.get(TCU::DELAY_A),      100.0);
        QCOMPARE(tcu.get(TCU::GATE_WIDTH_A),  40.0);
        QCOMPARE(tcu.get(TCU::MCP),            5.0);
        QCOMPARE(tcu.get(TCU::CCD_FREQ),      10.0);
    }

    // ------------------------------------------------------------------
    // TC-T-002: Type 0 get(GATE_WIDTH_A) returns gate_width_a
    // ------------------------------------------------------------------
    void type0GateWidthA()
    {
        TestTCU tcu(0);
        QCOMPARE(tcu.get(TCU::GATE_WIDTH_A), 40.0);
    }

    // ------------------------------------------------------------------
    // TC-T-003: Type 2 get(GATE_WIDTH_A) returns gate_width_b
    // ------------------------------------------------------------------
    void type2GateWidthAReturnsB()
    {
        TestTCU tcu(2);

        // Initially gate_width_a == gate_width_b == 40, so both look the same.
        // Modify gate_width_b through set_user_param(GATE_WIDTH_B, 99) and
        // then verify that get(GATE_WIDTH_A) for type 2 reflects the change.
        tcu.set_user_param(TCU::GATE_WIDTH_B, 99.0);
        QCOMPARE(tcu.get(TCU::GATE_WIDTH_A), 99.0);
    }

    // ------------------------------------------------------------------
    // TC-T-004: Type 2 get(LASER_WIDTH) returns gate_width_a
    // ------------------------------------------------------------------
    void type2LaserWidthReturnsGateWidthA()
    {
        TestTCU tcu(2);
        // Default gate_width_a = 40
        QCOMPARE(tcu.get(TCU::LASER_WIDTH), 40.0);
    }

    // ------------------------------------------------------------------
    // TC-T-005: Type 2 get(DELAY_A) returns delay_b
    // ------------------------------------------------------------------
    void type2DelayAReturnsDelayB()
    {
        TestTCU tcu(2);

        // Set delay_b to 200 and confirm get(DELAY_A) returns it for type 2.
        tcu.set_user_param(TCU::DELAY_B, 200.0);
        QCOMPARE(tcu.get(TCU::DELAY_A), 200.0);
    }

    // ------------------------------------------------------------------
    // TC-T-006: Type 0 REPEATED_FREQ protocol frame
    //   set_user_param(REPEATED_FREQ, 20.0)
    //   -> set_tcu_param(REPEATED_FREQ, (1e6 / 8) / 20 = 6250)
    //   -> frame: [0x88, 0x00, 0x00, 0x00, 0x18, 0x6A, 0x99]
    // ------------------------------------------------------------------
    void type0RepFreqFrame()
    {
        TestTCU tcu(0);
        tcu.set_user_param(TCU::REPEATED_FREQ, 20.0);

        QVERIFY(!tcu.frames.isEmpty());

        QByteArray frame = tcu.frames.last();
        QCOMPARE(frame.size(), 7);

        // 6250 = 0x0000186A
        QByteArray expected;
        expected.append(char(0x88));   // TCU_HEAD
        expected.append(char(0x00));   // REPEATED_FREQ cmd
        expected.append(char(0x00));   // MSB
        expected.append(char(0x00));
        expected.append(char(0x18));
        expected.append(char(0x6A));   // LSB
        expected.append(char(0x99));   // TCU_TAIL

        QCOMPARE(frame, expected);
    }

    // ------------------------------------------------------------------
    // TC-T-007: Type 0 DELAY_A frame
    //   set_user_param(DELAY_A, 200.0)
    //   -> round(200) = 200 = 0xC8
    //   -> frame: [0x88, 0x02, 0x00, 0x00, 0x00, 0xC8, 0x99]
    // ------------------------------------------------------------------
    void type0DelayAFrame()
    {
        TestTCU tcu(0);
        tcu.set_user_param(TCU::DELAY_A, 200.0);

        QVERIFY(!tcu.frames.isEmpty());

        QByteArray frame = tcu.frames.last();
        QCOMPARE(frame.size(), 7);

        QByteArray expected;
        expected.append(char(0x88));
        expected.append(char(0x02));   // DELAY_A cmd
        expected.append(char(0x00));
        expected.append(char(0x00));
        expected.append(char(0x00));
        expected.append(char(0xC8));   // 200
        expected.append(char(0x99));

        QCOMPARE(frame, expected);
    }

    // ------------------------------------------------------------------
    // TC-T-008: Type 1 DELAY_A emits two frames (integer + ps)
    //   set_user_param(DELAY_A, 100.0)
    //   -> integer_part = int(100.001)/4 = 25 = 0x19
    //   -> decimal_part = round(0.025) = 0
    //   Frame 1: [0x88, 0x02, 0,0,0,0x19, 0x99]   cmd=DELAY_A
    //   Frame 2: [0x88, 0x29, 0,0,0,0x00, 0x99]   cmd=DELAY_A_PS
    // ------------------------------------------------------------------
    void type1DelayATwoFrames()
    {
        TestTCU tcu(1);
        QVERIFY(tcu.frames.isEmpty());   // constructor sends nothing

        tcu.set_user_param(TCU::DELAY_A, 100.0);
        QCOMPARE(tcu.frames.size(), 2);

        // Frame 1: integer part
        QByteArray f1;
        f1.append(char(0x88));
        f1.append(char(0x02));   // DELAY_A
        f1.append(char(0x00));
        f1.append(char(0x00));
        f1.append(char(0x00));
        f1.append(char(0x19));   // 25
        f1.append(char(0x99));
        QCOMPARE(tcu.frames[0], f1);

        // Frame 2: picosecond part
        QByteArray f2;
        f2.append(char(0x88));
        f2.append(char(0x29));   // DELAY_A_PS
        f2.append(char(0x00));
        f2.append(char(0x00));
        f2.append(char(0x00));
        f2.append(char(0x00));   // 0
        f2.append(char(0x99));
        QCOMPARE(tcu.frames[1], f2);
    }

    // ------------------------------------------------------------------
    // TC-T-009: Type 2 DELAY_A cmd byte remapped to DELAY_B (0x04)
    //   set_user_param(DELAY_A, 100.0)
    //   -> in set_tcu_param type 2, DELAY_A is remapped to DELAY_B
    //   -> first frame cmd byte must be 0x04, NOT 0x02
    // ------------------------------------------------------------------
    void type2DelayARemappedToDelayB()
    {
        TestTCU tcu(2);
        tcu.set_user_param(TCU::DELAY_A, 100.0);

        QVERIFY(tcu.frames.size() >= 1);

        QByteArray frame = tcu.frames[0];
        QCOMPARE(frame.size(), 7);

        // cmd byte at index 1 should be DELAY_B (0x04), not DELAY_A (0x02)
        QCOMPARE(quint8(frame[1]), quint8(0x04));
    }

    // ------------------------------------------------------------------
    // TC-T-010: Frame structure — header 0x88, trailer 0x99, length 7
    // ------------------------------------------------------------------
    void frameStructure()
    {
        TestTCU tcu(0);
        tcu.set_user_param(TCU::MCP, uint(10));

        QVERIFY(!tcu.frames.isEmpty());

        QByteArray frame = tcu.frames.last();
        QCOMPARE(frame.size(), 7);
        QCOMPARE(quint8(frame[0]), quint8(TCU_HEAD));
        QCOMPARE(quint8(frame[6]), quint8(TCU_TAIL));
    }

    // ------------------------------------------------------------------
    // TC-T-011: EST_DIST -> delay_a calculation
    //   set_user_param(EST_DIST, 1500.0)
    //   delay_a = 1500 / 0.15 = 10000
    //   get(DELAY_A) == 10000
    //   get(EST_DIST) == 1500
    // ------------------------------------------------------------------
    void estDistDelayCalculation()
    {
        TestTCU tcu(0);
        tcu.set_user_param(TCU::EST_DIST, 1500.0);

        QCOMPARE(tcu.get(TCU::DELAY_A),  10000.0);
        QCOMPARE(tcu.get(TCU::EST_DIST),  1500.0);
    }

    // ------------------------------------------------------------------
    // TC-T-012: AB_LOCK off -> independent A/B
    //   Disable ab_lock, then set EST_DIST.
    //   delay_a should update; delay_b should remain at default 100.
    // ------------------------------------------------------------------
    void abLockOffIndependent()
    {
        TestTCU tcu(0);

        // Turn off AB lock via the uint overload
        tcu.set_user_param(TCU::AB_LOCK, uint(0));

        tcu.set_user_param(TCU::EST_DIST, 1500.0);

        QCOMPARE(tcu.get(TCU::DELAY_A), 10000.0);
        // delay_b not updated because ab_lock is off — stays at default
        QCOMPARE(tcu.get(TCU::DELAY_B),   100.0);
    }

    // ------------------------------------------------------------------
    // TC-T-013: Auto PRF — clamped and unclamped
    //
    // Part A: dist=3000 -> PRF ~47.5 -> clamped to 30
    //   delay_a = 3000/0.15 = 20000
    //   depth_of_view = 40*0.15 = 6.0
    //   rep_freq = 1e6 / (20000 + 6/0.15 + 1000) = 1e6/21040 ~= 47.53
    //   Clamped to TCU_MAX_PRF_KHZ = 30
    //
    // Part B: dist=15000 -> PRF < 30 -> not clamped
    //   delay_a = 15000/0.15 = 100000
    //   rep_freq = 1e6 / (100000 + 40 + 1000) = 1e6/101040 ~= 9.897
    // ------------------------------------------------------------------
    void autoPrfClamp()
    {
        // --- Part A: clamped ---
        {
            TestTCU tcu(0);
            tcu.set_user_param(TCU::AUTO_PRF, uint(1));
            tcu.set_user_param(TCU::EST_DIST, 3000.0);

            QCOMPARE(tcu.get(TCU::REPEATED_FREQ), TCU_MAX_PRF_KHZ);
        }

        // --- Part B: not clamped ---
        {
            TestTCU tcu(0);
            tcu.set_user_param(TCU::AUTO_PRF, uint(1));
            tcu.set_user_param(TCU::EST_DIST, 15000.0);

            // rep_freq = 1e6 / (100000 + 40 + 1000) = 1e6 / 101040
            double expected = 1e6 / (15000.0 / LIGHT_SPEED_M_NS
                                     + (40.0 * LIGHT_SPEED_M_NS) / LIGHT_SPEED_M_NS
                                     + 1000.0);
            QVERIFY(tcu.get(TCU::REPEATED_FREQ) < TCU_MAX_PRF_KHZ);
            QVERIFY(qAbs(tcu.get(TCU::REPEATED_FREQ) - expected) < 0.01);
        }
    }
};

QTEST_MAIN(TestTCUSuite)
#include "test_tcu.moc"
