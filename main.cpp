#include "demo.h"

#include <tchar.h>
#include <DbgHelp.h>

#pragma comment(lib, "user32.lib")

int GenerateMiniDump(PEXCEPTION_POINTERS pExceptionPointers)
{
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
    HMODULE hDbgHelp = LoadLibrary(_T("DbgHelp.dll"));
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
    TCHAR szFileName[MAX_PATH] = { 0 };
    TCHAR szVersion[] = "DumpFile";
    SYSTEMTIME stLocalTime;
    GetLocalTime(&stLocalTime);
    sprintf(szFileName, "%s-%04d%02d%02d-%02d%02d%02d.dmp",
        szVersion, stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
        stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond);
    HANDLE hDumpFile = CreateFile(szFileName, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
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
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    QApplication a(argc, argv);

    int monaco_id = QFontDatabase::addApplicationFont(":/fonts/monaco.ttf");
    int consolas_id = QFontDatabase::addApplicationFont(":/fonts/consola.ttf");
    monaco = QFont(QFontDatabase::applicationFontFamilies(monaco_id).at(0));
    monaco.setPointSizeF(8.6);
//    monaco.setLetterSpacing(QFont::PercentageSpacing, 120);
    consolas = QFont(QFontDatabase::applicationFontFamilies(consolas_id).at(0));
    consolas.setPointSizeF(9);
//    consolas.setLetterSpacing(QFont::PercentageSpacing, 120);

    a.setFont(monaco);
    QFile style(":/style/style.qss");
    style.open(QIODevice::ReadOnly);
    a.setStyleSheet(style.readAll());

//    qInstallMessageHandler(log_message);

    SetUnhandledExceptionFilter(ExceptionFilter);

    Demo w;
    w.show();
    return a.exec();
}
