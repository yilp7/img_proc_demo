/****************************************************************************
** Meta object code from reading C++ file 'demo.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.10)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../demo.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'demo.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.10. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MouseThread_t {
    QByteArrayData data[4];
    char stringdata0[36];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MouseThread_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MouseThread_t qt_meta_stringdata_MouseThread = {
    {
QT_MOC_LITERAL(0, 0, 11), // "MouseThread"
QT_MOC_LITERAL(1, 12, 10), // "set_cursor"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 11) // "draw_cursor"

    },
    "MouseThread\0set_cursor\0\0draw_cursor"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MouseThread[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   24,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    0,   27,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QCursor,    2,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void MouseThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MouseThread *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->set_cursor((*reinterpret_cast< QCursor(*)>(_a[1]))); break;
        case 1: _t->draw_cursor(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MouseThread::*)(QCursor );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MouseThread::set_cursor)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MouseThread::staticMetaObject = { {
    &QThread::staticMetaObject,
    qt_meta_stringdata_MouseThread.data,
    qt_meta_data_MouseThread,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MouseThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MouseThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MouseThread.stringdata0))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int MouseThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void MouseThread::set_cursor(QCursor _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
struct qt_meta_stringdata_Demo_t {
    QByteArrayData data[69];
    char stringdata0[1480];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Demo_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Demo_t qt_meta_stringdata_Demo = {
    {
QT_MOC_LITERAL(0, 0, 4), // "Demo"
QT_MOC_LITERAL(1, 5, 11), // "append_text"
QT_MOC_LITERAL(2, 17, 0), // ""
QT_MOC_LITERAL(3, 18, 4), // "text"
QT_MOC_LITERAL(4, 23, 11), // "update_scan"
QT_MOC_LITERAL(5, 35, 4), // "show"
QT_MOC_LITERAL(6, 40, 22), // "update_delay_in_thread"
QT_MOC_LITERAL(7, 63, 11), // "draw_cursor"
QT_MOC_LITERAL(8, 75, 1), // "c"
QT_MOC_LITERAL(9, 77, 15), // "switch_language"
QT_MOC_LITERAL(10, 93, 10), // "screenshot"
QT_MOC_LITERAL(11, 104, 5), // "clean"
QT_MOC_LITERAL(12, 110, 22), // "on_ENUM_BUTTON_clicked"
QT_MOC_LITERAL(13, 133, 23), // "on_START_BUTTON_clicked"
QT_MOC_LITERAL(14, 157, 26), // "on_SHUTDOWN_BUTTON_clicked"
QT_MOC_LITERAL(15, 184, 27), // "on_CONTINUOUS_RADIO_clicked"
QT_MOC_LITERAL(16, 212, 24), // "on_TRIGGER_RADIO_clicked"
QT_MOC_LITERAL(17, 237, 30), // "on_SOFTWARE_CHECK_stateChanged"
QT_MOC_LITERAL(18, 268, 4), // "arg1"
QT_MOC_LITERAL(19, 273, 34), // "on_SOFTWARE_TRIGGER_BUTTON_cl..."
QT_MOC_LITERAL(20, 308, 32), // "on_START_GRABBING_BUTTON_clicked"
QT_MOC_LITERAL(21, 341, 31), // "on_STOP_GRABBING_BUTTON_clicked"
QT_MOC_LITERAL(22, 373, 26), // "on_SAVE_BMP_BUTTON_clicked"
QT_MOC_LITERAL(23, 400, 29), // "on_SAVE_RESULT_BUTTON_clicked"
QT_MOC_LITERAL(24, 430, 26), // "on_SAVE_AVI_BUTTON_clicked"
QT_MOC_LITERAL(25, 457, 28), // "on_SAVE_FINAL_BUTTON_clicked"
QT_MOC_LITERAL(26, 486, 28), // "on_SET_PARAMS_BUTTON_clicked"
QT_MOC_LITERAL(27, 515, 28), // "on_GET_PARAMS_BUTTON_clicked"
QT_MOC_LITERAL(28, 544, 23), // "on_GAIN_EDIT_textEdited"
QT_MOC_LITERAL(29, 568, 27), // "on_FILE_PATH_BROWSE_clicked"
QT_MOC_LITERAL(30, 596, 19), // "on_DIST_BTN_clicked"
QT_MOC_LITERAL(31, 616, 28), // "on_IMG_3D_CHECK_stateChanged"
QT_MOC_LITERAL(32, 645, 31), // "on_FRAME_AVG_CHECK_stateChanged"
QT_MOC_LITERAL(33, 677, 22), // "on_ZOOM_IN_BTN_pressed"
QT_MOC_LITERAL(34, 700, 23), // "on_ZOOM_IN_BTN_released"
QT_MOC_LITERAL(35, 724, 23), // "on_ZOOM_OUT_BTN_pressed"
QT_MOC_LITERAL(36, 748, 24), // "on_ZOOM_OUT_BTN_released"
QT_MOC_LITERAL(37, 773, 25), // "on_FOCUS_NEAR_BTN_pressed"
QT_MOC_LITERAL(38, 799, 26), // "on_FOCUS_NEAR_BTN_released"
QT_MOC_LITERAL(39, 826, 24), // "on_FOCUS_FAR_BTN_pressed"
QT_MOC_LITERAL(40, 851, 25), // "on_FOCUS_FAR_BTN_released"
QT_MOC_LITERAL(41, 877, 30), // "on_FOCUS_SPEED_EDIT_textEdited"
QT_MOC_LITERAL(42, 908, 29), // "on_GET_LENS_PARAM_BTN_clicked"
QT_MOC_LITERAL(43, 938, 25), // "on_AUTO_FOCUS_BTN_clicked"
QT_MOC_LITERAL(44, 964, 28), // "on_LASER_ZOOM_IN_BTN_pressed"
QT_MOC_LITERAL(45, 993, 29), // "on_LASER_ZOOM_OUT_BTN_pressed"
QT_MOC_LITERAL(46, 1023, 29), // "on_LASER_ZOOM_IN_BTN_released"
QT_MOC_LITERAL(47, 1053, 30), // "on_LASER_ZOOM_OUT_BTN_released"
QT_MOC_LITERAL(48, 1084, 10), // "change_mcp"
QT_MOC_LITERAL(49, 1095, 3), // "val"
QT_MOC_LITERAL(50, 1099, 11), // "change_gain"
QT_MOC_LITERAL(51, 1111, 12), // "change_delay"
QT_MOC_LITERAL(52, 1124, 18), // "change_focus_speed"
QT_MOC_LITERAL(53, 1143, 22), // "on_SCAN_BUTTON_clicked"
QT_MOC_LITERAL(54, 1166, 31), // "on_CONTINUE_SCAN_BUTTON_clicked"
QT_MOC_LITERAL(55, 1198, 30), // "on_RESTART_SCAN_BUTTON_clicked"
QT_MOC_LITERAL(56, 1229, 11), // "append_data"
QT_MOC_LITERAL(57, 1241, 19), // "enable_scan_options"
QT_MOC_LITERAL(58, 1261, 12), // "update_delay"
QT_MOC_LITERAL(59, 1274, 20), // "on_LASER_BTN_clicked"
QT_MOC_LITERAL(60, 1295, 11), // "start_laser"
QT_MOC_LITERAL(61, 1307, 10), // "init_laser"
QT_MOC_LITERAL(62, 1318, 19), // "on_HIDE_BTN_clicked"
QT_MOC_LITERAL(63, 1338, 25), // "on_COM_DATA_RADIO_clicked"
QT_MOC_LITERAL(64, 1364, 26), // "on_HISTOGRAM_RADIO_clicked"
QT_MOC_LITERAL(65, 1391, 20), // "on_DRAG_TOOL_clicked"
QT_MOC_LITERAL(66, 1412, 22), // "on_SELECT_TOOL_clicked"
QT_MOC_LITERAL(67, 1435, 38), // "on_ENHANCE_OPTIONS_currentInd..."
QT_MOC_LITERAL(68, 1474, 5) // "index"

    },
    "Demo\0append_text\0\0text\0update_scan\0"
    "show\0update_delay_in_thread\0draw_cursor\0"
    "c\0switch_language\0screenshot\0clean\0"
    "on_ENUM_BUTTON_clicked\0on_START_BUTTON_clicked\0"
    "on_SHUTDOWN_BUTTON_clicked\0"
    "on_CONTINUOUS_RADIO_clicked\0"
    "on_TRIGGER_RADIO_clicked\0"
    "on_SOFTWARE_CHECK_stateChanged\0arg1\0"
    "on_SOFTWARE_TRIGGER_BUTTON_clicked\0"
    "on_START_GRABBING_BUTTON_clicked\0"
    "on_STOP_GRABBING_BUTTON_clicked\0"
    "on_SAVE_BMP_BUTTON_clicked\0"
    "on_SAVE_RESULT_BUTTON_clicked\0"
    "on_SAVE_AVI_BUTTON_clicked\0"
    "on_SAVE_FINAL_BUTTON_clicked\0"
    "on_SET_PARAMS_BUTTON_clicked\0"
    "on_GET_PARAMS_BUTTON_clicked\0"
    "on_GAIN_EDIT_textEdited\0"
    "on_FILE_PATH_BROWSE_clicked\0"
    "on_DIST_BTN_clicked\0on_IMG_3D_CHECK_stateChanged\0"
    "on_FRAME_AVG_CHECK_stateChanged\0"
    "on_ZOOM_IN_BTN_pressed\0on_ZOOM_IN_BTN_released\0"
    "on_ZOOM_OUT_BTN_pressed\0"
    "on_ZOOM_OUT_BTN_released\0"
    "on_FOCUS_NEAR_BTN_pressed\0"
    "on_FOCUS_NEAR_BTN_released\0"
    "on_FOCUS_FAR_BTN_pressed\0"
    "on_FOCUS_FAR_BTN_released\0"
    "on_FOCUS_SPEED_EDIT_textEdited\0"
    "on_GET_LENS_PARAM_BTN_clicked\0"
    "on_AUTO_FOCUS_BTN_clicked\0"
    "on_LASER_ZOOM_IN_BTN_pressed\0"
    "on_LASER_ZOOM_OUT_BTN_pressed\0"
    "on_LASER_ZOOM_IN_BTN_released\0"
    "on_LASER_ZOOM_OUT_BTN_released\0"
    "change_mcp\0val\0change_gain\0change_delay\0"
    "change_focus_speed\0on_SCAN_BUTTON_clicked\0"
    "on_CONTINUE_SCAN_BUTTON_clicked\0"
    "on_RESTART_SCAN_BUTTON_clicked\0"
    "append_data\0enable_scan_options\0"
    "update_delay\0on_LASER_BTN_clicked\0"
    "start_laser\0init_laser\0on_HIDE_BTN_clicked\0"
    "on_COM_DATA_RADIO_clicked\0"
    "on_HISTOGRAM_RADIO_clicked\0"
    "on_DRAG_TOOL_clicked\0on_SELECT_TOOL_clicked\0"
    "on_ENHANCE_OPTIONS_currentIndexChanged\0"
    "index"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Demo[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      61,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  319,    2, 0x06 /* Public */,
       4,    1,  322,    2, 0x06 /* Public */,
       6,    0,  325,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    1,  326,    2, 0x0a /* Public */,
       9,    0,  329,    2, 0x0a /* Public */,
      10,    0,  330,    2, 0x0a /* Public */,
      11,    0,  331,    2, 0x0a /* Public */,
      12,    0,  332,    2, 0x08 /* Private */,
      13,    0,  333,    2, 0x08 /* Private */,
      14,    0,  334,    2, 0x08 /* Private */,
      15,    0,  335,    2, 0x08 /* Private */,
      16,    0,  336,    2, 0x08 /* Private */,
      17,    1,  337,    2, 0x08 /* Private */,
      19,    0,  340,    2, 0x08 /* Private */,
      20,    0,  341,    2, 0x08 /* Private */,
      21,    0,  342,    2, 0x08 /* Private */,
      22,    0,  343,    2, 0x08 /* Private */,
      23,    0,  344,    2, 0x08 /* Private */,
      24,    0,  345,    2, 0x08 /* Private */,
      25,    0,  346,    2, 0x08 /* Private */,
      26,    0,  347,    2, 0x08 /* Private */,
      27,    0,  348,    2, 0x08 /* Private */,
      28,    1,  349,    2, 0x08 /* Private */,
      29,    0,  352,    2, 0x08 /* Private */,
      30,    0,  353,    2, 0x08 /* Private */,
      31,    1,  354,    2, 0x08 /* Private */,
      32,    1,  357,    2, 0x08 /* Private */,
      33,    0,  360,    2, 0x08 /* Private */,
      34,    0,  361,    2, 0x08 /* Private */,
      35,    0,  362,    2, 0x08 /* Private */,
      36,    0,  363,    2, 0x08 /* Private */,
      37,    0,  364,    2, 0x08 /* Private */,
      38,    0,  365,    2, 0x08 /* Private */,
      39,    0,  366,    2, 0x08 /* Private */,
      40,    0,  367,    2, 0x08 /* Private */,
      41,    1,  368,    2, 0x08 /* Private */,
      42,    0,  371,    2, 0x08 /* Private */,
      43,    0,  372,    2, 0x08 /* Private */,
      44,    0,  373,    2, 0x08 /* Private */,
      45,    0,  374,    2, 0x08 /* Private */,
      46,    0,  375,    2, 0x08 /* Private */,
      47,    0,  376,    2, 0x08 /* Private */,
      48,    1,  377,    2, 0x08 /* Private */,
      50,    1,  380,    2, 0x08 /* Private */,
      51,    1,  383,    2, 0x08 /* Private */,
      52,    1,  386,    2, 0x08 /* Private */,
      53,    0,  389,    2, 0x08 /* Private */,
      54,    0,  390,    2, 0x08 /* Private */,
      55,    0,  391,    2, 0x08 /* Private */,
      56,    1,  392,    2, 0x08 /* Private */,
      57,    1,  395,    2, 0x08 /* Private */,
      58,    0,  398,    2, 0x08 /* Private */,
      59,    0,  399,    2, 0x08 /* Private */,
      60,    0,  400,    2, 0x08 /* Private */,
      61,    0,  401,    2, 0x08 /* Private */,
      62,    0,  402,    2, 0x08 /* Private */,
      63,    0,  403,    2, 0x08 /* Private */,
      64,    0,  404,    2, 0x08 /* Private */,
      65,    0,  405,    2, 0x08 /* Private */,
      66,    0,  406,    2, 0x08 /* Private */,
      67,    1,  407,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QCursor,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   18,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   18,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   18,
    QMetaType::Void, QMetaType::Int,   18,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   18,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   49,
    QMetaType::Void, QMetaType::Int,   49,
    QMetaType::Void, QMetaType::Int,   49,
    QMetaType::Void, QMetaType::Int,   49,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   68,

       0        // eod
};

