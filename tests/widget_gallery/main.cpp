// Widget Gallery — visual smoke test for Sprint 15 custom widgets.
//
// Build only when BUILD_TESTS=ON. Launches a tabbed window with one tab per
// widget; each tab shows the widget plus a log of emitted signals. Not
// shipped with the main app.

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

QTextEdit* makeLog()
{
    auto *log = new QTextEdit;
    log->setReadOnly(true);
    log->setFixedHeight(120);
    log->setStyleSheet("background:#111;color:#ddd;font-family:monospace;");
    return log;
}

void logLine(QTextEdit *log, const QString &msg)
{
    log->append(QStringLiteral("[%1] %2")
                .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss.zzz")))
                .arg(msg));
}

QWidget* buildFloatingPanelTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *canvas = new QWidget;
    canvas->setMinimumSize(600, 400);
    canvas->setStyleSheet("background:#2a2a2a;");

    auto *panelA = new FloatingPanel(canvas);
    panelA->setTitle(QStringLiteral("Optics"));
    panelA->resize(220, 160);
    panelA->move(40, 40);
    auto *labelA = new QLabel(QStringLiteral("drag me by the header"));
    panelA->contentLayout()->addWidget(labelA);

    auto *panelB = new FloatingPanel(canvas);
    panelB->setTitle(QStringLiteral("TCU"));
    panelB->resize(220, 160);
    panelB->move(160, 120);
    auto *labelB = new QLabel(QStringLiteral("z-order rises on click"));
    panelB->contentLayout()->addWidget(labelB);

    auto *log = makeLog();
    QObject::connect(panelA, &FloatingPanel::pinToggled,    [log](bool v){ logLine(log, QStringLiteral("A pinToggled(%1)").arg(v)); });
    QObject::connect(panelA, &FloatingPanel::hideRequested, [log](){     logLine(log, QStringLiteral("A hideRequested")); });
    QObject::connect(panelA, &FloatingPanel::dragged,       [log](QPoint p){ logLine(log, QStringLiteral("A dragged(%1,%2)").arg(p.x()).arg(p.y())); });
    QObject::connect(panelB, &FloatingPanel::pinToggled,    [log](bool v){ logLine(log, QStringLiteral("B pinToggled(%1)").arg(v)); });
    QObject::connect(panelB, &FloatingPanel::hideRequested, [log](){     logLine(log, QStringLiteral("B hideRequested")); });
    QObject::connect(panelB, &FloatingPanel::dragged,       [log](QPoint p){ logLine(log, QStringLiteral("B dragged(%1,%2)").arg(p.x()).arg(p.y())); });

    auto *reshow = new QPushButton(QStringLiteral("Show hidden panels"));
    QObject::connect(reshow, &QPushButton::clicked, [panelA, panelB](){ panelA->show(); panelB->show(); });

    layout->addWidget(canvas, 1);
    layout->addWidget(reshow);
    layout->addWidget(log);
    return page;
}

QWidget* buildTimingBarTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *bar = new TimingBarWidget;
    // Defaults already set by the widget — explicit values here just to show
    // the typical usage pattern.
    bar->setBarValue(TimingBarWidget::LaserWidth, 100);
    bar->setBarValue(TimingBarWidget::Distance,   500);
    bar->setBarValue(TimingBarWidget::Dov,        100);
    bar->setBarValue(TimingBarWidget::Mcp,        128);

    auto *log = makeLog();
    QObject::connect(bar, &TimingBarWidget::barValueChanged,
                     [log](TimingBarWidget::BarId id, quint64 v){
        static const char *names[] = {"LaserWidth", "Distance", "DOV", "MCP"};
        logLine(log, QStringLiteral("%1 = %2").arg(names[id]).arg(v));
    });

    layout->addWidget(new QLabel(QStringLiteral("Drag the right edge of each bar to change its value")));
    layout->addWidget(bar, 1);
    layout->addWidget(log);
    return page;
}

QWidget* buildJoystickTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *js = new JoystickWidget;
    js->setMinimumSize(240, 240);

    auto *readout = new QLabel(QStringLiteral("(0.00, 0.00)"));
    auto *log = makeLog();
    QObject::connect(js, &JoystickWidget::directionChanged,
                     [readout, log](double dx, double dy){
        readout->setText(QStringLiteral("(%1, %2)")
                         .arg(dx, 0, 'f', 3).arg(dy, 0, 'f', 3));
        logLine(log, QStringLiteral("directionChanged(%1, %2)")
                .arg(dx, 0, 'f', 3).arg(dy, 0, 'f', 3));
    });

    layout->addWidget(js, 1, Qt::AlignCenter);
    layout->addWidget(readout, 0, Qt::AlignCenter);
    layout->addWidget(log);
    return page;
}

