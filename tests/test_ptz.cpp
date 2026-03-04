#include <QtTest>
#include "port/ptz.h"
#include "port/pelco_protocol.h"

// ---------------------------------------------------------------------------
// Test subclass: intercepts communicate() so no real I/O is needed.
// ---------------------------------------------------------------------------
class TestPTZ : public PTZ {
    Q_OBJECT
public:
    TestPTZ(uchar addr = 0x01, uint speed = 16) : PTZ(-1, addr, speed) {}

    QList<QByteArray> frames;
    QByteArray mock_response;

    void communicate(QByteArray write, uint read_size, uint read_timeout, bool heartbeat) override {
        Q_UNUSED(read_timeout);
        Q_UNUSED(heartbeat);
        frames.append(write);
        if (read_size > 0) {
            retrieve_mutex.lock();
            last_read = mock_response;
            retrieve_mutex.unlock();
        }
    }

    // Expose protected slot for direct testing
    using PTZ::ptz_control;
};

// ---------------------------------------------------------------------------
// The actual test class
// ---------------------------------------------------------------------------
class TestPTZSuite : public QObject
{
    Q_OBJECT

private:
    // Helper: compute expected checksum identical to PTZ::checksum()
    static uchar expectedChecksum(const QByteArray &data)
    {
        uint sum = 1;
        for (char c : data) sum += uchar(c);
        return sum & 0xFF;
    }

private slots:

    // TC-P-001: STOP frame --------------------------------------------------
    void stopFrame()
    {
        TestPTZ ptz;
        ptz.ptz_control(PTZ::STOP);

        QCOMPARE(ptz.frames.size(), 1);
        QByteArray f = ptz.frames.first();

        QCOMPARE(f.size(), 7);
        QCOMPARE(uchar(f[0]), uchar(0xFF));       // SYNC_BYTE
        QCOMPARE(uchar(f[1]), uchar(0x01));        // default address
        QCOMPARE(uchar(f[3]), uchar(PTZ_DIR_STOP));
        QCOMPARE(uchar(f[4]), uchar(0x00));        // no horiz speed
        QCOMPARE(uchar(f[5]), uchar(0x00));        // no vert speed
    }

    // TC-P-002: UP -- speed in byte[5] only ---------------------------------
    void upFrame()
    {
        TestPTZ ptz;   // default speed = 16 (0x10)
        ptz.ptz_control(PTZ::UP);

        QCOMPARE(ptz.frames.size(), 1);
        QByteArray f = ptz.frames.first();

        QCOMPARE(uchar(f[3]), uchar(PTZ_DIR_UP));
        QCOMPARE(uchar(f[4]), uchar(0x00));        // no horizontal speed
        QCOMPARE(uchar(f[5]), uchar(0x10));         // vertical speed = 16
    }

    // TC-P-003: UP_RIGHT -- both speed bytes --------------------------------
    void upRightFrame()
    {
        TestPTZ ptz;
        ptz.ptz_control(PTZ::UP_RIGHT);

        QCOMPARE(ptz.frames.size(), 1);
        QByteArray f = ptz.frames.first();

        QCOMPARE(uchar(f[3]), uchar(PTZ_DIR_UP_RIGHT));
        QCOMPARE(uchar(f[4]), uchar(0x10));         // horiz speed
        QCOMPARE(uchar(f[5]), uchar(0x10));          // vert speed
    }

