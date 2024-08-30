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
        {ZOOM_STOP, generate_ba(new uchar[6]{0x81, 0x01, 0x04, 0x07, 0x00, 0xFF}, 6)},
        {ZOOM_IN,   generate_ba(new uchar[6]{0x81, 0x01, 0x04, 0x07, 0x20, 0xFF}, 6)},
        {ZOOM_OUT,  generate_ba(new uchar[6]{0x81, 0x01, 0x04, 0x07, 0x30, 0xFF}, 6)},
        {ZOOM_POS,  generate_ba(new uchar[5]{0x81, 0x09, 0x04, 0x47, 0xFF}, 5)},

        {FOCUS_STOP, generate_ba(new uchar[6]{0x81, 0x01, 0x04, 0x08, 0x00, 0xFF}, 6)},
        {FOCUS_FAR,  generate_ba(new uchar[6]{0x81, 0x01, 0x04, 0x08, 0x20, 0xFF}, 6)},
        {FOCUS_NEAR, generate_ba(new uchar[6]{0x81, 0x01, 0x04, 0x08, 0x30, 0xFF}, 6)},
        {FOCUS_POS,  generate_ba(new uchar[5]{0x81, 0x09, 0x04, 0x48, 0xFF}, 5)},

        {ICR_AUTO,   generate_ba(new uchar[6]{0x81, 0x01, 0x04, 0x51, 0x01, 0xFF}, 6)},
        {ICR_MANUAL, generate_ba(new uchar[6]{0x81, 0x01, 0x04, 0x51, 0x03, 0xFF}, 6)},
        {ICR_ON,     generate_ba(new uchar[6]{0x81, 0x01, 0x04, 0x01, 0x02, 0xFF}, 6)},
        {ICR_OFF,    generate_ba(new uchar[6]{0x81, 0x09, 0x04, 0x01, 0x03, 0xFF}, 6)},

        {OSD_ON,  generate_ba(new uchar[6]{0x81, 0x01, 0x04, 0x74, 0x2F, 0xFF}, 6)},
        {OSD_OFF, generate_ba(new uchar[6]{0x81, 0x01, 0x04, 0x74, 0x3F, 0xFF}, 6)},
    };

    uchar speed;
};

#endif // HUANYU_H
