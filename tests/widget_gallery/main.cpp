// Widget gallery: a visual smoke test of the custom widgets.
// Built only with BUILD_TESTS=ON. One tab per widget, each with a log of the
// signals it emits. Not shipped with the application.

#include <QApplication>
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QTime>
#include <QVBoxLayout>

#include "widgets/arcsliderwidget.h"
#include "widgets/dotindicator.h"
#include "widgets/floatingpanel.h"
#include "widgets/joystickwidget.h"
#include "widgets/timingbarwidget.h"
#include "widgets/unlockbar.h"

namespace {

QTextEdit *make_log()
{
    auto *log = new QTextEdit;
    log->setReadOnly(true);
    log->setFixedHeight(120);
    log->setStyleSheet("background:#111;color:#ddd;font-family:monospace;");
    return log;
}

void log_line(QTextEdit *log, const QString &msg)
{
    log->append(QStringLiteral("[%1] %2")
                .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss.zzz")))
                .arg(msg));
}

QWidget *build_floating_panel_tab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *canvas = new QWidget;
    canvas->setMinimumSize(600, 400);
    canvas->setStyleSheet("background:#2a2a2a;");

    auto *panel_a = new FloatingPanel(canvas);
    panel_a->set_title(QStringLiteral("Optics"));
    panel_a->resize(220, 160);
    panel_a->move(40, 40);
    auto *label_a = new QLabel(QStringLiteral("drag me by the header"));
    panel_a->get_content_layout()->addWidget(label_a);

    auto *panel_b = new FloatingPanel(canvas);
    panel_b->set_title(QStringLiteral("TCU"));
    panel_b->resize(220, 160);
    panel_b->move(160, 120);
    auto *label_b = new QLabel(QStringLiteral("z-order rises on click"));
    panel_b->get_content_layout()->addWidget(label_b);

    auto *log = make_log();
    QObject::connect(panel_a, &FloatingPanel::pin_toggled,    [log](bool v){ log_line(log, QStringLiteral("A pin_toggled(%1)").arg(v)); });
    QObject::connect(panel_a, &FloatingPanel::hide_requested, [log](){     log_line(log, QStringLiteral("A hide_requested")); });
    QObject::connect(panel_a, &FloatingPanel::dragged,        [log](QPoint p){ log_line(log, QStringLiteral("A dragged(%1,%2)").arg(p.x()).arg(p.y())); });
    QObject::connect(panel_b, &FloatingPanel::pin_toggled,    [log](bool v){ log_line(log, QStringLiteral("B pin_toggled(%1)").arg(v)); });
    QObject::connect(panel_b, &FloatingPanel::hide_requested, [log](){     log_line(log, QStringLiteral("B hide_requested")); });
    QObject::connect(panel_b, &FloatingPanel::dragged,        [log](QPoint p){ log_line(log, QStringLiteral("B dragged(%1,%2)").arg(p.x()).arg(p.y())); });

    auto *reshow = new QPushButton(QStringLiteral("Show hidden panels"));
    QObject::connect(reshow, &QPushButton::clicked, [panel_a, panel_b](){ panel_a->show(); panel_b->show(); });

    layout->addWidget(canvas, 1);
    layout->addWidget(reshow);
    layout->addWidget(log);
    return page;
}

QWidget *build_timing_bar_tab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *bar = new TimingBarWidget;
    // the widget already has these defaults; they are set here to show typical usage
    bar->set_bar_value(TimingBarWidget::LASER_WIDTH, 100);
    bar->set_bar_value(TimingBarWidget::DISTANCE,    500);
    bar->set_bar_value(TimingBarWidget::DOV,         100);
    bar->set_bar_value(TimingBarWidget::MCP,         128);

    auto *log = make_log();
    QObject::connect(bar, &TimingBarWidget::bar_value_changed,
                     [log](TimingBarWidget::BarId id, quint64 v){
        static const char *names[] = {"LaserWidth", "Distance", "DOV", "MCP"};
        log_line(log, QStringLiteral("%1 = %2").arg(names[id]).arg(v));
    });

    layout->addWidget(new QLabel(QStringLiteral("Drag the right edge of each bar to change its value")));
    layout->addWidget(bar, 1);
    layout->addWidget(log);
    return page;
}

QWidget *build_joystick_tab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *js = new JoystickWidget;
    js->setMinimumSize(240, 240);

    auto *readout = new QLabel(QStringLiteral("(0.00, 0.00)"));
    auto *log = make_log();
    QObject::connect(js, &JoystickWidget::direction_changed,
                     [readout, log](double dx, double dy){
        readout->setText(QStringLiteral("(%1, %2)")
                         .arg(dx, 0, 'f', 3).arg(dy, 0, 'f', 3));
        log_line(log, QStringLiteral("direction_changed(%1, %2)")
                 .arg(dx, 0, 'f', 3).arg(dy, 0, 'f', 3));
    });

    layout->addWidget(js, 1, Qt::AlignCenter);
    layout->addWidget(readout, 0, Qt::AlignCenter);
    layout->addWidget(log);
    return page;
}

