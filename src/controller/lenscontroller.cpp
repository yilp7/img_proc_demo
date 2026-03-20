#include "controller/lenscontroller.h"
#include "controller/devicemanager.h"

LensController::LensController(DeviceManager *device_mgr, QObject *parent)
    : QObject(parent),
      m_device_mgr(device_mgr),
      m_simple_ui(nullptr),
      m_lang(nullptr),
      zoom(0),
      focus(0),
      focus_direction(0),
      lens_adjust_ongoing(0)
{
    clarity[0] = clarity[1] = clarity[2] = 0;
}

void LensController::init(bool *simple_ui, uchar *lang)
{
    m_simple_ui = simple_ui;
    m_lang = lang;
}

// --- Button pressed handlers ---

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

// --- Button released handlers ---

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

// --- IRIS handlers ---

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

// --- Core lens methods ---

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
    zoom = val;
    emit send_lens_msg(Lens::SET_ZOOM, zoom);
}

void LensController::set_focus_value(uint val)
{
    focus = val;
    emit send_lens_msg(Lens::SET_FOCUS, focus);
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
    switch (lens_param)
    {
        case Lens::ZOOM_POS:  emit zoom_text_updated(QString::number(val)); break;
        case Lens::FOCUS_POS: emit focus_text_updated(QString::number(val)); break;
        case Lens::LASER_RADIUS: qDebug() << "laser radius" << val; break;
        case Lens::NO_PARAM:
            update_lens_params(Lens::ZOOM_POS, m_device_mgr->lens()->get(Lens::ZOOM_POS));
            update_lens_params(Lens::FOCUS_POS, m_device_mgr->lens()->get(Lens::FOCUS_POS));
            emit focus_speed_display_changed(std::round(m_device_mgr->lens()->get(Lens::STEPPING)), "");
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
    focus_direction = 1;
    focus_far();
}
