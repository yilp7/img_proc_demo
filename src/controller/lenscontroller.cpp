#include "controller/lenscontroller.h"
#include "controller/devicemanager.h"

LensController::LensController(DeviceManager *device_mgr, QObject *parent) :
    QObject(parent),
    m_device_mgr(device_mgr),
    m_simple_ui(nullptr),
    m_lang(nullptr),
    m_zoom(0),
    m_focus(0),
    m_focus_direction(0),
    m_lens_adjust_ongoing(0)
{
    m_clarity[0] = m_clarity[1] = m_clarity[2] = 0;
}

void LensController::init(bool *simple_ui, uchar *lang)
{
    m_simple_ui = simple_ui;
    m_lang = lang;
}

void LensController::on_ZOOM_IN_BTN_pressed()
{
    emit button_text_changed(BTN_ZOOM_IN, "x");
    emit send_lens_msg(Lens::ZOOM_IN);
}

void LensController::on_ZOOM_OUT_BTN_pressed()
{
    emit button_text_changed(BTN_ZOOM_OUT, "x");
    emit send_lens_msg(Lens::ZOOM_OUT);
}

void LensController::on_FOCUS_FAR_BTN_pressed()
{
    emit button_text_changed(BTN_FOCUS_FAR, "x");
    focus_far();
}

void LensController::on_FOCUS_NEAR_BTN_pressed()
{
    emit button_text_changed(BTN_FOCUS_NEAR, "x");
    focus_near();
}

void LensController::on_RADIUS_INC_BTN_pressed()
{
    emit button_text_changed(BTN_RADIUS_INC, "x");
    emit send_lens_msg(Lens::RADIUS_UP);
}

void LensController::on_RADIUS_DEC_BTN_pressed()
{
    emit button_text_changed(BTN_RADIUS_DEC, "x");
    emit send_lens_msg(Lens::RADIUS_DOWN);
}

void LensController::on_ZOOM_IN_BTN_released()
{
    emit button_text_changed(BTN_ZOOM_IN, (*m_simple_ui & *m_lang) ? tr("IN") : "+");
    lens_stop();
}

void LensController::on_ZOOM_OUT_BTN_released()
{
    emit button_text_changed(BTN_ZOOM_OUT, (*m_simple_ui & *m_lang) ? tr("OUT") : "-");
    lens_stop();
}

void LensController::on_FOCUS_NEAR_BTN_released()
{
    emit button_text_changed(BTN_FOCUS_NEAR, (*m_simple_ui & *m_lang) ? tr("NEAR") : "+");
    lens_stop();
}

void LensController::on_FOCUS_FAR_BTN_released()
{
    emit button_text_changed(BTN_FOCUS_FAR, (*m_simple_ui & *m_lang) ? tr("FAR") : "-");
    lens_stop();
}

void LensController::on_RADIUS_INC_BTN_released()
{
    emit button_text_changed(BTN_RADIUS_INC, (*m_simple_ui & *m_lang) ? tr("INC") : "+");
    lens_stop();
}

void LensController::on_RADIUS_DEC_BTN_released()
{
    emit button_text_changed(BTN_RADIUS_DEC, (*m_simple_ui & *m_lang) ? tr("DEC") : "-");
    lens_stop();
}

void LensController::on_IRIS_OPEN_BTN_pressed()
{
    emit button_text_changed(BTN_IRIS_OPEN, "x");
    emit send_lens_msg(Lens::RADIUS_UP);
}

void LensController::on_IRIS_CLOSE_BTN_pressed()
{
    emit button_text_changed(BTN_IRIS_CLOSE, "x");
    emit send_lens_msg(Lens::RADIUS_DOWN);
}

void LensController::on_IRIS_OPEN_BTN_released()
{
    emit button_text_changed(BTN_IRIS_OPEN, "+");
    lens_stop();
}

void LensController::on_IRIS_CLOSE_BTN_released()
{
    emit button_text_changed(BTN_IRIS_CLOSE, "-");
    lens_stop();
}

void LensController::lens_stop()
{
    emit send_lens_msg(Lens::STOP);
}

void LensController::focus_far()
{
    emit send_lens_msg(Lens::FOCUS_FAR);
}