QWidget *build_arc_slider_tab()
{
    auto *page = new QWidget;
    auto *outer = new QVBoxLayout(page);

    auto *sliders_row = new QHBoxLayout;
    auto *log = make_log();

    static const char *labels[] = {"ZOOM", "FOCUS", "IRIS"};
    for (int i = 0; i < 3; ++i) {
        auto *col = new QVBoxLayout;
        auto *slider = new ArcSliderWidget;
        slider->set_range(0, 100);
        slider->set_value(50 + i * 10);
        slider->setMinimumHeight(240);
        QObject::connect(slider, &ArcSliderWidget::value_changed,
                         [log, i](int v){
            log_line(log, QStringLiteral("%1 = %2").arg(labels[i]).arg(v));
        });
        col->addWidget(slider, 1);
        col->addWidget(new QLabel(labels[i]), 0, Qt::AlignCenter);
        sliders_row->addLayout(col);
    }

    outer->addLayout(sliders_row, 1);
    outer->addWidget(log);
    return page;
}

QWidget *build_dot_indicator_tab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *grid = new QGridLayout;
    static const char *names[]  = {"TCU", "Camera", "Laser", "Lens", "PTZ", "Rangefinder"};
    static const DotIndicator::State states[] = {
        DotIndicator::OK, DotIndicator::OK, DotIndicator::WARN,
        DotIndicator::ERR, DotIndicator::OFF, DotIndicator::OK
    };
    QVector<DotIndicator*> dots;
    for (int i = 0; i < 6; ++i) {
        auto *dot = new DotIndicator;
        dot->set_state(states[i]);
        dots.push_back(dot);
        grid->addWidget(dot,             i, 0);
        grid->addWidget(new QLabel(names[i]), i, 1);
    }

    auto *cycle = new QPushButton(QStringLiteral("Cycle all states"));
    QObject::connect(cycle, &QPushButton::clicked, [dots]() mutable {
        static int step = 0;
        step = (step + 1) % 4;
        for (auto *d : dots) d->set_state(static_cast<DotIndicator::State>(step));
    });

    layout->addLayout(grid, 1);
    layout->addWidget(cycle);
    return page;
}

QWidget *build_unlock_bar_tab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *bar = new UnlockBar;
    bar->setMinimumWidth(400);
    auto *log = make_log();

    auto *state_label = new QLabel(QStringLiteral("state: locked"));
    auto *manual = new QPushButton(QStringLiteral("relock()"));
    QObject::connect(manual, &QPushButton::clicked, bar, &UnlockBar::relock);

    QObject::connect(bar, &UnlockBar::unlocked, [log, state_label](){
        log_line(log, QStringLiteral("unlocked()"));
        state_label->setText(QStringLiteral("state: UNLOCKED"));
    });
    QObject::connect(bar, &UnlockBar::locked, [log, state_label](){
        log_line(log, QStringLiteral("locked()"));
        state_label->setText(QStringLiteral("state: locked"));
    });

    auto *relock_ms = new QSpinBox;
    relock_ms->setRange(0, 10000);
    relock_ms->setSingleStep(500);
    relock_ms->setValue(bar->get_relock_ms());
    relock_ms->setSuffix(QStringLiteral(" ms auto-relock (0 = off)"));
    QObject::connect(relock_ms, QOverload<int>::of(&QSpinBox::valueChanged),
                     bar, &UnlockBar::set_relock_ms);

    layout->addWidget(bar);
    layout->addWidget(state_label);
    layout->addWidget(relock_ms);
    layout->addWidget(manual);
    layout->addStretch(1);
    layout->addWidget(log);
    return page;
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QMainWindow win;
    win.setWindowTitle(QStringLiteral("Sprint 15 Widget Gallery"));
    win.resize(800, 600);

    auto *tabs = new QTabWidget;
    tabs->addTab(build_floating_panel_tab(), QStringLiteral("FloatingPanel"));
    tabs->addTab(build_timing_bar_tab(),     QStringLiteral("TimingBar"));
    tabs->addTab(build_joystick_tab(),       QStringLiteral("Joystick"));
    tabs->addTab(build_arc_slider_tab(),     QStringLiteral("ArcSlider"));
    tabs->addTab(build_dot_indicator_tab(),  QStringLiteral("DotIndicator"));
    tabs->addTab(build_unlock_bar_tab(),     QStringLiteral("UnlockBar"));

    win.setCentralWidget(tabs);
    win.show();
    return app.exec();
}