void Demo::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Demo *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->append_text((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->update_scan((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->update_delay_in_thread(); break;
        case 3: _t->draw_cursor((*reinterpret_cast< QCursor(*)>(_a[1]))); break;
        case 4: _t->switch_language(); break;
        case 5: _t->screenshot(); break;
        case 6: _t->clean(); break;
        case 7: _t->on_ENUM_BUTTON_clicked(); break;
        case 8: _t->on_START_BUTTON_clicked(); break;
        case 9: _t->on_SHUTDOWN_BUTTON_clicked(); break;
        case 10: _t->on_CONTINUOUS_RADIO_clicked(); break;
        case 11: _t->on_TRIGGER_RADIO_clicked(); break;
        case 12: _t->on_SOFTWARE_CHECK_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 13: _t->on_SOFTWARE_TRIGGER_BUTTON_clicked(); break;
        case 14: _t->on_START_GRABBING_BUTTON_clicked(); break;
        case 15: _t->on_STOP_GRABBING_BUTTON_clicked(); break;
        case 16: _t->on_SAVE_BMP_BUTTON_clicked(); break;
        case 17: _t->on_SAVE_RESULT_BUTTON_clicked(); break;
        case 18: _t->on_SAVE_AVI_BUTTON_clicked(); break;
        case 19: _t->on_SAVE_FINAL_BUTTON_clicked(); break;
        case 20: _t->on_SET_PARAMS_BUTTON_clicked(); break;
        case 21: _t->on_GET_PARAMS_BUTTON_clicked(); break;
        case 22: _t->on_GAIN_EDIT_textEdited((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 23: _t->on_FILE_PATH_BROWSE_clicked(); break;
        case 24: _t->on_DIST_BTN_clicked(); break;
        case 25: _t->on_IMG_3D_CHECK_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 26: _t->on_FRAME_AVG_CHECK_stateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 27: _t->on_ZOOM_IN_BTN_pressed(); break;
        case 28: _t->on_ZOOM_IN_BTN_released(); break;
        case 29: _t->on_ZOOM_OUT_BTN_pressed(); break;
        case 30: _t->on_ZOOM_OUT_BTN_released(); break;
        case 31: _t->on_FOCUS_NEAR_BTN_pressed(); break;
        case 32: _t->on_FOCUS_NEAR_BTN_released(); break;
        case 33: _t->on_FOCUS_FAR_BTN_pressed(); break;
        case 34: _t->on_FOCUS_FAR_BTN_released(); break;
        case 35: _t->on_FOCUS_SPEED_EDIT_textEdited((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 36: _t->on_GET_LENS_PARAM_BTN_clicked(); break;
        case 37: _t->on_AUTO_FOCUS_BTN_clicked(); break;
        case 38: _t->on_LASER_ZOOM_IN_BTN_pressed(); break;
        case 39: _t->on_LASER_ZOOM_OUT_BTN_pressed(); break;
        case 40: _t->on_LASER_ZOOM_IN_BTN_released(); break;
        case 41: _t->on_LASER_ZOOM_OUT_BTN_released(); break;
        case 42: _t->change_mcp((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 43: _t->change_gain((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 44: _t->change_delay((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 45: _t->change_focus_speed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 46: _t->on_SCAN_BUTTON_clicked(); break;
        case 47: _t->on_CONTINUE_SCAN_BUTTON_clicked(); break;
        case 48: _t->on_RESTART_SCAN_BUTTON_clicked(); break;
        case 49: _t->append_data((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 50: _t->enable_scan_options((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 51: _t->update_delay(); break;
        case 52: _t->on_LASER_BTN_clicked(); break;
        case 53: _t->start_laser(); break;
        case 54: _t->init_laser(); break;
        case 55: _t->on_HIDE_BTN_clicked(); break;
        case 56: _t->on_COM_DATA_RADIO_clicked(); break;
        case 57: _t->on_HISTOGRAM_RADIO_clicked(); break;
        case 58: _t->on_DRAG_TOOL_clicked(); break;
        case 59: _t->on_SELECT_TOOL_clicked(); break;
        case 60: _t->on_ENHANCE_OPTIONS_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Demo::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Demo::append_text)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Demo::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Demo::update_scan)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Demo::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Demo::update_delay_in_thread)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Demo::staticMetaObject = { {
    &QMainWindow::staticMetaObject,
    qt_meta_stringdata_Demo.data,
    qt_meta_data_Demo,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Demo::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Demo::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Demo.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int Demo::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 61)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 61;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 61)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 61;
    }
    return _id;
}

// SIGNAL 0
void Demo::append_text(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Demo::update_scan(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Demo::update_delay_in_thread()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
