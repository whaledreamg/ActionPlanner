/****************************************************************************
** Meta object code from reading C++ file 'RealtimeController.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../src/robot/RealtimeController.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'RealtimeController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN18RealtimeControllerE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN18RealtimeControllerE = QtMocHelpers::stringData(
    "RealtimeController",
    "streamingStarted",
    "",
    "streamingStopped",
    "leadInFinished",
    "armSessionChanged",
    "armName",
    "active",
    "errorOccurred",
    "message",
    "statusMessage",
    "streamStats",
    "actualHz",
    "totalPackets",
    "startRealtime",
    "stopRealtime",
    "pushFrame",
    "JointState",
    "state",
    "onResponse",
    "response",
    "onLeadInTimer",
    "onStreamStats",
    "checkStartupHealth",
    "probeSessionDiagnostics",
    "reportSessionDiagnostics",
    "statusDb"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN18RealtimeControllerE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  110,    2, 0x06,    1 /* Public */,
       3,    0,  111,    2, 0x06,    2 /* Public */,
       4,    0,  112,    2, 0x06,    3 /* Public */,
       5,    2,  113,    2, 0x06,    4 /* Public */,
       8,    1,  118,    2, 0x06,    7 /* Public */,
      10,    1,  121,    2, 0x06,    9 /* Public */,
      11,    2,  124,    2, 0x06,   11 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      14,    0,  129,    2, 0x0a,   14 /* Public */,
      15,    0,  130,    2, 0x0a,   15 /* Public */,
      16,    1,  131,    2, 0x0a,   16 /* Public */,
      19,    1,  134,    2, 0x08,   18 /* Private */,
      21,    0,  137,    2, 0x08,   20 /* Private */,
      22,    2,  138,    2, 0x08,   21 /* Private */,
      23,    0,  143,    2, 0x08,   24 /* Private */,
      24,    0,  144,    2, 0x08,   25 /* Private */,
      25,    1,  145,    2, 0x08,   26 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,    6,    7,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::Int, QMetaType::LongLong,   12,   13,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 17,   18,
    QMetaType::Void, QMetaType::QJsonObject,   20,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::LongLong,   12,   13,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QJsonObject,   26,

       0        // eod
};

Q_CONSTINIT const QMetaObject RealtimeController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN18RealtimeControllerE.offsetsAndSizes,
    qt_meta_data_ZN18RealtimeControllerE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN18RealtimeControllerE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<RealtimeController, std::true_type>,
        // method 'streamingStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'streamingStopped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'leadInFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'armSessionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'errorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'statusMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'streamStats'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'startRealtime'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopRealtime'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'pushFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const JointState &, std::false_type>,
        // method 'onResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>,
        // method 'onLeadInTimer'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onStreamStats'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'checkStartupHealth'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'probeSessionDiagnostics'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'reportSessionDiagnostics'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>
    >,
    nullptr
} };

void RealtimeController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<RealtimeController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->streamingStarted(); break;
        case 1: _t->streamingStopped(); break;
        case 2: _t->leadInFinished(); break;
        case 3: _t->armSessionChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 4: _t->errorOccurred((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->statusMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->streamStats((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 7: _t->startRealtime(); break;
        case 8: _t->stopRealtime(); break;
        case 9: _t->pushFrame((*reinterpret_cast< std::add_pointer_t<JointState>>(_a[1]))); break;
        case 10: _t->onResponse((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 11: _t->onLeadInTimer(); break;
        case 12: _t->onStreamStats((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 13: _t->checkStartupHealth(); break;
        case 14: _t->probeSessionDiagnostics(); break;
        case 15: _t->reportSessionDiagnostics((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (RealtimeController::*)();
            if (_q_method_type _q_method = &RealtimeController::streamingStarted; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (RealtimeController::*)();
            if (_q_method_type _q_method = &RealtimeController::streamingStopped; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (RealtimeController::*)();
            if (_q_method_type _q_method = &RealtimeController::leadInFinished; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (RealtimeController::*)(const QString & , bool );
            if (_q_method_type _q_method = &RealtimeController::armSessionChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (RealtimeController::*)(const QString & );
            if (_q_method_type _q_method = &RealtimeController::errorOccurred; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (RealtimeController::*)(const QString & );
            if (_q_method_type _q_method = &RealtimeController::statusMessage; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (RealtimeController::*)(int , qint64 );
            if (_q_method_type _q_method = &RealtimeController::streamStats; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
    }
}

const QMetaObject *RealtimeController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RealtimeController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN18RealtimeControllerE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int RealtimeController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void RealtimeController::streamingStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void RealtimeController::streamingStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void RealtimeController::leadInFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void RealtimeController::armSessionChanged(const QString & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void RealtimeController::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void RealtimeController::statusMessage(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void RealtimeController::streamStats(int _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_WARNING_POP