    // TC-P-004: All 10 direction commands -> correct byte[3] ----------------
    void allDirectionBytes()
    {
        struct DirCase {
            qint32 param;
            uchar expected;
        };
        const DirCase cases[] = {
            { PTZ::STOP,       PTZ_DIR_STOP       },
            { PTZ::RIGHT,      PTZ_DIR_RIGHT      },
            { PTZ::LEFT,       PTZ_DIR_LEFT       },
            { PTZ::UP,         PTZ_DIR_UP         },
            { PTZ::UP_RIGHT,   PTZ_DIR_UP_RIGHT   },
            { PTZ::UP_LEFT,    PTZ_DIR_UP_LEFT    },
            { PTZ::DOWN,       PTZ_DIR_DOWN       },
            { PTZ::DOWN_RIGHT, PTZ_DIR_DOWN_RIGHT },
            { PTZ::DOWN_LEFT,  PTZ_DIR_DOWN_LEFT  },
            { PTZ::SELF_CHECK, PTZ_DIR_SELF_CHECK },
        };

        for (const auto &dc : cases) {
            TestPTZ ptz;
            ptz.ptz_control(dc.param);

            QVERIFY2(ptz.frames.size() == 1,
                      qPrintable(QString("Expected 1 frame for param 0x%1")
                                     .arg(dc.param, 2, 16, QChar('0'))));
            QCOMPARE(uchar(ptz.frames.first()[3]), dc.expected);
        }
    }

    // TC-P-005: SET_H(180.5) -> angle encoding ------------------------------
    void setHorizontalAngle()
    {
        TestPTZ ptz;
        ptz.ptz_control(PTZ::SET_H, 180.5);

        QCOMPARE(ptz.frames.size(), 1);
        QByteArray f = ptz.frames.first();

        // angle = int(180.5 * 100) % 36000 = 18050
        QCOMPARE(uchar(f[3]), uchar(PTZ_SET_H));    // 0x4B
        QCOMPARE(uchar(f[4]), uchar(0x46));          // (18050 >> 8) & 0xFF
        QCOMPARE(uchar(f[5]), uchar(0x82));          // 18050 & 0xFF
    }

    // TC-P-006: SET_H(-10) -> negative angle wrapping -----------------------
    void setHorizontalAngleNegative()
    {
        TestPTZ ptz;
        ptz.ptz_control(PTZ::SET_H, -10.0);

        QCOMPARE(ptz.frames.size(), 1);
        QByteArray f = ptz.frames.first();

        // angle = (int(-10.0 * 100) % 36000 + 36000) % 36000 = 35000
        QCOMPARE(uchar(f[4]), uchar(0x88));          // (35000 >> 8) & 0xFF
        QCOMPARE(uchar(f[5]), uchar(0xB8));          // 35000 & 0xFF
    }

    // TC-P-007: SET_V(50) -> clamped to 40 degrees --------------------------
    void setVerticalAngleClamped()
    {
        TestPTZ ptz;
        ptz.ptz_control(PTZ::SET_V, 50.0);

        QCOMPARE(ptz.frames.size(), 1);
        QByteArray f = ptz.frames.first();

        // clamped to 40.0, angle = (int(40.0 * 100) % 36000 + 36000) % 36000 = 4000
        QCOMPARE(uchar(f[3]), uchar(PTZ_SET_V));    // 0x4D
        QCOMPARE(uchar(f[4]), uchar(0x0F));          // (4000 >> 8) & 0xFF
        QCOMPARE(uchar(f[5]), uchar(0xA0));          // 4000 & 0xFF
    }

    // TC-P-008: SET_V(-30) -> negative vertical -----------------------------
    void setVerticalAngleNegative()
    {
        TestPTZ ptz;
        ptz.ptz_control(PTZ::SET_V, -30.0);

        QCOMPARE(ptz.frames.size(), 1);
        QByteArray f = ptz.frames.first();

        // angle = (int(-30.0 * 100) % 36000 + 36000) % 36000 = 33000
        QCOMPARE(uchar(f[4]), uchar(0x80));          // (33000 >> 8) & 0xFF
        QCOMPARE(uchar(f[5]), uchar(0xE8));          // 33000 & 0xFF
    }

    // TC-P-009: SPEED(100) -> clamped to MAX_SPEED (63) ---------------------
    void speedClampedHigh()
    {
        TestPTZ ptz;
        int ret = ptz.ptz_control(PTZ::SPEED, 100);

        // SPEED returns 0 immediately, no frame sent
        QCOMPARE(ret, 0);
        QCOMPARE(ptz.frames.size(), 0);
        QCOMPARE(ptz.get(PTZ::SPEED), 63.0);
    }

