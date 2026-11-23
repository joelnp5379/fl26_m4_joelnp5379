/****************************************************************************
** Meta object code from reading C++ file 'workspace_widget.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/aiws/gui/workspace_widget.hpp"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'workspace_widget.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_aiws__gui__WorkspaceWidget_t {
    uint offsetsAndSizes[12];
    char stringdata0[27];
    char stringdata1[23];
    char stringdata2[1];
    char stringdata3[23];
    char stringdata4[18];
    char stringdata5[4];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_aiws__gui__WorkspaceWidget_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_aiws__gui__WorkspaceWidget_t qt_meta_stringdata_aiws__gui__WorkspaceWidget = {
    {
        QT_MOC_LITERAL(0, 26),  // "aiws::gui::WorkspaceWidget"
        QT_MOC_LITERAL(27, 22),  // "add_document_requested"
        QT_MOC_LITERAL(50, 0),  // ""
        QT_MOC_LITERAL(51, 22),  // "build_corpus_requested"
        QT_MOC_LITERAL(74, 17),  // "document_selected"
        QT_MOC_LITERAL(92, 3)   // "row"
    },
    "aiws::gui::WorkspaceWidget",
    "add_document_requested",
    "",
    "build_corpus_requested",
    "document_selected",
    "row"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_aiws__gui__WorkspaceWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   32,    2, 0x06,    1 /* Public */,
       3,    0,   33,    2, 0x06,    2 /* Public */,
       4,    1,   34,    2, 0x06,    3 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    5,

       0        // eod
};

Q_CONSTINIT const QMetaObject aiws::gui::WorkspaceWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_aiws__gui__WorkspaceWidget.offsetsAndSizes,
    qt_meta_data_aiws__gui__WorkspaceWidget,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_aiws__gui__WorkspaceWidget_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<WorkspaceWidget, std::true_type>,
        // method 'add_document_requested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'build_corpus_requested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'document_selected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void aiws::gui::WorkspaceWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<WorkspaceWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->add_document_requested(); break;
        case 1: _t->build_corpus_requested(); break;
        case 2: _t->document_selected((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (WorkspaceWidget::*)();
            if (_t _q_method = &WorkspaceWidget::add_document_requested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (WorkspaceWidget::*)();
            if (_t _q_method = &WorkspaceWidget::build_corpus_requested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (WorkspaceWidget::*)(int );
            if (_t _q_method = &WorkspaceWidget::document_selected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *aiws::gui::WorkspaceWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *aiws::gui::WorkspaceWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_aiws__gui__WorkspaceWidget.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "Resettable"))
        return static_cast< Resettable*>(this);
    return QWidget::qt_metacast(_clname);
}

int aiws::gui::WorkspaceWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void aiws::gui::WorkspaceWidget::add_document_requested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void aiws::gui::WorkspaceWidget::build_corpus_requested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void aiws::gui::WorkspaceWidget::document_selected(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
