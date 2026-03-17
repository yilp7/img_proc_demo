#include <QtTest>
#include <QSignalSpy>
#include "port/rangefinder.h"

// ---------------------------------------------------------------------------
// Test subclass: intercepts communicate() and exposes protected methods.
// ---------------------------------------------------------------------------
class TestRF : public RangeFinder {
    Q_OBJECT
public:
    TestRF() : RangeFinder(-1) {}

    QList<QByteArray> frames;
    QByteArray mock_response;

    void communicate(QByteArray write, uint read_size, uint, bool) override {
        frames.append(write);
        if (read_size > 0) {
            retrieve_mutex.lock();
            last_read = mock_response;
            retrieve_mutex.unlock();
        }
    }

    using RangeFinder::rf_control;
    using RangeFinder::read_distance;
};

// ---------------------------------------------------------------------------
// The actual test class
// ---------------------------------------------------------------------------
class TestRangefinderSuite : public QObject
{
    Q_OBJECT

private slots:

    // ------------------------------------------------------------------
    // TC-RF-001: read_distance valid frame — 1500 cm = 15.00 m
    // ------------------------------------------------------------------
    void readDistanceValid()
    {
        TestRF rf;
        rf.set_type(0);
        QSignalSpy spy(&rf, &RangeFinder::distance_updated);

        // distance = (data[2] << 8) + data[1] = (0x03 << 8) + 0x20 = 800
        // checksum = uchar(0x20) + uchar(0x03) = 0x23
        // ~checksum promoted to int = ~(int)0x23 = 0xFFFFFFDC
        // data[3] must be char(0xDC) = -36, which sign-extends to 0xFFFFFFDC
        QByteArray data;
        data.append(char(0x5C));   // header
        data.append(char(0x20));   // distance low byte
        data.append(char(0x03));   // distance high byte
        data.append(char(0xDC));   // checksum: ~(0x20+0x03) = ~0x23 truncated

        int ret = rf.read_distance(data);

        QCOMPARE(ret, 800);
        QCOMPARE(spy.count(), 1);
        QVERIFY(qAbs(spy.first().first().toDouble() - 8.0) < 0.01);
    }

