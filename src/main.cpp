#include "demo.h"

#ifdef WIN32
#include <DbgHelp.h>

#if BUILD_STATIC
#include <QtCore/QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#pragma comment(lib, "user32.lib")
#endif

int GenerateMiniDump(PEXCEPTION_POINTERS pExceptionPointers)
{
    // check if dmp folder exists
    DWORD dwStatus = GetFileAttributesA("dmp");
    if ((dwStatus == INVALID_FILE_ATTRIBUTES) || !(dwStatus & FILE_ATTRIBUTE_DIRECTORY)) return EXCEPTION_EXECUTE_HANDLER;

    // define function ptr
    typedef BOOL(WINAPI * MiniDumpWriteDumpT)(
        HANDLE,
        DWORD,
        HANDLE,
        MINIDUMP_TYPE,
        PMINIDUMP_EXCEPTION_INFORMATION,
        PMINIDUMP_USER_STREAM_INFORMATION,
        PMINIDUMP_CALLBACK_INFORMATION
        );
    // load function MiniDumpWriteDump from DbgHelp.dll
    MiniDumpWriteDumpT pfnMiniDumpWriteDump = NULL;
    HMODULE hDbgHelp = LoadLibrary("DbgHelp.dll");
    if (NULL == hDbgHelp)
    {
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    pfnMiniDumpWriteDump = (MiniDumpWriteDumpT)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");

    if (NULL == pfnMiniDumpWriteDump)
    {
        FreeLibrary(hDbgHelp);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    // create dump file
    HANDLE hDumpFile = CreateFile(("dmp/DumpFile" + QDateTime::currentDateTime().toString("-yyyyMMdd-hhmmss") + ".dmp").toUtf8().constData(),
                                  GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
    if (INVALID_HANDLE_VALUE == hDumpFile)
    {
        FreeLibrary(hDbgHelp);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    // write dump file
    MINIDUMP_EXCEPTION_INFORMATION expParam;
    expParam.ThreadId = GetCurrentThreadId();
    expParam.ExceptionPointers = pExceptionPointers;
    expParam.ClientPointers = FALSE;
    pfnMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
        hDumpFile, MiniDumpWithDataSegs, (pExceptionPointers ? &expParam : NULL), NULL, NULL);
    // release file handle
    CloseHandle(hDumpFile);
    FreeLibrary(hDbgHelp);
    return EXCEPTION_EXECUTE_HANDLER;
}

LONG WINAPI ExceptionFilter(LPEXCEPTION_POINTERS lpExceptionInfo)
{
    if (IsDebuggerPresent()) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    return GenerateMiniDump(lpExceptionInfo);
}
#endif

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

int main(int argc, char *argv[])
{
//    setlocale(LC_ALL, ".65001");
//    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
//    qDebug() << QTextCodec::availableCodecs();

    QApplication a(argc, argv);

//    QPixmap pixmap("screen.png");
//    QSplashScreen screen(pixmap);
//    screen.show();
//    QElapsedTimer timer;
//    timer.start();
//    while(timer.elapsed() < (2000)) a.processEvents();

    int monaco_id = QFontDatabase::addApplicationFont(":/fonts/monaco");
    int consolas_id = QFontDatabase::addApplicationFont(":/fonts/consolas");
    monaco = QFont(QFontDatabase::applicationFontFamilies(monaco_id).at(0));
    monaco.setPixelSize(11);
//    monaco.setLetterSpacing(QFont::PercentageSpacing, 120);
    consolas = QFont(QFontDatabase::applicationFontFamilies(consolas_id).at(0));
    consolas.setPixelSize(12);
//    consolas.setLetterSpacing(QFont::PercentageSpacing, 120);

    a.setFont(monaco);
    QFile style(":/style/style.qss");
    style.open(QIODevice::ReadOnly);
    a.setStyleSheet(style.readAll());
    style.close();

    a.setWindowIcon(QIcon(":/tools/brush"));

//    qInstallMessageHandler(log_message);

#ifdef WIN32
    SetUnhandledExceptionFilter(ExceptionFilter);
#endif

    Demo w;
    w.show();
    return a.exec();
}
