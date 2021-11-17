#include "demo.h"

#include <QApplication>
#include <QTranslator>
#include <QTextCodec>
#include <QStandardPaths>

void log_message(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex mutex;
    mutex.lock();
    QString text = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ");
    switch(type)
    {
    case QtDebugMsg:
        text += QString("[Debug] ");
        break;
    case QtWarningMsg:
        text += QString("[Warning] ");
        break;
    case QtCriticalMsg:
        text += QString("[Critical] ");
        break;
    case QtFatalMsg:
        text += QString("[Fatal] ");
        break;
    default:
        break;
    }

//    QString message = text + QString::asprintf("File:(%s) Line (%d), ", context.file, context.line) + msg + "\n";
    static QFile file("../log");
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    file.write((text + msg + "\n").toUtf8());
    file.flush();
    file.close();
    mutex.unlock();
}


int buggyFunc() {
    delete reinterpret_cast<QString*>(0xFEE1DEAD);
    return 0;
}

int main(int argc, char *argv[])
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    QApplication a(argc, argv);

    int id = QFontDatabase::addApplicationFont(":/fonts/monaco.ttf");
    monaco = QFont(QFontDatabase::applicationFontFamilies(id).at(0));
    monaco.setPointSizeF(8.6);
//    monaco.setLetterSpacing(QFont::PercentageSpacing, 120);

    id = QFontDatabase::addApplicationFont(":/fonts/consola.ttf");
    consolas = QFont(QFontDatabase::applicationFontFamilies(id).at(0), 9);
//    monaco.setLetterSpacing(QFont::PercentageSpacing, 120);

//    a.setFont(QFont("Consolas", 9));
    a.setFont(monaco);
    QFile style(":/style/style.qss");
    style.open(QIODevice::ReadOnly);
    a.setStyleSheet(style.readAll());

//    qInstallMessageHandler(log_message);

//    buggyFunc();

    Demo w;
    w.show();
    return a.exec();
}