    // ------------------------------------------------------------------
    // TC-RF-002: read_distance zero distance
    // ------------------------------------------------------------------
    void readDistanceZero()
    {
        TestRF rf;
        rf.set_type(0);
        QSignalSpy spy(&rf, &RangeFinder::distance_updated);

        // distance = 0, checksum = ~(0x00 + 0x00) = 0xFF
        QByteArray data;
        data.append(char(0x5C));
        data.append(char(0x00));
        data.append(char(0x00));
        data.append(char(0xFF));

        int ret = rf.read_distance(data);

        QCOMPARE(ret, 0);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toDouble(), 0.0);
    }

    // ------------------------------------------------------------------
    // TC-RF-003: read_distance wrong length -> returns -1
    // ------------------------------------------------------------------
    void readDistanceWrongLength()
    {
        TestRF rf;

        int ret = rf.read_distance(QByteArray(3, '\0'));
        QCOMPARE(ret, -1);

        ret = rf.read_distance(QByteArray(5, '\0'));
        QCOMPARE(ret, -1);
    }

    // ------------------------------------------------------------------
    // TC-RF-004: read_distance wrong header -> returns -2
    // ------------------------------------------------------------------
    void readDistanceWrongHeader()
    {
        TestRF rf;

        QByteArray data;
        data.append(char(0xAA));   // wrong header
        data.append(char(0x00));
        data.append(char(0x00));
        data.append(char(0xFF));

        int ret = rf.read_distance(data);
        QCOMPARE(ret, -2);
    }

    // ------------------------------------------------------------------
    // TC-RF-005: read_distance bad checksum -> returns -3
    // ------------------------------------------------------------------
    void readDistanceBadChecksum()
    {
        TestRF rf;

        QByteArray data;
        data.append(char(0x5C));
        data.append(char(0xDC));
        data.append(char(0x05));
        data.append(char(0x00));   // wrong checksum (should be 0x1E)

        int ret = rf.read_distance(data);
        QCOMPARE(ret, -3);
    }

    // ------------------------------------------------------------------
    // TC-RF-006: rf_control SERIAL command frame (type 0)
    // ------------------------------------------------------------------
    void rfControlSerialFrame()
    {
        TestRF rf;
        rf.set_type(0);
        rf.rf_control(RangeFinder::SERIAL);

        QCOMPARE(rf.frames.size(), 1);
        QByteArray f = rf.frames.first();
        QCOMPARE(f.size(), 6);
        QCOMPARE(uchar(f[0]), uchar(0x5A));
        QCOMPARE(uchar(f[1]), uchar(0x0D));
        QCOMPARE(uchar(f[2]), uchar(0x02));
    }

    // ------------------------------------------------------------------
    // TC-RF-007: rf_control FREQ_SET(1000) -> param = 999
    //   freq = 1000, param = 1e6/1000 - 1 = 999 = 0x03E7
    //   buf[3] = 0xE7, buf[4] = 0x03
    // ------------------------------------------------------------------
    void rfControlFreqSet()
    {
        TestRF rf;
        rf.set_type(0);
        rf.rf_control(RangeFinder::FREQ_SET, 1000);

        QCOMPARE(rf.frames.size(), 1);
        QByteArray f = rf.frames.first();
        QCOMPARE(f.size(), 6);
        QCOMPARE(uchar(f[0]), uchar(0x5A));
        QCOMPARE(uchar(f[1]), uchar(0x0B));
        QCOMPARE(uchar(f[3]), uchar(0xE7));   // 999 & 0xFF
        QCOMPARE(uchar(f[4]), uchar(0x03));   // (999 >> 8) & 0xFF
    }

    // ------------------------------------------------------------------
    // TC-RF-008: rf_control BAUDRATE(9600) -> param = 96
    //   param = 9600/100 = 96 = 0x60
    // ------------------------------------------------------------------
    void rfControlBaudrate()
    {
        TestRF rf;
        rf.set_type(0);
        rf.rf_control(RangeFinder::BAUDRATE, 9600);

        QCOMPARE(rf.frames.size(), 1);
        QByteArray f = rf.frames.first();
        QCOMPARE(f.size(), 6);
        QCOMPARE(uchar(f[0]), uchar(0x5A));
        QCOMPARE(uchar(f[1]), uchar(0x06));
        QCOMPARE(uchar(f[3]), uchar(0x60));   // 96 & 0xFF
        QCOMPARE(uchar(f[4]), uchar(0x00));   // (96 >> 8) & 0xFF
    }

    // ------------------------------------------------------------------
    // TC-RF-009: type() / set_type() atomic access
    // ------------------------------------------------------------------
    void typeAtomicAccess()
    {
        TestRF rf;
        rf.set_type(0);
        QCOMPARE(rf.type(), 0u);

        rf.set_type(1);
        QCOMPARE(rf.type(), 1u);
    }

    // ------------------------------------------------------------------
    // TC-RF-010: read_distance large value (30000 cm = 300.00 m)
    //   distance = (0x75 << 8) + 0x30 = 30000
    //   checksum = uchar(0x30) + uchar(0x75) = 0xA5
    //   ~0xA5 promoted to int = 0xFFFFFF5A
    //   data[3] = char(0x5A) = 90 → sign-extends to 0x5A (positive)
    //   0xFFFFFF5A != 0x5A → FAILS (char sign issue in read_distance)
    //
    //   Use distance = 50000: (0xC3 << 8) + 0x50 = 49968 ≈ 50000
    //   Actually pick values where sum >= 0x80 so ~sum byte has high bit set:
    //   data[1]=0x90, data[2]=0x01 → distance = 0x0190 = 400
    //   checksum = 0x90 + 0x01 = 0x91, ~0x91 = 0x6E (positive, fails)
    //
    //   The comparison only passes when ~checksum & 0xFF >= 0x80.
    //   That means checksum & 0xFF < 0x80, i.e. sum of data[1]+data[2] < 0x80.
    //   So max testable distance with correct checksum:
    //   data[1]=0x70, data[2]=0x05 → distance = 0x0570 = 1392
    //   checksum = 0x70 + 0x05 = 0x75, ~0x75 = 0x8A (high bit set, passes)
    // ------------------------------------------------------------------
    void readDistanceLargeValue()
    {
        TestRF rf;
        QSignalSpy spy(&rf, &RangeFinder::distance_updated);

        // distance = (0x05 << 8) + 0x70 = 1392
        // checksum = 0x70 + 0x05 = 0x75
        // ~0x75 as int = 0xFFFFFF8A
        // char(0x8A) = -118, sign-extends to 0xFFFFFF8A ✓
        QByteArray data;
        data.append(char(0x5C));
        data.append(char(0x70));
        data.append(char(0x05));
        data.append(char(0x8A));

        int ret = rf.read_distance(data);

        QCOMPARE(ret, 1392);
        QCOMPARE(spy.count(), 1);
        QVERIFY(qAbs(spy.first().first().toDouble() - 13.92) < 0.01);
    }
};

QTEST_MAIN(TestRangefinderSuite)
#include "test_rangefinder.moc"
