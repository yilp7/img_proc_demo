#include "controller/lenscontroller.h"
#include "controller/devicemanager.h"
#include "ui_user_panel.h"

LensController::LensController(DeviceManager *device_mgr, QObject *parent)
    : QObject(parent),
      m_device_mgr(device_mgr),
      m_ui(nullptr),
      m_simple_ui(nullptr),
      m_lang(nullptr),
      zoom(0),
      focus(0),
      focus_direction(0),
      lens_adjust_ongoing(0)
{
    clarity[0] = clarity[1] = clarity[2] = 0;
}

void LensController::init(Ui::UserPanel *ui, bool *simple_ui, uchar *lang)
{
    m_ui = ui;
    m_simple_ui = simple_ui;
    m_lang = lang;
}

// --- Button pressed handlers ---

void LensController::on_ZOOM_IN_BTN_pressed()
{
    m_ui->ZOOM_IN_BTN->setText("x");
    emit send_lens_msg(Lens::ZOOM_IN);
}

void LensController::on_ZOOM_OUT_BTN_pressed()
{
    m_ui->ZOOM_OUT_BTN->setText("x");
    emit send_lens_msg(Lens::ZOOM_OUT);
}

void LensController::on_FOCUS_FAR_BTN_pressed()
{
    m_ui->FOCUS_FAR_BTN->setText("x");
    focus_far();
}

void LensController::on_FOCUS_NEAR_BTN_pressed()
{
    m_ui->FOCUS_NEAR_BTN->setText("x");
    focus_near();
}

void LensController::on_RADIUS_INC_BTN_pressed()
{
    m_ui->RADIUS_INC_BTN->setText("x");
    emit send_lens_msg(Lens::RADIUS_UP);
}

void LensController::on_RADIUS_DEC_BTN_pressed()
{
    m_ui->RADIUS_DEC_BTN->setText("x");
    emit send_lens_msg(Lens::RADIUS_DOWN);
}

// --- Button released handlers ---

void LensController::on_ZOOM_IN_BTN_released()
{
    m_ui->ZOOM_IN_BTN->setText((*m_simple_ui & *m_lang) ? tr("IN") : "+");
    lens_stop();
}

void LensController::on_ZOOM_OUT_BTN_released()
{
    m_ui->ZOOM_OUT_BTN->setText((*m_simple_ui & *m_lang) ? tr("OUT") : "-");
    lens_stop();
}

void LensController::on_FOCUS_NEAR_BTN_released()
{
    m_ui->FOCUS_NEAR_BTN->setText((*m_simple_ui & *m_lang) ? tr("NEAR") : "+");
    lens_stop();
}

void LensController::on_FOCUS_FAR_BTN_released()
{
    m_ui->FOCUS_FAR_BTN->setText((*m_simple_ui & *m_lang) ? tr("FAR") : "-");
    lens_stop();
}

void LensController::on_RADIUS_INC_BTN_released()
{
    m_ui->RADIUS_INC_BTN->setText((*m_simple_ui & *m_lang) ? tr("INC") : "+");
    lens_stop();
}

void LensController::on_RADIUS_DEC_BTN_released()
{
    m_ui->RADIUS_DEC_BTN->setText((*m_simple_ui & *m_lang) ? tr("DEC") : "-");
    lens_stop();
}

// --- IRIS handlers ---

void LensController::on_IRIS_OPEN_BTN_pressed()
{
    m_ui->IRIS_OPEN_BTN->setText("x");
    emit send_lens_msg(Lens::RADIUS_UP);
}

void LensController::on_IRIS_CLOSE_BTN_pressed()
{
    m_ui->IRIS_CLOSE_BTN->setText("x");
    emit send_lens_msg(Lens::RADIUS_DOWN);
}

void LensController::on_IRIS_OPEN_BTN_released()
{
    m_ui->IRIS_OPEN_BTN->setText("+");
    lens_stop();
}

void LensController::on_IRIS_CLOSE_BTN_released()
{
    m_ui->IRIS_CLOSE_BTN->setText("-");
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

void LensController::set_zoom_from_ui()
{
    zoom = m_ui->ZOOM_EDIT->text().toUInt();
    emit send_lens_msg(Lens::SET_ZOOM, zoom);
}

void LensController::set_focus_from_ui()
{
    focus = m_ui->FOCUS_EDIT->text().toUInt();
    emit send_lens_msg(Lens::SET_FOCUS, focus);
}

void LensController::change_focus_speed(int val)
{
    if (val < 1)  val = 1;
    if (val > 63) val = 63;
    m_ui->FOCUS_SPEED_EDIT->setText(QString::asprintf("%d", val));
    m_ui->FOCUS_SPEED_SLIDER->setValue(val);
    emit send_lens_msg(Lens::STEPPING, val);
}

void LensController::update_lens_params(qint32 lens_param, uint val)
{
    switch (lens_param)
    {
        case Lens::ZOOM_POS:  if (!m_ui->ZOOM_EDIT->hasFocus())  m_ui->ZOOM_EDIT->setText(QString::number(val)); break;
        case Lens::FOCUS_POS: if (!m_ui->FOCUS_EDIT->hasFocus()) m_ui->FOCUS_EDIT->setText(QString::number(val)); break;
        case Lens::LASER_RADIUS: qDebug() << "laser radius" << val; break;
        case Lens::NO_PARAM:
            update_lens_params(Lens::ZOOM_POS, m_device_mgr->lens()->get(Lens::ZOOM_POS));
            update_lens_params(Lens::FOCUS_POS, m_device_mgr->lens()->get(Lens::FOCUS_POS));
            m_ui->FOCUS_SPEED_SLIDER->setValue(std::round(m_device_mgr->lens()->get(Lens::STEPPING)));
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
