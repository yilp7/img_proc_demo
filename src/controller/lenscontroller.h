#ifndef LENSCONTROLLER_H
#define LENSCONTROLLER_H

#include <QObject>

#include "port/lens.h"

QT_BEGIN_NAMESPACE
namespace Ui { class UserPanel; }
QT_END_NAMESPACE

class DeviceManager;

class LensController : public QObject
{
    Q_OBJECT

public:
    explicit LensController(DeviceManager *device_mgr, QObject *parent = nullptr);

    void init(Ui::UserPanel *ui, bool *simple_ui, uchar *lang);

    // Getters
    uint get_zoom() const           { return zoom; }
    uint get_focus() const          { return focus; }
    int  get_focus_direction() const{ return focus_direction; }
    int  get_clarity(int i) const   { return clarity[i]; }
    uint get_lens_adjust_ongoing() const { return lens_adjust_ongoing; }

    // Setters
    void set_zoom(uint v)           { zoom = v; }
    void set_focus(uint v)          { focus = v; }
    void set_focus_direction(int v) { focus_direction = v; }
    void set_clarity(int i, int v)  { clarity[i] = v; }
    void set_lens_adjust_ongoing(uint v) { lens_adjust_ongoing = v; }

public slots:
    // Button pressed/released handlers
    void on_ZOOM_IN_BTN_pressed();
    void on_ZOOM_IN_BTN_released();
    void on_ZOOM_OUT_BTN_pressed();
    void on_ZOOM_OUT_BTN_released();
    void on_FOCUS_FAR_BTN_pressed();
    void on_FOCUS_FAR_BTN_released();
    void on_FOCUS_NEAR_BTN_pressed();
    void on_FOCUS_NEAR_BTN_released();
    void on_RADIUS_INC_BTN_pressed();
    void on_RADIUS_INC_BTN_released();
    void on_RADIUS_DEC_BTN_pressed();
    void on_RADIUS_DEC_BTN_released();

    void on_IRIS_OPEN_BTN_pressed();
    void on_IRIS_CLOSE_BTN_pressed();
    void on_IRIS_OPEN_BTN_released();
    void on_IRIS_CLOSE_BTN_released();

    void on_GET_LENS_PARAM_BTN_clicked();
    void on_AUTO_FOCUS_BTN_clicked();

    void change_focus_speed(int val);
    void update_lens_params(qint32 lens_param, uint val);
    void lens_stop();

    // Set zoom/focus from UI edits
    void set_zoom_from_ui();
    void set_focus_from_ui();

    // Inline helpers for auto-focus
    void focus_far();
    void focus_near();

signals:
    void send_lens_msg(qint32 lens_param, uint val = 0);

private:
    DeviceManager*  m_device_mgr;
    Ui::UserPanel*  m_ui;
    bool*           m_simple_ui;
    uchar*          m_lang;

    uint            zoom;
    uint            focus;
    int             focus_direction;
    int             clarity[3];
    uint            lens_adjust_ongoing;
};

#endif // LENSCONTROLLER_H
