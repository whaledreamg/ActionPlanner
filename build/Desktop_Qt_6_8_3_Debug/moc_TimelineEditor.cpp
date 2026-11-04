/****************************************************************************
** Meta object code from reading C++ file 'TimelineEditor.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../src/ui/TimelineEditor.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TimelineEditor.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14TimelineEditorE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN14TimelineEditorE = QtMocHelpers::stringData(
    "TimelineEditor",
    "playRequested",
    "",
    "segmentAdded",
    "Segment",
    "segment",
    "streamFrequencyChanged",
    "hz",
    "previewRequested",
    "QList<double>",
    "leftArm",
    "rightArm",
    "playbackFrame",
    "playbackStateChanged",
    "active",
    "setCurrentAngles",
    "onRobotAvailabilityChanged",
    "available",
    "onSave",
    "onLoad",
    "onAddSegment",
    "onAddHandSegment",
    "onDeleteSegment",
    "onDuplicateSegment",
    "onPlay",
    "onPause",
    "onStop",
    "onPlaybackStarted",
    "onPlaybackFinished",
    "onPlaybackFrame",
    "JointState",
    "state",
    "onPlaybackProgress",
    "time",
    "total",
    "onSeek",
    "onRealtimeError",
    "message",
    "onRealtimeStatus",
    "onSegmentEntered",
    "index"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN14TimelineEditorE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      25,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  164,    2, 0x06,    1 /* Public */,
       3,    1,  165,    2, 0x06,    2 /* Public */,
       6,    1,  168,    2, 0x06,    4 /* Public */,
       8,    2,  171,    2, 0x06,    6 /* Public */,
      12,    2,  176,    2, 0x06,    9 /* Public */,
      13,    1,  181,    2, 0x06,   12 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      15,    2,  184,    2, 0x0a,   14 /* Public */,
      16,    1,  189,    2, 0x0a,   17 /* Public */,
      18,    0,  192,    2, 0x0a,   19 /* Public */,
      19,    0,  193,    2, 0x0a,   20 /* Public */,
      20,    0,  194,    2, 0x08,   21 /* Private */,
      21,    0,  195,    2, 0x08,   22 /* Private */,
      22,    0,  196,    2, 0x08,   23 /* Private */,
      23,    0,  197,    2, 0x08,   24 /* Private */,
      24,    0,  198,    2, 0x08,   25 /* Private */,
      25,    0,  199,    2, 0x08,   26 /* Private */,
      26,    0,  200,    2, 0x08,   27 /* Private */,
      27,    0,  201,    2, 0x08,   28 /* Private */,
      28,    0,  202,    2, 0x08,   29 /* Private */,
      29,    1,  203,    2, 0x08,   30 /* Private */,
      32,    2,  206,    2, 0x08,   32 /* Private */,
      35,    1,  211,    2, 0x08,   35 /* Private */,
      36,    1,  214,    2, 0x08,   37 /* Private */,
      38,    1,  217,    2, 0x08,   39 /* Private */,
      39,    1,  220,    2, 0x08,   41 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, 0x80000000 | 9, 0x80000000 | 9,   10,   11,
    QMetaType::Void, 0x80000000 | 9, 0x80000000 | 9,   10,   11,
    QMetaType::Void, QMetaType::Bool,   14,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 9, 0x80000000 | 9,   10,   11,
    QMetaType::Void, QMetaType::Bool,   17,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 30,   31,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,   33,   34,
    QMetaType::Void, QMetaType::Double,   33,
    QMetaType::Void, QMetaType::QString,   37,
    QMetaType::Void, QMetaType::QString,   37,
    QMetaType::Void, QMetaType::Int,   40,

       0        // eod
};

Q_CONSTINIT const QMetaObject TimelineEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ZN14TimelineEditorE.offsetsAndSizes,
    qt_meta_data_ZN14TimelineEditorE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN14TimelineEditorE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TimelineEditor, std::true_type>,
        // method 'playRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'segmentAdded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const Segment &, std::false_type>,
        // method 'streamFrequencyChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'previewRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<double> &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<double> &, std::false_type>,
        // method 'playbackFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<double> &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<double> &, std::false_type>,
        // method 'playbackStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setCurrentAngles'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<double> &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<double> &, std::false_type>,
        // method 'onRobotAvailabilityChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onSave'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLoad'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAddSegment'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAddHandSegment'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDeleteSegment'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDuplicateSegment'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPlay'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPause'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onStop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPlaybackStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPlaybackFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPlaybackFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const JointState &, std::false_type>,
        // method 'onPlaybackProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'onSeek'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'onRealtimeError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onRealtimeStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onSegmentEntered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void TimelineEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TimelineEditor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->playRequested(); break;
        case 1: _t->segmentAdded((*reinterpret_cast< std::add_pointer_t<Segment>>(_a[1]))); break;
        case 2: _t->streamFrequencyChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->previewRequested((*reinterpret_cast< std::add_pointer_t<QList<double>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<double>>>(_a[2]))); break;
        case 4: _t->playbackFrame((*reinterpret_cast< std::add_pointer_t<QList<double>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<double>>>(_a[2]))); break;
        case 5: _t->playbackStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 6: _t->setCurrentAngles((*reinterpret_cast< std::add_pointer_t<QList<double>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<double>>>(_a[2]))); break;
        case 7: _t->onRobotAvailabilityChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 8: _t->onSave(); break;
        case 9: _t->onLoad(); break;
        case 10: _t->onAddSegment(); break;
        case 11: _t->onAddHandSegment(); break;
        case 12: _t->onDeleteSegment(); break;
        case 13: _t->onDuplicateSegment(); break;
        case 14: _t->onPlay(); break;
        case 15: _t->onPause(); break;
        case 16: _t->onStop(); break;
        case 17: _t->onPlaybackStarted(); break;
        case 18: _t->onPlaybackFinished(); break;
        case 19: _t->onPlaybackFrame((*reinterpret_cast< std::add_pointer_t<JointState>>(_a[1]))); break;
        case 20: _t->onPlaybackProgress((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2]))); break;
        case 21: _t->onSeek((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 22: _t->onRealtimeError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 23: _t->onRealtimeStatus((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 24: _t->onSegmentEntered((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<double> >(); break;
            }
            break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<double> >(); break;
            }
            break;
        case 6:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<double> >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (TimelineEditor::*)();
            if (_q_method_type _q_method = &TimelineEditor::playRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (TimelineEditor::*)(const Segment & );
            if (_q_method_type _q_method = &TimelineEditor::segmentAdded; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (TimelineEditor::*)(int );
            if (_q_method_type _q_method = &TimelineEditor::streamFrequencyChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (TimelineEditor::*)(const QVector<double> & , const QVector<double> & );
            if (_q_method_type _q_method = &TimelineEditor::previewRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (TimelineEditor::*)(const QVector<double> & , const QVector<double> & );
            if (_q_method_type _q_method = &TimelineEditor::playbackFrame; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (TimelineEditor::*)(bool );
            if (_q_method_type _q_method = &TimelineEditor::playbackStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject *TimelineEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TimelineEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN14TimelineEditorE.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TimelineEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 25)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 25;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 25)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 25;
    }
    return _id;
}

// SIGNAL 0
void TimelineEditor::playRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void TimelineEditor::segmentAdded(const Segment & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void TimelineEditor::streamFrequencyChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void TimelineEditor::previewRequested(const QVector<double> & _t1, const QVector<double> & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void TimelineEditor::playbackFrame(const QVector<double> & _t1, const QVector<double> & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void TimelineEditor::playbackStateChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
