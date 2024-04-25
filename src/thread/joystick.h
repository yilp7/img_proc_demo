#ifndef JOYSTICKTHREAD_H
#define JOYSTICKTHREAD_H

#ifdef WIN32
#include <windows.h>
#include <MMSystem.h>
#endif
#include <QThread>
#include <QTimer>

class JoystickThread : public QThread
{
    Q_OBJECT
public:
    JoystickThread(void *info);
    ~JoystickThread();

    enum BUTTON {
        NONE     = 0x0000,
        A        = 0x0001,
        X        = 0x0002,
        B        = 0x0004,
        Y        = 0x0008,
        L1       = 0x0010,
        R1       = 0x0020,
        L2       = 0x0040,
        R2       = 0x0080,
        MINUS    = 0x0100,
        PLUS     = 0x0200,
        HANDLE_L = 0x0400,
        HANDLE_R = 0x0800,
        HOME     = 0x1000,
        SELECT   = 0x2000,
    };

    enum DIRECTION {
        UP        = 0,
        UPRIGHT   = 1,
        RIGHT     = 2,
        DOWNRIGHT = 3,
        DOWN      = 4,
        DOWNLEFT  = 5,
        LEFT      = 6,
        UPLEFT    = 7,
        STOP      = 14,
    };

protected:
    void run();

private slots:
    void process_joyinfo();

signals:
    void button_pressed(int btn);
    void button_released(int btn);
    void direction_changed(int direction);
    void pos_xy(int x, int y);
    void pos_ur(int u, int r);

private:
    void          *p_info;
    QTimer        *t;
#ifdef WIN32
    JOYINFOEX     *joy;
    MMRESULT      joy_return;
#endif

    ulong         curr_btn;
    ulong         prev_btn;
    int           curr_direction;
    int           prev_direction;
    int           x, y, u, r;
    bool          pressed[14];
};

#endif // JOYSTICKTHREAD_H
