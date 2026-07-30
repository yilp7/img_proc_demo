#include "visual/userpanel.h"
#include "util/config.h"
#include "util/thememanager.h"
#include "util/version.h"

//#define _DEBUG
//#include "vld.h"

#ifdef WIN32
#include <DbgHelp.h>

namespace {
QString g_dump_directory;
}

#if BUILD_STATIC
#include <QtCore/QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#pragma comment(lib, "user32.lib")
#endif

int GenerateMiniDump(PEXCEPTION_POINTERS pExceptionPointers)
{
    // check if dmp folder exists
    const QString native_dump_directory = QDir::toNativeSeparators(g_dump_directory);
    DWORD dwStatus = GetFileAttributesW(
        reinterpret_cast<LPCWSTR>(native_dump_directory.utf16()));
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
    HMODULE hDbgHelp = LoadLibraryW(L"DbgHelp.dll");
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
    const QString dump_file_path = QDir::toNativeSeparators(
        QDir(g_dump_directory).filePath(
            "DumpFile" + QDateTime::currentDateTime().toString("-yyyyMMdd-hhmmss") + ".dmp"));
    HANDLE hDumpFile = CreateFileW(reinterpret_cast<LPCWSTR>(dump_file_path.utf16()),
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_WRITE | FILE_SHARE_READ,
                                   0, CREATE_ALWAYS, 0, 0);
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
    static QFile file("./GLI_logs/log");
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

#ifdef WIN32
    Config startup_config;
    const QString default_config_path = QCoreApplication::applicationDirPath() + "/default.json";
    if (QFile::exists(default_config_path)) {
        startup_config.load_from_file(default_config_path);
    }
    if (startup_config.get_data().debug.generate_dmp) {
        g_dump_directory = QDir(QCoreApplication::applicationDirPath()).filePath("dmp");
        if (QDir().mkpath(g_dump_directory)) {
            SetUnhandledExceptionFilter(ExceptionFilter);
        } else {
            qWarning() << "Cannot create dump directory:" << g_dump_directory;
        }
    }
#endif

//    QPixmap pixmap("screen.png");
//    QSplashScreen screen(pixmap);
//    screen.show();
//    QElapsedTimer timer;
//    timer.start();
//    while(timer.elapsed() < (2000)) a.processEvents();

    auto load_app_font = [](const QString &path, const QString &fallback, int pixel_size) {
        const int id = QFontDatabase::addApplicationFont(path);
        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        QFont font(families.isEmpty() ? fallback : families.at(0));
        font.setPixelSize(pixel_size);
        return font;
    };
    auto available_system_sans_families = []() {
        const QStringList preferred_families = QStringList()
            << "Source Han Sans SC"
            << "Source Han Sans CN"
            << "Noto Sans CJK SC"
            << "Noto Sans SC"
            << "Microsoft YaHei UI"
            << "Microsoft YaHei"
            << "SimHei"
            << "Heiti SC";
        const QStringList available_families = QFontDatabase().families();
        QStringList families;
        for (const QString &candidate : preferred_families) {
            if (available_families.contains(candidate, Qt::CaseInsensitive)) {
                families << candidate;
            }
        }
        if (families.isEmpty()) families << "Microsoft YaHei";
        return families;
    };

    auto load_system_sans_font = [](const QStringList &families, int pixel_size) {
        QFont font(families.isEmpty() ? QString("Microsoft YaHei") : families.first());
        font.setStyleHint(QFont::SansSerif);
        font.setPixelSize(pixel_size);
        return font;
    };

    monaco = load_app_font(":/fonts/monaco", "Monaco", 11);
//    monaco.setLetterSpacing(QFont::PercentageSpacing, 120);
    consolas = load_app_font(":/fonts/consolas", "Consolas", 12);
    const QStringList cjk_fallback_families = available_system_sans_families();
    sans_heiti = load_system_sans_font(cjk_fallback_families, 10);
    QFont::insertSubstitutions(monaco.family(), cjk_fallback_families);
    QFont::insertSubstitutions("Monaco", cjk_fallback_families);
    QFont::insertSubstitutions(consolas.family(), cjk_fallback_families);
    QFont::insertSubstitutions("Consolas", cjk_fallback_families);
//    consolas.setLetterSpacing(QFont::PercentageSpacing, 120);

    a.setFont(monaco);

    stylesheet_dark = ThemeManager::instance().buildStyleSheet("gruvbox", "dark");
    stylesheet_light = ThemeManager::instance().buildStyleSheet("gruvbox", "light");
    ThemeManager::instance().setCurrentTheme("gruvbox", "dark");

    app_color_mode = 0;

    cursor_dark_pointer   = QCursor(QPixmap(":/cursor/dark/cursor").scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation), 0, 0);
    cursor_dark_resize_h  = QCursor(QPixmap(":/cursor/dark/resize_h").scaled(20, 20));
    cursor_dark_resize_v  = QCursor(QPixmap(":/cursor/dark/resize_v").scaled(20, 20));
    cursor_dark_resize_md = QCursor(QPixmap(":/cursor/dark/resize_md").scaled(16, 16));
    cursor_dark_resize_sd = QCursor(QPixmap(":/cursor/dark/resize_sd").scaled(16, 16));

    cursor_light_pointer   = QCursor(QPixmap(":/cursor/light/cursor").scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation), 0, 0);
    cursor_light_resize_h  = QCursor(QPixmap(":/cursor/light/resize_h").scaled(20, 20));
    cursor_light_resize_v  = QCursor(QPixmap(":/cursor/light/resize_v").scaled(20, 20));
    cursor_light_resize_md = QCursor(QPixmap(":/cursor/light/resize_md").scaled(16, 16));
    cursor_light_resize_sd = QCursor(QPixmap(":/cursor/light/resize_sd").scaled(16, 16));

    // Theme family and color mode will be loaded from default.json in UserPanel initialization.
    app_color_mode = 0;

    a.setStyleSheet(app_color_mode ? stylesheet_light : stylesheet_dark);
    cursor_curr_pointer   = app_color_mode ? cursor_light_pointer   : cursor_dark_pointer;
    cursor_curr_resize_h  = app_color_mode ? cursor_light_resize_h  : cursor_dark_resize_h;
    cursor_curr_resize_v  = app_color_mode ? cursor_light_resize_v  : cursor_dark_resize_v;
    cursor_curr_resize_md = app_color_mode ? cursor_light_resize_md : cursor_dark_resize_md;
    cursor_curr_resize_sd = app_color_mode ? cursor_light_resize_sd : cursor_dark_resize_sd;

//    a.setWindowIcon(QIcon(":/tools/brush"));

//    qInstallMessageHandler(log_message);

    UserPanel w;

    w.init();
    w.show();
    
    return a.exec();
}
