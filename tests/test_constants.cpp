#include <QtTest>
#include "util/constants.h"
#include "port/pelco_protocol.h"
#include "port/tcu_protocol.h"

class TestConstants : public QObject
{
    Q_OBJECT

private slots:
    // TC-C-001: Pelco shared protocol constants
    void pelcoSharedConstants()
    {
        QCOMPARE(SYNC_BYTE, (unsigned char)0xFF);
        QCOMPARE(MIN_SPEED, (unsigned char)0x01);
        QCOMPARE(MAX_SPEED, (unsigned char)0x3F);
        QCOMPARE(READ_TIMEOUT_MS, 40);
    }

    // TC-C-002: Angle encoding constants
    void angleConstants()
    {
        QCOMPARE(ANGLE_SCALE, 100);
        QCOMPARE(FULL_ROTATION, 36000);
        QCOMPARE(VERTICAL_LIMIT, 4000);
        QCOMPARE(VERTICAL_DEG, 40.0);
    }

    // TC-C-003: TCU protocol constants
    void tcuProtocolConstants()
    {
        QCOMPARE(TCU_HEAD, (unsigned char)0x88);
        QCOMPARE(TCU_TAIL, (unsigned char)0x99);
        QCOMPARE(TCU_QUERY_CMD, (unsigned char)0x15);
        QCOMPARE(TCU_CLOCK_8, 8);
        QCOMPARE(TCU_CLOCK_4, 4);
    }

    // TC-C-004: Physics constant
    void physicsConstant()
    {
        double expected = 3e8 * 1e-9 / 2; // ~0.15 m/ns
        QCOMPARE(LIGHT_SPEED_M_NS, expected);
    }

    // TC-C-005: Pipeline constants
    void pipelineConstants()
    {
        QCOMPARE(MAX_QUEUE_SIZE, 5);
        QCOMPARE(AUTO_MCP_DIVISOR, 200);
        QCOMPARE(THROTTLE_MS, 40);
        QCOMPARE(MAX_MCP_8BIT, 255);
        QCOMPARE(MAX_MCP_12BIT, 4095);
    }
};

QTEST_MAIN(TestConstants)
#include "test_constants.moc"
