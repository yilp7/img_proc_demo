#include <QtTest>
#include "port/lens.h"
#include "port/pelco_protocol.h"

// Test subclass that captures frames instead of sending them over serial/TCP
class TestLens : public Lens {
    Q_OBJECT
public:
    TestLens(uchar addr = 0x01, uint speed = 32) : Lens(-1, addr, speed) {}
    QList<QByteArray> frames;
    QByteArray mock_response;
    void communicate(QByteArray write, uint read_size, uint read_timeout, bool heartbeat) override {
        frames.append(write);
        if (read_size > 0) {
            retrieve_mutex.lock();
            last_read = mock_response;
            retrieve_mutex.unlock();
        }
    }
    using Lens::lens_control;
};

class TestLensProtocol : public QObject
{
    Q_OBJECT

private slots:
    // TC-L-001: STOP frame
    void stopFrame()
    {
        TestLens lens;
        lens.lens_control(Lens::STOP);

        QCOMPARE(lens.frames.size(), 1);
        QByteArray f = lens.frames[0];
        QCOMPARE(f.size(), 7);
        QCOMPARE((uchar)f[0], (uchar)0xFF);  // SYNC_BYTE
        QCOMPARE((uchar)f[1], (uchar)0x01);  // address
        QCOMPARE((uchar)f[2], (uchar)0x00);  // cmd byte 2
        QCOMPARE((uchar)f[3], (uchar)0x00);  // LENS_CMD_STOP
        QCOMPARE((uchar)f[4], (uchar)0x00);  // data_hi
        QCOMPARE((uchar)f[5], (uchar)0x00);  // data_lo
    }

    // TC-L-002: ZOOM_IN direction + speed
    void zoomInDirectionAndSpeed()
    {
        TestLens lens(0x01, 32);  // default speed = 32 = 0x20
        lens.lens_control(Lens::ZOOM_IN);

        QCOMPARE(lens.frames.size(), 1);
        QByteArray f = lens.frames[0];
        QCOMPARE(f.size(), 7);
        QCOMPARE((uchar)f[3], (uchar)0x20);  // LENS_CMD_ZOOM_IN
        QCOMPARE((uchar)f[5], (uchar)0x20);  // speed = 32
    }

    // TC-L-003: FOCUS_NEAR uses byte[2]
    void focusNearUsesByte2()
    {
        TestLens lens(0x01, 32);
        lens.lens_control(Lens::FOCUS_NEAR);

        QCOMPARE(lens.frames.size(), 1);
        QByteArray f = lens.frames[0];
        QCOMPARE((uchar)f[2], (uchar)0x01);  // LENS_CMD_FOCUS_NEAR in byte 2
        QCOMPARE((uchar)f[3], (uchar)0x00);  // byte 3 is zero
        QCOMPARE((uchar)f[5], (uchar)0x20);  // speed = 32
    }

    // TC-L-004: RADIUS_UP/DOWN byte[2] values
    void radiusUpDownByte2()
    {
        TestLens lens(0x01, 32);

        lens.lens_control(Lens::RADIUS_UP);
        QCOMPARE(lens.frames.size(), 1);
        QCOMPARE((uchar)lens.frames[0][2], (uchar)0x02);  // LENS_CMD_RADIUS_UP

        lens.lens_control(Lens::RADIUS_DOWN);
        QCOMPARE(lens.frames.size(), 2);
        QCOMPARE((uchar)lens.frames[1][2], (uchar)0x04);  // LENS_CMD_RADIUS_DOWN
    }

    // TC-L-005: SET_ZOOM(0x1234)
    void setZoomEncoding()
    {
        TestLens lens;
        lens.lens_control(Lens::SET_ZOOM, 0x1234);

        QCOMPARE(lens.frames.size(), 1);
        QByteArray f = lens.frames[0];
        QCOMPARE((uchar)f[3], (uchar)0x4F);  // LENS_SET_ZOOM
        QCOMPARE((uchar)f[4], (uchar)0x12);  // val >> 8
        QCOMPARE((uchar)f[5], (uchar)0x34);  // val & 0xFF
    }