    // TC-P-010: SPEED(0) -> clamped to MIN_SPEED (1) -----------------------
    void speedClampedLow()
    {
        TestPTZ ptz;
        int ret = ptz.ptz_control(PTZ::SPEED, 0);

        QCOMPARE(ret, 0);
        QCOMPARE(ptz.frames.size(), 0);
        QCOMPARE(ptz.get(PTZ::SPEED), 1.0);
    }

    // TC-P-011: ADDRESS(5) -> propagation to subsequent frame ---------------
    void addressPropagation()
    {
        TestPTZ ptz;
        ptz.ptz_control(PTZ::ADDRESS, 5);

        // ADDRESS returns immediately, no frame
        QCOMPARE(ptz.frames.size(), 0);

        // Now send STOP -- it should use the new address
        ptz.ptz_control(PTZ::STOP);
        QCOMPARE(ptz.frames.size(), 1);
        QCOMPARE(uchar(ptz.frames.first()[1]), uchar(0x05));
    }

    // TC-P-012: Checksum verification for STOP frame ------------------------
    void checksumVerification()
    {
        TestPTZ ptz;
        ptz.ptz_control(PTZ::STOP);

        QByteArray f = ptz.frames.first();

        // Build expected data (bytes 0-5) with byte[6] = 0x00 (pre-checksum)
        QByteArray preChecksum = f.left(6);
        preChecksum.append(char(0x00));
        uchar expected = expectedChecksum(preChecksum);

        // For STOP with addr=0x01:
        //   sum = 1 + 0xFF + 0x01 + 0x00 + 0x00 + 0x00 + 0x00 + 0x00 = 0x101
        //   checksum = 0x101 & 0xFF = 0x01
        QCOMPARE(expected, uchar(0x01));
        QCOMPARE(uchar(f[6]), expected);
    }

    // TC-P-013: Horizontal angle feedback parsing ---------------------------
    void horizontalAngleFeedback()
    {
        TestPTZ ptz;

        // angle = 18000 -> 180.0 degrees
        // bytes: 0x46 = 18000 >> 8 = 70, 0x50 = 18000 & 0xFF = 80
        const char resp[] = {
            char(0xFF), char(0x01), char(0x00), char(PTZ_REPLY_H),
            char(0x46), char(0x50), char(0x00)
        };
        ptz.mock_response = QByteArray(resp, 7);

        // ptz_control(ANGLE_H) sends a query and reads back mock_response
        ptz.ptz_control(PTZ::ANGLE_H);

        QCOMPARE(ptz.get(PTZ::ANGLE_H), 180.0);
    }

    // TC-P-014: Vertical angle feedback with offset (negative angle) --------
    void verticalAngleFeedback()
    {
        TestPTZ ptz;

        // Encode -20 degrees on wire:
        //   -20 * 100 = -2000
        //   ((-2000 % 36000) + 36000) % 36000 = 34000
        //   34000 >> 8 = 0x84, 34000 & 0xFF = 0xD0
        // Parsing:
        //   angle = (0x84 << 8) + 0xD0 = 34000
        //   (34000 + 4000) % 36000 - 4000 = 2000 - 4000 = -2000
        //   clamped: min(max(-2000, -4000), 4000) = -2000
        //   angle_v = -2000 / 100.0 = -20.0
        const char resp[] = {
            char(0xFF), char(0x01), char(0x00), char(PTZ_REPLY_V),
            char(0x84), char(0xD0), char(0x00)
        };
        ptz.mock_response = QByteArray(resp, 7);

        ptz.ptz_control(PTZ::ANGLE_V);

        QCOMPARE(ptz.get(PTZ::ANGLE_V), -20.0);
    }
};

QTEST_MAIN(TestPTZSuite)
#include "test_ptz.moc"
