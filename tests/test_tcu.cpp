#include <QtTest>
#include "port/tcu.h"
#include "port/tcu_protocol.h"
#include "util/constants.h"

// Test subclass: intercepts communicate() so no real serial or TCP I/O happens.
// The using declaration exposes the protected set_user_param overloads.
class TestTCU : public TCU
{
    Q_OBJECT
public:
    explicit TestTCU(int type = 0) : TCU(-1, type) {}

    QList<QByteArray> m_frames;

    void communicate(QByteArray write, uint, uint, bool) override
    {
        m_frames.append(write);
    }

    using TCU::set_user_param;   // make both overloads callable from tests
};

class TestTCUSuite : public QObject
{
    Q_OBJECT

private slots:

    // TC-T-001: default values after construction
    void default_values()
    {
        TestTCU tcu(0);

        QCOMPARE(tcu.get(TCU::REPEATED_FREQ), 30.0);
        QCOMPARE(tcu.get(TCU::LASER_WIDTH),   40.0);
        QCOMPARE(tcu.get(TCU::DELAY_A),      100.0);
        QCOMPARE(tcu.get(TCU::GATE_WIDTH_A),  40.0);
        QCOMPARE(tcu.get(TCU::MCP),            5.0);
        QCOMPARE(tcu.get(TCU::CCD_FREQ),      10.0);
    }

    // TC-T-002: type 0 get(GATE_WIDTH_A) returns gate_width_a
    void type0_gate_width_a()
    {
        TestTCU tcu(0);
        QCOMPARE(tcu.get(TCU::GATE_WIDTH_A), 40.0);
    }

    // TC-T-003: type 2 get(GATE_WIDTH_A) returns gate_width_b
    void type2_gate_width_a_returns_b()
    {
        TestTCU tcu(2);

        // Initially gate_width_a == gate_width_b == 40, so both look the same.
        // Modify gate_width_b through set_user_param(GATE_WIDTH_B, 99) and
        // then verify that get(GATE_WIDTH_A) for type 2 reflects the change.
        tcu.set_user_param(TCU::GATE_WIDTH_B, 99.0);
        QCOMPARE(tcu.get(TCU::GATE_WIDTH_A), 99.0);
    }

    // TC-T-004: type 2 get(LASER_WIDTH) returns gate_width_a
    void type2_laser_width_returns_gate_width_a()
    {
        TestTCU tcu(2);
        // Default gate_width_a = 40
        QCOMPARE(tcu.get(TCU::LASER_WIDTH), 40.0);
    }

    // TC-T-005: type 2 get(DELAY_A) returns delay_b
    void type2_delay_a_returns_delay_b()
    {
        TestTCU tcu(2);

        // Set delay_b to 200 and confirm get(DELAY_A) returns it for type 2.
        tcu.set_user_param(TCU::DELAY_B, 200.0);
        QCOMPARE(tcu.get(TCU::DELAY_A), 200.0);
    }

