/****************************************************************************
** Meta object code from reading C++ file 'search_widget.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/aiws/gui/search_widget.hpp"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'search_widget.hpp' doesn't include <QObject>."
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
struct qt_meta_stringdata_aiws__gui__SearchWidget_t {
    uint offsetsAndSizes[26];
    char stringdata0[24];
    char stringdata1[17];
    char stringdata2[1];
    char stringdata3[6];
    char stringdata4[2];
    char stringdata5[14];
    char stringdata6[26];
    char stringdata7[12];
    char stringdata8[23];
    char stringdata9[9];
    char stringdata10[15];
    char stringdata11[21];
    char stringdata12[24];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_aiws__gui__SearchWidget_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_aiws__gui__SearchWidget_t qt_meta_stringdata_aiws__gui__SearchWidget = {
    {
        QT_MOC_LITERAL(0, 23),  // "aiws::gui::SearchWidget"
        QT_MOC_LITERAL(24, 16),  // "search_requested"
        QT_MOC_LITERAL(41, 0),  // ""
        QT_MOC_LITERAL(42, 5),  // "query"
        QT_MOC_LITERAL(48, 1),  // "k"
        QT_MOC_LITERAL(50, 13),  // "query_changed"
        QT_MOC_LITERAL(64, 25),  // "source_document_requested"
        QT_MOC_LITERAL(90, 11),  // "document_id"
        QT_MOC_LITERAL(102, 22),  // "selected_chunk_changed"
        QT_MOC_LITERAL(125, 8),  // "chunk_id"
        QT_MOC_LITERAL(134, 14),  // "request_search"
        QT_MOC_LITERAL(149, 20),  // "show_selected_result"
        QT_MOC_LITERAL(170, 23)   // "request_source_document"
    },
    "aiws::gui::SearchWidget",
    "search_requested",
    "",
    "query",
    "k",
    "query_changed",
    "source_document_requested",
    "document_id",
    "selected_chunk_changed",
    "chunk_id",
    "request_search",
    "show_selected_result",
    "request_source_document"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_aiws__gui__SearchWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   56,    2, 0x06,    1 /* Public */,
       5,    1,   61,    2, 0x06,    4 /* Public */,
       6,    1,   64,    2, 0x06,    6 /* Public */,
       8,    1,   67,    2, 0x06,    8 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      10,    0,   70,    2, 0x08,   10 /* Private */,
      11,    0,   71,    2, 0x08,   11 /* Private */,
      12,    0,   72,    2, 0x08,   12 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::Int,    3,    4,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    9,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject aiws::gui::SearchWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_aiws__gui__SearchWidget.offsetsAndSizes,
    qt_meta_data_aiws__gui__SearchWidget,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_aiws__gui__SearchWidget_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SearchWidget, std::true_type>,
        // method 'search_requested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'query_changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'source_document_requested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'selected_chunk_changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'request_search'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'show_selected_result'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'request_source_document'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void aiws::gui::SearchWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SearchWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->search_requested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 1: _t->query_changed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->source_document_requested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->selected_chunk_changed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->request_search(); break;
        case 5: _t->show_selected_result(); break;
        case 6: _t->request_source_document(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SearchWidget::*)(const QString & , int );
            if (_t _q_method = &SearchWidget::search_requested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SearchWidget::*)(const QString & );
            if (_t _q_method = &SearchWidget::query_changed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SearchWidget::*)(const QString & );
            if (_t _q_method = &SearchWidget::source_document_requested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SearchWidget::*)(const QString & );
            if (_t _q_method = &SearchWidget::selected_chunk_changed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject *aiws::gui::SearchWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *aiws::gui::SearchWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_aiws__gui__SearchWidget.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "Resettable"))
        return static_cast< Resettable*>(this);
    return QWidget::qt_metacast(_clname);
}

int aiws::gui::SearchWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void aiws::gui::SearchWidget::search_requested(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void aiws::gui::SearchWidget::query_changed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void aiws::gui::SearchWidget::source_document_requested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void aiws::gui::SearchWidget::selected_chunk_changed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
