#include "joystick.h"

JoystickThread::JoystickThread(void *info) : QThread(),
    t(NULL),
    curr_btn(0),
    prev_btn(0),
    curr_direction(14),
    prev_direction(14),
    x(0),
    y(0),
    u(0),
    r(0),
    pressed{false}
{
    p_info = info;
#ifdef WIN32
    joy = (JOYINFOEX*)malloc(sizeof(JOYINFOEX));
    joy->dwSize = sizeof(JOYINFOEX);
    joy->dwFlags = JOY_RETURNALL;
#endif
}

JoystickThread::~JoystickThread()
{
#ifdef WIN32
    free(joy);
#endif
}

void JoystickThread::run()
{
    if (!t) t = new QTimer();
    connect(t, SIGNAL(timeout()), this, SLOT(process_joyinfo()));
    t->start(100);
    this->exec();
//    while (1) {
//        process_joyinfo();
//        QThread::msleep(100);
//    }
}

void JoystickThread::process_joyinfo()
{
    static const int   button_id[14] = {A, X, B, Y, L1, R1, L2, R2, MINUS, PLUS, HANDLE_L, HANDLE_R, HOME, SELECT};
    static const char* button_name[14] = {"A", "X", "B", "Y", "L1", "R1", "L2", "R2", "-", "+", "LEFT HANDLE", "RIGHT HANDLE", "HOME", "SELECT"};
#ifdef WIN32
    joy_return = joyGetPosEx(JOYSTICKID1, joy);
    switch (joy_return) {
    case JOYERR_PARMS :
//        qDebug("bad parameter");
        break;
    case JOYERR_NOCANDO :
//        qDebug("request not completed");
        break;
    case JOYERR_UNPLUGGED :
//        qDebug("unplugged");
        break;
    case JOYERR_NOERROR: {
buttons:
        curr_btn = joy->dwButtons ^ prev_btn;
        prev_btn = joy->dwButtons;
        if (!curr_btn) goto directions;
        for (int i = 0; i < 14; i++) {
            if (curr_btn & button_id[i]) {
                pressed[i] ^= 1;
                if (pressed[i]) qDebug("%s pressed", button_name[i]),  emit button_pressed(button_id[i]);
                else            qDebug("%s released", button_name[i]), emit button_released(button_id[i]);
            }
        }
directions:
        curr_direction = joy->dwPOV / 4500;
        if (curr_direction == prev_direction) goto handles;
        prev_direction = curr_direction;
        qDebug("direction %d", curr_direction);
        emit direction_changed(curr_direction);
handles:
        x = joy->dwXpos - 32767;
        y = joy->dwYpos - 32767;
        u = joy->dwUpos - 32767;
        r = joy->dwRpos - 32767;
        if (x | y) qDebug("XY (%d, %d)", x, y), emit pos_xy(x, y);
        if (u | r) qDebug("UR (%d, %d)", u, r), emit pos_ur(u, r);
        break;
    }
    default:
//        qDebug("unknown error");
        break;
    }
#endif
}
