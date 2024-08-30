#include "huanyu.h"

HuanYu::HuanYu(int index, int type) {}

int HuanYu::cam_control(qint32 cam_param, uint val)
{
    QByteArray command;
    switch (cam_param)
    {
    case ZOOM_STOP:
    case ZOOM_POS:
    case SET_ZOOM:
    case FOCUS_STOP:
    case FOCUS_POS:
    case SET_FOCUS:
    case ICR_AUTO:
    case ICR_MANUAL:
    case ICR_ON:
    case ICR_OFF:
    case OSD_ON:
    case OSD_OFF:
        command = commands[cam_param];
        break;

    case ZOOM_IN:
    case ZOOM_OUT:
    case FOCUS_FAR:
    case FOCUS_NEAR:
        command = commands[cam_param];
        command[4] = command[4] + speed;
        break;

    case OSD_TEXT:
    default: return -11111;
    }

    switch (cam_param)
    {
    default:
        communicate(command, 0, 40);
        break;
    }
    return 0;

}