void LensController::focus_near()
{
    emit send_lens_msg(Lens::FOCUS_NEAR);
}

void LensController::set_zoom_value(uint val)
{
    m_zoom = val;
    emit send_lens_msg(Lens::SET_ZOOM, m_zoom);
}

void LensController::set_focus_value(uint val)
{
    m_focus = val;
    emit send_lens_msg(Lens::SET_FOCUS, m_focus);
}

void LensController::set_iris_value(uint val)
{
    emit send_lens_msg(Lens::SET_RADIUS, val);
}

void LensController::change_focus_speed(int val)
{
    if (val < 1)  val = 1;
    if (val > 63) val = 63;
    emit focus_speed_display_changed(val, QString::asprintf("%d", val));
    emit send_lens_msg(Lens::STEPPING, val);
}

void LensController::update_lens_params(qint32 lens_param, uint val)
{
    switch (lens_param) {
    case Lens::ZOOM_POS:     emit zoom_text_updated (QString::number(val)); break;
    case Lens::FOCUS_POS:    emit focus_text_updated(QString::number(val)); break;
    case Lens::LASER_RADIUS: emit iris_text_updated (QString::number(val)); break;
    case Lens::NO_PARAM:
        update_lens_params(Lens::ZOOM_POS,     m_device_mgr->get_lens()->get(Lens::ZOOM_POS));
        update_lens_params(Lens::FOCUS_POS,    m_device_mgr->get_lens()->get(Lens::FOCUS_POS));
        update_lens_params(Lens::LASER_RADIUS, m_device_mgr->get_lens()->get(Lens::LASER_RADIUS));
        emit focus_speed_display_changed(std::round(m_device_mgr->get_lens()->get(Lens::STEPPING)), "");
    default:break;
    }
}

void LensController::on_GET_LENS_PARAM_BTN_clicked()
{
    emit send_lens_msg(Lens::ZOOM_POS);
    emit send_lens_msg(Lens::FOCUS_POS);
    emit send_lens_msg(Lens::LASER_RADIUS);
}

void LensController::on_AUTO_FOCUS_BTN_clicked()
{
    m_focus_direction = 1;
    focus_far();
}

void LensController::on_LENS_ON_BTN_toggled(bool on)
{
    // Pelco-D Aux 1 on / off over the lens port, sent through send_lens_msg like
    // zoom and focus, so it uses the same queue, hold flag and address.
    emit send_lens_msg(on ? Lens::RELAY1_ON : Lens::RELAY1_OFF);
}

void LensController::set_modern_speed(ModernAxis axis, int signed_speed)
{
    if (axis < 0 || axis >= MODERN_AXIS_COUNT) return;

    const int last = m_modern_last_speed[axis];
    if (signed_speed == last) return;

    // stop: release or midpoint
    if (signed_speed == 0) {
        emit send_lens_msg(Lens::STOP);
        m_modern_last_speed[axis] = 0;
        return;
    }

    // Direction flip mid-drag: insert a STOP so the device sees a clean change.
    if (last != 0 && ((last > 0) != (signed_speed > 0))) {
        emit send_lens_msg(Lens::STOP);
    }

    const uint mag = static_cast<uint>(qAbs(signed_speed));
    emit send_lens_msg(Lens::STEPPING, mag);

    qint32 dir = Lens::STOP;
    switch (axis) {
    case ZOOM:  dir = (signed_speed > 0) ? Lens::ZOOM_IN    : Lens::ZOOM_OUT;    break;
    case FOCUS: dir = (signed_speed > 0) ? Lens::FOCUS_NEAR : Lens::FOCUS_FAR;   break;
    case IRIS:  dir = (signed_speed > 0) ? Lens::RADIUS_UP  : Lens::RADIUS_DOWN; break;
    default: return;
    }
    emit send_lens_msg(dir);
    m_modern_last_speed[axis] = signed_speed;
}

void LensController::restore_classic_stepping(int val)
{
    // Send STEPPING without focus_speed_display_changed: the classic slider already
    // shows the value, this only re-syncs the device.
    if (val < 1)  val = 1;
    if (val > 63) val = 63;
    emit send_lens_msg(Lens::STEPPING, static_cast<uint>(val));
}
