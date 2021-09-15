#include "demo.h"

#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setFont(QFont("Consolas", 9));
    QFile style(":/style/style.qss");
    style.open(QIODevice::ReadOnly);
    a.setStyleSheet(style.readAll());

    Demo w;
    w.show();
    return a.exec();
}
