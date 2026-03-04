#ifndef HUANYU_H
#define HUANYU_H

#include "controlport.h"

class HuanYu : public ControlPort
{
    Q_OBJECT
public:
    enum PARAMS {
        ZOOM_STOP  = 0x01,
        ZOOM_IN    = 0x02,
        ZOOM_OUT   = 0x03,
        ZOOM_POS   = 0x04,
        SET_ZOOM   = 0x05,

        FOCUS_STOP = 0x06,
        FOCUS_FAR  = 0x07, // need verification
        FOCUS_NEAR = 0x08, // need verification
        FOCUS_POS  = 0x09,
        SET_FOCUS  = 0x0A,

        ICR_AUTO   = 0x0B,
        ICR_MANUAL = 0x0C,
        ICR_ON     = 0x0D,
        ICR_OFF    = 0x0E,

        OSD_ON     = 0x10,
        OSD_OFF    = 0x11,
        OSD_TEXT   = 0x12,
    };
    explicit HuanYu(int index = -1, int type = 0);

protected slots:
    int cam_control(qint32 cam_param, uint val = 0);

private:
    std::map<int, QByteArray> commands = {
        {ZOOM_STOP, QByteArray("\x81\x01\x04\x07\x00\xFF", 6)},
        {ZOOM_IN,   QByteArray("\x81\x01\x04\x07\x20\xFF", 6)},
        {ZOOM_OUT,  QByteArray("\x81\x01\x04\x07\x30\xFF", 6)},
        {ZOOM_POS,  QByteArray("\x81\x09\x04\x47\xFF", 5)},

        {FOCUS_STOP, QByteArray("\x81\x01\x04\x08\x00\xFF", 6)},
        {FOCUS_FAR,  QByteArray("\x81\x01\x04\x08\x20\xFF", 6)},
        {FOCUS_NEAR, QByteArray("\x81\x01\x04\x08\x30\xFF", 6)},
        {FOCUS_POS,  QByteArray("\x81\x09\x04\x48\xFF", 5)},

        {ICR_AUTO,   QByteArray("\x81\x01\x04\x51\x01\xFF", 6)},
        {ICR_MANUAL, QByteArray("\x81\x01\x04\x51\x03\xFF", 6)},
        {ICR_ON,     QByteArray("\x81\x01\x04\x01\x02\xFF", 6)},
        {ICR_OFF,    QByteArray("\x81\x09\x04\x01\x03\xFF", 6)},

        {OSD_ON,  QByteArray("\x81\x01\x04\x74\x2F\xFF", 6)},
        {OSD_OFF, QByteArray("\x81\x01\x04\x74\x3F\xFF", 6)},
    };

    uchar speed;
};

#endif // HUANYU_H
