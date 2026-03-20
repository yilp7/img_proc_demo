#ifndef LENSCONTROLLER_H
#define LENSCONTROLLER_H

#include <QObject>

#include "port/lens.h"

class DeviceManager;

class LensController : public QObject
{
    Q_OBJECT

public:
    enum LensButton { BTN_ZOOM_IN, BTN_ZOOM_OUT, BTN_FOCUS_FAR, BTN_FOCUS_NEAR,
                      BTN_RADIUS_INC, BTN_RADIUS_DEC, BTN_IRIS_OPEN, BTN_IRIS_CLOSE };

    explicit LensController(DeviceManager *device_mgr, QObject *parent = nullptr);

    void init(bool *simple_ui, uchar *lang);

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

    // Set zoom/focus from value (previously read from UI)
    void set_zoom_value(uint val);
    void set_focus_value(uint val);

    // Inline helpers for auto-focus
    void focus_far();
    void focus_near();

signals:
    void send_lens_msg(qint32 lens_param, uint val = 0);
    void button_text_changed(int button_id, QString text);
    void zoom_text_updated(QString text);
    void focus_text_updated(QString text);
    void focus_speed_display_changed(int slider_val, QString edit_text);

private:
    DeviceManager*  m_device_mgr;
    bool*           m_simple_ui;
    uchar*          m_lang;

    uint            zoom;
    uint            focus;
    int             focus_direction;
    int             clarity[3];
    uint            lens_adjust_ongoing;
};

#endif // LENSCONTROLLER_H
