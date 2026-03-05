#ifndef IPTZCONTROLLER_H
#define IPTZCONTROLLER_H

#include <QObject>

class IPTZController {
public:
    virtual ~IPTZController() = default;

    // Direction values match the 3x3 button grid layout:
    //   0=UP_LEFT,   1=UP,   2=UP_RIGHT
    //   3=LEFT,      4=SELF, 5=RIGHT
    //   6=DOWN_LEFT, 7=DOWN, 8=DOWN_RIGHT
    enum Direction {
        DIR_UP_LEFT    = 0,
        DIR_UP         = 1,
        DIR_UP_RIGHT   = 2,
        DIR_LEFT       = 3,
        DIR_SELF_CHECK = 4,
        DIR_RIGHT      = 5,
        DIR_DOWN_LEFT  = 6,
        DIR_DOWN       = 7,
        DIR_DOWN_RIGHT = 8,
    };

    virtual void ptz_move(int direction, int speed) = 0;
    virtual void ptz_stop() = 0;
    virtual void ptz_set_angle(float h, float v) = 0;
    virtual void ptz_set_angle_h(float h) = 0;
    virtual void ptz_set_angle_v(float v) = 0;
    virtual float ptz_get_angle_h() const = 0;
    virtual float ptz_get_angle_v() const = 0;
    virtual bool ptz_is_connected() const = 0;

    // All implementors must define signal: angle_updated(float h, float v)
    // Returns the QObject pointer for connect()
    virtual QObject* ptz_qobject() = 0;
};

#endif // IPTZCONTROLLER_H
