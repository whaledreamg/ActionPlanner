/****************************************************************************
** Meta object code from reading C++ file 'PlaybackEngine.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../src/timeline/PlaybackEngine.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PlaybackEngine.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14PlaybackEngineE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN14PlaybackEngineE = QtMocHelpers::stringData(
    "PlaybackEngine",
    "started",
    "",
    "paused",
    "resumed",
    "stopped",
    "finished",
    "looped",
    "frameReady",
    "time",
    "JointState",
    "state",
    "progressChanged",
    "total",
    "segmentEntered",
    "index",
    "play",
    "pause",
    "resume",
    "stop",
    "primeStartFrame",
    "seek",
    "setRewindOnFinish",
    "on",
    "setLoop",
    "loop",
    "onTick",
    "refreshKeyframes"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN14PlaybackEngineE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      20,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  134,    2, 0x06,    1 /* Public */,
       3,    0,  135,    2, 0x06,    2 /* Public */,
       4,    0,  136,    2, 0x06,    3 /* Public */,
       5,    0,  137,    2, 0x06,    4 /* Public */,
       6,    0,  138,    2, 0x06,    5 /* Public */,
       7,    0,  139,    2, 0x06,    6 /* Public */,
       8,    2,  140,    2, 0x06,    7 /* Public */,
      12,    2,  145,    2, 0x06,   10 /* Public */,
      14,    1,  150,    2, 0x06,   13 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      16,    0,  153,    2, 0x0a,   15 /* Public */,
      17,    0,  154,    2, 0x0a,   16 /* Public */,
      18,    0,  155,    2, 0x0a,   17 /* Public */,
      19,    0,  156,    2, 0x0a,   18 /* Public */,
      20,    0,  157,    2, 0x0a,   19 /* Public */,
      21,    1,  158,    2, 0x0a,   20 /* Public */,
      22,    1,  161,    2, 0x0a,   22 /* Public */,
      24,    1,  164,    2, 0x0a,   24 /* Public */,
      25,    0,  167,    2, 0x10a,   26 /* Public | MethodIsConst  */,
      26,    0,  168,    2, 0x08,   27 /* Private */,
      27,    0,  169,    2, 0x08,   28 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double, 0x80000000 | 10,    9,   11,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,    9,   13,
    QMetaType::Void, QMetaType::Int,   15,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double,    9,
    QMetaType::Void, QMetaType::Bool,   23,
    QMetaType::Void, QMetaType::Bool,   23,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject PlaybackEngine::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN14PlaybackEngineE.offsetsAndSizes,
    qt_meta_data_ZN14PlaybackEngineE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN14PlaybackEngineE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PlaybackEngine, std::true_type>,
        // method 'started'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'paused'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resumed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'finished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'looped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'frameReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<const JointState &, std::false_type>,
        // method 'progressChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'segmentEntered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'play'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'pause'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resume'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'primeStartFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'seek'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'setRewindOnFinish'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setLoop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'loop'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onTick'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshKeyframes'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void PlaybackEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PlaybackEngine *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->started(); break;
        case 1: _t->paused(); break;
        case 2: _t->resumed(); break;
        case 3: _t->stopped(); break;
        case 4: _t->finished(); break;
        case 5: _t->looped(); break;
        case 6: _t->frameReady((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<JointState>>(_a[2]))); break;
        case 7: _t->progressChanged((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2]))); break;
        case 8: _t->segmentEntered((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->play(); break;
        case 10: _t->pause(); break;
        case 11: _t->resume(); break;
        case 12: _t->stop(); break;
        case 13: _t->primeStartFrame(); break;
        case 14: _t->seek((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 15: _t->setRewindOnFinish((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 16: _t->setLoop((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 17: { bool _r = _t->loop();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 18: _t->onTick(); break;
        case 19: _t->refreshKeyframes(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (PlaybackEngine::*)();
            if (_q_method_type _q_method = &PlaybackEngine::started; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackEngine::*)();
            if (_q_method_type _q_method = &PlaybackEngine::paused; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackEngine::*)();
            if (_q_method_type _q_method = &PlaybackEngine::resumed; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackEngine::*)();
            if (_q_method_type _q_method = &PlaybackEngine::stopped; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackEngine::*)();
            if (_q_method_type _q_method = &PlaybackEngine::finished; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackEngine::*)();
            if (_q_method_type _q_method = &PlaybackEngine::looped; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackEngine::*)(double , const JointState & );
            if (_q_method_type _q_method = &PlaybackEngine::frameReady; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackEngine::*)(double , double );
            if (_q_method_type _q_method = &PlaybackEngine::progressChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (PlaybackEngine::*)(int );
            if (_q_method_type _q_method = &PlaybackEngine::segmentEntered; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
    }
}

const QMetaObject *PlaybackEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PlaybackEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN14PlaybackEngineE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int PlaybackEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 20;
    }
    return _id;
}

// SIGNAL 0
void PlaybackEngine::started()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void PlaybackEngine::paused()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void PlaybackEngine::resumed()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void PlaybackEngine::stopped()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void PlaybackEngine::finished()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void PlaybackEngine::looped()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void PlaybackEngine::frameReady(double _t1, const JointState & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void PlaybackEngine::progressChanged(double _t1, double _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void PlaybackEngine::segmentEntered(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}
QT_WARNING_POP
