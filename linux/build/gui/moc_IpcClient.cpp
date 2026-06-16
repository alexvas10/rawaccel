/****************************************************************************
** Meta object code from reading C++ file 'IpcClient.hpp'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../gui/IpcClient.hpp"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'IpcClient.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
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
struct qt_meta_tag_ZN9IpcClientE_t {};
} // unnamed namespace

template <> constexpr inline auto IpcClient::qt_create_metaobjectdata<qt_meta_tag_ZN9IpcClientE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "IpcClient",
        "mouseEventReceived",
        "",
        "x",
        "y",
        "time_ms",
        "connectionChanged",
        "connected",
        "tryConnect",
        "onSubDataReady",
        "onSubDisconnected"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'mouseEventReceived'
        QtMocHelpers::SignalData<void(double, double, double)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 3 }, { QMetaType::Double, 4 }, { QMetaType::Double, 5 },
        }}),
        // Signal 'connectionChanged'
        QtMocHelpers::SignalData<void(bool)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 7 },
        }}),
        // Slot 'tryConnect'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSubDataReady'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSubDisconnected'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<IpcClient, qt_meta_tag_ZN9IpcClientE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject IpcClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9IpcClientE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9IpcClientE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9IpcClientE_t>.metaTypes,
    nullptr
} };

void IpcClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<IpcClient *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->mouseEventReceived((*reinterpret_cast<std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[3]))); break;
        case 1: _t->connectionChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 2: _t->tryConnect(); break;
        case 3: _t->onSubDataReady(); break;
        case 4: _t->onSubDisconnected(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (IpcClient::*)(double , double , double )>(_a, &IpcClient::mouseEventReceived, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (IpcClient::*)(bool )>(_a, &IpcClient::connectionChanged, 1))
            return;
    }
}

const QMetaObject *IpcClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *IpcClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9IpcClientE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int IpcClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void IpcClient::mouseEventReceived(double _t1, double _t2, double _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2, _t3);
}

// SIGNAL 1
void IpcClient::connectionChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}
QT_WARNING_POP