    // TC-L-006: SET_FOCUS/SET_RADIUS same pattern
    void setFocusAndRadiusEncoding()
    {
        TestLens lens;

        lens.lens_control(Lens::SET_FOCUS, 0xABCD);
        QCOMPARE(lens.frames.size(), 1);
        QByteArray ff = lens.frames[0];
        QCOMPARE((uchar)ff[3], (uchar)0x4E);  // LENS_SET_FOCUS
        QCOMPARE((uchar)ff[4], (uchar)0xAB);  // val >> 8
        QCOMPARE((uchar)ff[5], (uchar)0xCD);  // val & 0xFF

        lens.lens_control(Lens::SET_RADIUS, 0x5678);
        QCOMPARE(lens.frames.size(), 2);
        QByteArray fr = lens.frames[1];
        QCOMPARE((uchar)fr[3], (uchar)0x81);  // LENS_SET_RADIUS
        QCOMPARE((uchar)fr[4], (uchar)0x56);  // val >> 8
        QCOMPARE((uchar)fr[5], (uchar)0x78);  // val & 0xFF
    }

    // TC-L-007: RELAY1_ON/OFF
    void relayOnOff()
    {
        TestLens lens;

        lens.lens_control(Lens::RELAY1_ON);
        QCOMPARE(lens.frames.size(), 1);
        QCOMPARE((uchar)lens.frames[0][3], (uchar)0x09);  // LENS_RELAY_ON

        lens.lens_control(Lens::RELAY1_OFF);
        QCOMPARE(lens.frames.size(), 2);
        QCOMPARE((uchar)lens.frames[1][3], (uchar)0x0B);  // LENS_RELAY_OFF
    }

    // TC-L-008: STEPPING(100) clamped to MAX_SPEED (63)
    void steppingClampedToMax()
    {
        TestLens lens;
        int ret = lens.lens_control(Lens::STEPPING, 100);

        QCOMPARE(ret, 0);
        QCOMPARE(lens.frames.size(), 0);  // no frame sent
        QCOMPARE(lens.get(Lens::STEPPING), (uint)63);  // MAX_SPEED = 0x3F = 63
    }

    // TC-L-009: Query frames use correct command bytes
    void queryFrameEncoding()
    {
        TestLens lens;
        // Prepare a mock response so the query path completes
        lens.mock_response = QByteArray(7, '\0');

        // ZOOM_POS query
        lens.lens_control(Lens::ZOOM_POS);
        QCOMPARE(lens.frames.size(), 1);
        QCOMPARE((uchar)lens.frames[0][3], (uchar)0x55);  // LENS_QUERY_ZOOM

        // FOCUS_POS query
        lens.lens_control(Lens::FOCUS_POS);
        QCOMPARE(lens.frames.size(), 2);
        QCOMPARE((uchar)lens.frames[1][3], (uchar)0x56);  // LENS_QUERY_FOCUS

        // LASER_RADIUS query
        lens.lens_control(Lens::LASER_RADIUS);
        QCOMPARE(lens.frames.size(), 3);
        QCOMPARE((uchar)lens.frames[2][3], (uchar)0x57);  // LENS_QUERY_RADIUS
    }

    // TC-L-010: Checksum verification
    void checksumVerification()
    {
        TestLens lens(0x01);
        lens.lens_control(Lens::STOP);

        QCOMPARE(lens.frames.size(), 1);
        QByteArray f = lens.frames[0];
        // Manual checksum: sum starts at 1, add all 7 bytes (byte[6] = 0x00 placeholder)
        // sum = 1 + 0xFF + 0x01 + 0x00 + 0x00 + 0x00 + 0x00 + 0x00 = 0x101
        // checksum = 0x101 & 0xFF = 0x01
        QCOMPARE((uchar)f[6], (uchar)0x01);

        // Also verify by recomputing: sum(1 + all bytes except last) & 0xFF
        uint sum = 1;
        for (int i = 0; i < 6; i++)
            sum += (uchar)f[i];
        uchar expected = sum & 0xFF;
        QCOMPARE((uchar)f[6], expected);
    }
};

QTEST_MAIN(TestLensProtocol)
#include "test_lens.moc"