QWidget* buildArcSliderTab()
{
    auto *page = new QWidget;
    auto *outer = new QVBoxLayout(page);

    auto *sliders_row = new QHBoxLayout;
    auto *log = makeLog();

    static const char *labels[] = {"ZOOM", "FOCUS", "IRIS"};
    for (int i = 0; i < 3; ++i) {
        auto *col = new QVBoxLayout;
        auto *slider = new ArcSliderWidget;
        slider->setRange(0, 100);
        slider->setValue(50 + i * 10);
        slider->setMinimumHeight(240);
        QObject::connect(slider, &ArcSliderWidget::valueChanged,
                         [log, i](int v){
            logLine(log, QStringLiteral("%1 = %2").arg(labels[i]).arg(v));
        });
        col->addWidget(slider, 1);
        col->addWidget(new QLabel(labels[i]), 0, Qt::AlignCenter);
        sliders_row->addLayout(col);
    }

    outer->addLayout(sliders_row, 1);
    outer->addWidget(log);
    return page;
}

QWidget* buildDotIndicatorTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *grid = new QGridLayout;
    static const char *names[]  = {"TCU", "Camera", "Laser", "Lens", "PTZ", "Rangefinder"};
    static const DotIndicator::State states[] = {
        DotIndicator::Ok, DotIndicator::Ok, DotIndicator::Warn,
        DotIndicator::Err, DotIndicator::Off, DotIndicator::Ok
    };
    QVector<DotIndicator*> dots;
    for (int i = 0; i < 6; ++i) {
        auto *dot = new DotIndicator;
        dot->setState(states[i]);
        dots.push_back(dot);
        grid->addWidget(dot,             i, 0);
        grid->addWidget(new QLabel(names[i]), i, 1);
    }

    auto *cycle = new QPushButton(QStringLiteral("Cycle all states"));
    QObject::connect(cycle, &QPushButton::clicked, [dots]() mutable {
        static int step = 0;
        step = (step + 1) % 4;
        for (auto *d : dots) d->setState(static_cast<DotIndicator::State>(step));
    });

    layout->addLayout(grid, 1);
    layout->addWidget(cycle);
    return page;
}

QWidget* buildUnlockBarTab()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *bar = new UnlockBar;
    bar->setMinimumWidth(400);
    auto *log = makeLog();

    auto *state_label = new QLabel(QStringLiteral("state: locked"));
    auto *manual = new QPushButton(QStringLiteral("relock()"));
    QObject::connect(manual, &QPushButton::clicked, bar, &UnlockBar::relock);

    QObject::connect(bar, &UnlockBar::unlocked, [log, state_label](){
        logLine(log, QStringLiteral("unlocked()"));
        state_label->setText(QStringLiteral("state: UNLOCKED"));
    });
    QObject::connect(bar, &UnlockBar::locked, [log, state_label](){
        logLine(log, QStringLiteral("locked()"));
        state_label->setText(QStringLiteral("state: locked"));
    });

    auto *relock_ms = new QSpinBox;
    relock_ms->setRange(0, 10000);
    relock_ms->setSingleStep(500);
    relock_ms->setValue(bar->relockMs());
    relock_ms->setSuffix(QStringLiteral(" ms auto-relock (0 = off)"));
    QObject::connect(relock_ms, QOverload<int>::of(&QSpinBox::valueChanged),
                     bar, &UnlockBar::setRelockMs);

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
    tabs->addTab(buildFloatingPanelTab(), QStringLiteral("FloatingPanel"));
    tabs->addTab(buildTimingBarTab(),     QStringLiteral("TimingBar"));
    tabs->addTab(buildJoystickTab(),      QStringLiteral("Joystick"));
    tabs->addTab(buildArcSliderTab(),     QStringLiteral("ArcSlider"));
    tabs->addTab(buildDotIndicatorTab(),  QStringLiteral("DotIndicator"));
    tabs->addTab(buildUnlockBarTab(),     QStringLiteral("UnlockBar"));

    win.setCentralWidget(tabs);
    win.show();
    return app.exec();
}