    // TC-T-006: type 0 REPEATED_FREQ frame
    // set_user_param(REPEATED_FREQ, 20.0) sends (1e6 / 8) / 20 = 6250 = 0x186A:
    // [0x88, 0x00, 0x00, 0x00, 0x18, 0x6A, 0x99]
    void type0_rep_freq_frame()
    {
        TestTCU tcu(0);
        tcu.set_user_param(TCU::REPEATED_FREQ, 20.0);

        QVERIFY(!tcu.m_frames.isEmpty());

        QByteArray frame = tcu.m_frames.last();
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

    // TC-T-007: type 0 DELAY_A frame
    // set_user_param(DELAY_A, 200.0) sends 200 = 0xC8: [0x88, 0x02, 0x00, 0x00, 0x00, 0xC8, 0x99]
    void type0_delay_a_frame()
    {
        TestTCU tcu(0);
        tcu.set_user_param(TCU::DELAY_A, 200.0);

        QVERIFY(!tcu.m_frames.isEmpty());

        QByteArray frame = tcu.m_frames.last();
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

    // TC-T-008: type 1 DELAY_A sends two frames, integer and ps
    // For 100.0: integer part int(100.001) / 4 = 25 = 0x19, ps part 0.
    // [0x88, 0x02, 0,0,0, 0x19, 0x99] then [0x88, 0x29, 0,0,0, 0x00, 0x99] (DELAY_A_PS)
    void type1_delay_a_two_frames()
    {
        TestTCU tcu(1);
        QVERIFY(tcu.m_frames.isEmpty());   // constructor sends nothing

        tcu.set_user_param(TCU::DELAY_A, 100.0);
        QCOMPARE(tcu.m_frames.size(), 2);

        // Frame 1: integer part
        QByteArray f1;
        f1.append(char(0x88));
        f1.append(char(0x02));   // DELAY_A
        f1.append(char(0x00));
        f1.append(char(0x00));
        f1.append(char(0x00));
        f1.append(char(0x19));   // 25
        f1.append(char(0x99));
        QCOMPARE(tcu.m_frames[0], f1);

        // Frame 2: picosecond part
        QByteArray f2;
        f2.append(char(0x88));
        f2.append(char(0x29));   // DELAY_A_PS
        f2.append(char(0x00));
        f2.append(char(0x00));
        f2.append(char(0x00));
        f2.append(char(0x00));   // 0
        f2.append(char(0x99));
        QCOMPARE(tcu.m_frames[1], f2);
    }

    // TC-T-009: type 2 remaps DELAY_A to DELAY_B
    // the first frame's command byte must be 0x04, not 0x02
    void type2_delay_a_remapped_to_delay_b()
    {
        TestTCU tcu(2);
        tcu.set_user_param(TCU::DELAY_A, 100.0);

        QVERIFY(tcu.m_frames.size() >= 1);

        QByteArray frame = tcu.m_frames[0];
        QCOMPARE(frame.size(), 7);

        // cmd byte at index 1 should be DELAY_B (0x04), not DELAY_A (0x02)
        QCOMPARE(quint8(frame[1]), quint8(0x04));
    }

    // TC-T-010: frame structure: header 0x88, trailer 0x99, length 7
    void frame_structure()
    {
        TestTCU tcu(0);
        tcu.set_user_param(TCU::MCP, uint(10));

        QVERIFY(!tcu.m_frames.isEmpty());

        QByteArray frame = tcu.m_frames.last();
        QCOMPARE(frame.size(), 7);
        QCOMPARE(quint8(frame[0]), quint8(TCU_HEAD));
        QCOMPARE(quint8(frame[6]), quint8(TCU_TAIL));
    }

    // TC-T-011: EST_DIST sets delay_a
    // set_user_param(EST_DIST, 1500.0) gives delay_a = 1500 / 0.15 = 10000
    void est_dist_delay_calculation()
    {
        TestTCU tcu(0);
        tcu.set_user_param(TCU::EST_DIST, 1500.0);

        QCOMPARE(tcu.get(TCU::DELAY_A),  10000.0);
        QCOMPARE(tcu.get(TCU::EST_DIST),  1500.0);
    }

    // TC-T-012: with AB_LOCK off, A and B are independent
    // setting EST_DIST updates delay_a; delay_b stays at its default 100
    void ab_lock_off_independent()
    {
        TestTCU tcu(0);

        // Turn off AB lock via the uint overload
        tcu.set_user_param(TCU::AB_LOCK, uint(0));

        tcu.set_user_param(TCU::EST_DIST, 1500.0);

        QCOMPARE(tcu.get(TCU::DELAY_A), 10000.0);
        // delay_b is not updated because ab_lock is off
        QCOMPARE(tcu.get(TCU::DELAY_B),   100.0);
    }

    // TC-T-013: auto PRF, clamped and unclamped
    // A: dist 3000 gives 1e6 / 21040 = 47.5 kHz, clamped to TCU_MAX_PRF_KHZ = 30.
    // B: dist 15000 gives 1e6 / 101040 = 9.9 kHz, not clamped.
    void auto_prf_clamp()
    {
        // part A: clamped
        {
            TestTCU tcu(0);
            tcu.set_user_param(TCU::AUTO_PRF, uint(1));
            tcu.set_user_param(TCU::EST_DIST, 3000.0);

            QCOMPARE(tcu.get(TCU::REPEATED_FREQ), TCU_MAX_PRF_KHZ);
        }

        // part B: not clamped
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
