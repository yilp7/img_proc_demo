#include "joystick.h"

JoystickThread::JoystickThread(UserPanel *panel) :
    QThread(),
    m_t(nullptr),
    m_curr_btn(0),
    m_prev_btn(0),
    m_curr_direction(14),
    m_prev_direction(14),
    m_x(0),
    m_y(0),
    m_u(0),
    m_r(0),
    m_pressed{false}
{
    m_panel = panel;
#ifdef WIN32
    m_joy = (JOYINFOEX*)malloc(sizeof(JOYINFOEX));
    m_joy->dwSize = sizeof(JOYINFOEX);
    m_joy->dwFlags = JOY_RETURNALL;
#endif
}

JoystickThread::~JoystickThread()
{
#ifdef WIN32
    free(m_joy);
#endif
}

void JoystickThread::run()
{
    if (!m_t) m_t = new QTimer();
    connect(m_t, SIGNAL(timeout()), this, SLOT(process_joyinfo()));
    m_t->start(100);
    this->exec();
//    while (1) {
//        process_joyinfo();
//        QThread::msleep(100);
//    }
}

void JoystickThread::process_joyinfo()
{
    static const int   button_id[14] = {A, X, B, Y, L1, R1, L2, R2, MINUS, PLUS, HANDLE_L, HANDLE_R, HOME, SELECT};
    static const char *button_name[14] = {"A", "X", "B", "Y", "L1", "R1", "L2", "R2", "-", "+", "LEFT HANDLE", "RIGHT HANDLE", "HOME", "SELECT"};
#ifdef WIN32
    m_joy_return = joyGetPosEx(JOYSTICKID1, m_joy);
    switch (m_joy_return) {
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
        m_curr_btn = m_joy->dwButtons ^ m_prev_btn;
        m_prev_btn = m_joy->dwButtons;
        if (!m_curr_btn) goto directions;
        for (int i = 0; i < 14; i++) {
            if (m_curr_btn & button_id[i]) {
                m_pressed[i] ^= 1;
                if (m_pressed[i]) qDebug("%s pressed", button_name[i]),  emit button_pressed(button_id[i]);
                else              qDebug("%s released", button_name[i]), emit button_released(button_id[i]);
            }
        }
directions:
        m_curr_direction = m_joy->dwPOV / 4500;
        if (m_curr_direction == m_prev_direction) goto handles;
        m_prev_direction = m_curr_direction;
        qDebug("direction %d", m_curr_direction);
        emit direction_changed(m_curr_direction);
handles:
        m_x = m_joy->dwXpos - 32767;
        m_y = m_joy->dwYpos - 32767;
        m_u = m_joy->dwUpos - 32767;
        m_r = m_joy->dwRpos - 32767;
        if (m_x | m_y) qDebug("XY (%d, %d)", m_x, m_y), emit pos_xy(m_x, m_y);
        if (m_u | m_r) qDebug("UR (%d, %d)", m_u, m_r), emit pos_ur(m_u, m_r);
        break;
    }
    default:
//        qDebug("unknown error");
        break;
    }
#endif
}
