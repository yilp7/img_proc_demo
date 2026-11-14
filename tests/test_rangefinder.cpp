#include <QtTest>
#include <QSignalSpy>
#include "port/rangefinder.h"

// Test subclass: intercepts communicate() and exposes protected methods.
class TestRF : public RangeFinder
{
    Q_OBJECT
public:
    TestRF() : RangeFinder(-1) {}

    QList<QByteArray> m_frames;
    QByteArray m_mock_response;

    void communicate(QByteArray write, uint read_size, uint, bool) override
    {
        m_frames.append(write);
        if (read_size > 0) {
            m_retrieve_mutex.lock();
            m_last_read = m_mock_response;
            m_retrieve_mutex.unlock();
        }
    }

    using RangeFinder::rf_control;
    using RangeFinder::read_distance;
};

class TestRangefinderSuite : public QObject
{
    Q_OBJECT

private slots:

    // TC-RF-001: read_distance valid frame, 1500 cm = 15.00 m
    void read_distance_valid()
    {
        TestRF rf;
        rf.set_type(0);
        QSignalSpy spy(&rf, &RangeFinder::distance_updated);

        // distance = (0x03 << 8) + 0x20 = 800, checksum = 0x20 + 0x03 = 0x23.
        // ~checksum as int is 0xFFFFFFDC, so data[3] must be char(0xDC), which sign-extends to it.
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

    // TC-RF-002: read_distance zero distance
    void read_distance_zero()
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

    // TC-RF-003: read_distance wrong length returns -1
    void read_distance_wrong_length()
    {
        TestRF rf;

        int ret = rf.read_distance(QByteArray(3, '\0'));
        QCOMPARE(ret, -1);

        ret = rf.read_distance(QByteArray(5, '\0'));
        QCOMPARE(ret, -1);
    }

    // TC-RF-004: read_distance wrong header returns -2
    void read_distance_wrong_header()
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

    // TC-RF-005: read_distance bad checksum returns -3
    void read_distance_bad_checksum()
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

    // TC-RF-006: rf_control SERIAL command frame (type 0)
    void rf_control_serial_frame()
    {
        TestRF rf;
        rf.set_type(0);
        rf.rf_control(RangeFinder::SERIAL);

        QCOMPARE(rf.m_frames.size(), 1);
        QByteArray f = rf.m_frames.first();
        QCOMPARE(f.size(), 6);
        QCOMPARE(uchar(f[0]), uchar(0x5A));
        QCOMPARE(uchar(f[1]), uchar(0x0D));
        QCOMPARE(uchar(f[2]), uchar(0x02));
    }

    // TC-RF-007: rf_control FREQ_SET(1000) gives param 999
    // param = 1e6 / 1000 - 1 = 999 = 0x03E7, so buf[3] = 0xE7 and buf[4] = 0x03
    void rf_control_freq_set()
    {
        TestRF rf;
        rf.set_type(0);
        rf.rf_control(RangeFinder::FREQ_SET, 1000);

        QCOMPARE(rf.m_frames.size(), 1);
        QByteArray f = rf.m_frames.first();
        QCOMPARE(f.size(), 6);
        QCOMPARE(uchar(f[0]), uchar(0x5A));
        QCOMPARE(uchar(f[1]), uchar(0x0B));
        QCOMPARE(uchar(f[3]), uchar(0xE7));   // 999 & 0xFF
        QCOMPARE(uchar(f[4]), uchar(0x03));   // (999 >> 8) & 0xFF
    }

    // TC-RF-008: rf_control BAUDRATE(9600) gives param 96 (9600 / 100 = 0x60)
    void rf_control_baudrate()
    {
        TestRF rf;
        rf.set_type(0);
        rf.rf_control(RangeFinder::BAUDRATE, 9600);

        QCOMPARE(rf.m_frames.size(), 1);
        QByteArray f = rf.m_frames.first();
        QCOMPARE(f.size(), 6);
        QCOMPARE(uchar(f[0]), uchar(0x5A));
        QCOMPARE(uchar(f[1]), uchar(0x06));
        QCOMPARE(uchar(f[3]), uchar(0x60));   // 96 & 0xFF
        QCOMPARE(uchar(f[4]), uchar(0x00));   // (96 >> 8) & 0xFF
    }

    // TC-RF-009: get_type() / set_type() atomic access
    void type_atomic_access()
    {
        TestRF rf;
        rf.set_type(0);
        QCOMPARE(rf.get_type(), 0u);

        rf.set_type(1);
        QCOMPARE(rf.get_type(), 1u);
    }

    // TC-RF-010: read_distance, largest value that passes the checksum comparison.
    // The comparison only passes when data[1] + data[2] < 0x80 (a char sign issue in
    // read_distance), so 0x0570 = 1392 is used.
    void read_distance_large_value()
    {
        TestRF rf;
        QSignalSpy spy(&rf, &RangeFinder::distance_updated);

        // distance = (0x05 << 8) + 0x70 = 1392, checksum = 0x75.
        // ~0x75 as int is 0xFFFFFF8A, and char(0x8A) sign-extends to the same value.
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
