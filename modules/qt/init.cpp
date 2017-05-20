#include <QApplication>
#include <QSplitter>
#include <QTimer>
#include "init.h"
#include "line_editor.h"
#include "rt/lua_thread.h"
#include "rt/lua_thread_helper.h"

using namespace VVal;

/* IDEEN LINEEDITOR:

   - Highlight & Selektion sollte in m_line_meta gespeichert sein
   - Cursor-State muss abrufbar & setzbar sein
   - Für die Meta, Info und Meldungszeilen können Text-Farben
     angegeben werden. Die Texte sind dann listen von listen
     und werden "life" gerendert?
   - Neue Theme-Farben müssen definiert werden können als "referenz"?
   - Convenience:
        - Meldungszeile mit Eingabe-Menü/-Möglichkeit
        - Info-Zeile oben

*/

//#ifndef __gnu_linux__
//Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
//#endif

void init()
{
}

//---------------------------------------------------------------------------

static QCoreApplication *s_App = 0;
std::thread::id         m_app_thread_id;

//---------------------------------------------------------------------------

static void check_qt_thread()
{
    if (std::this_thread::get_id() != m_app_thread_id)
    {
        L_FATAL << "Calling a QT function from the wrong thread!";
        throw QtThreadException("Calling a QT function from the wrong thread!");
    }
}
//---------------------------------------------------------------------------

static void check_qt_access()
{
    if (!s_App)
    {
        L_FATAL << "Calling a QT function without running (qt-exec)!";
        throw QtThreadException("Calling a QT function without running (qt-exec)!");
    }

    check_qt_thread();
}
//---------------------------------------------------------------------------

static void register_qt_object(lal_rt::LuaThread *lt, void *objPtr)
{
    lt->register_resource(objPtr);
    QObject::connect(
        ((QObject *) objPtr),
        &QObject::destroyed,
        [=]() {
            // L_TRACE << "DESTROY " << objPtr;
            lt->delete_resource(objPtr); });
}
//---------------------------------------------------------------------------

QWidget *qwidget(const VVal::VV &vv_obj, const std::string &func, const VVal::VV &arg, std::string type = "")
{
    if (arg->is_undef()) return 0;

    if (type.empty()) type = arg->type();

    LT->check_resource(arg, type);
    if (arg->type().substr(0, 3) != "Qt_")
    {
        L_FATAL << "Programmer error, passed non Qt object to " << func;
        throw QtThreadException("Passed non Qt object to " + func);
    }

    return (QWidget *) arg->p(arg->type());
}
//---------------------------------------------------------------------------

QWidget *qwidget(const VVal::VV &vv_obj, const std::string &func, const VVal::VV &args, int idx, const std::string &type = "")
{
    return qwidget(vv_obj, func + " " + std::to_string(idx), args->_(idx), type);
}
//---------------------------------------------------------------------------

class QtAppEventLoop : public lal_rt::EventLoop
{
    public:
        QtAppEventLoop() { }
        virtual ~QtAppEventLoop() { }

        // An idea from SO:
        // http://stackoverflow.com/questions/21646467/how-to-execute-a-functor-in-a-given-thread-in-qt-gcd-style/21653558#21653558
        // void postToMainThread(const std::function<void()> & fun) {
        //   QObject signalSource;
        //   QObject::connect(&signalSource, &QObject::destroyed, qApp, [=](QObject*){
        //     fun();
        //   });
        // }
        virtual void unsafe_notify_msg_arrived(lal_rt::Process *proc)
        {
            QObject signalSource;
            QObject::connect(
                &signalSource, &QObject::destroyed, s_App,
                [this, proc](QObject *) {
                    m_handle_messages_synchronized();

                    if (proc->is_terminated())
                        s_App->quit();
                });
        }
};
//---------------------------------------------------------------------------

void singleShot(std::function<void()> func)
{
    QTimer* timer = new QTimer();
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [=](){
        func();
        timer->deleteLater();
    });
    timer->start(0);
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(qt_exec,
"@qt procedure (qt-exec [_init-cb_])\n"
"Start the Qt event loop.\n"
"_init-cb_ can be a callback or a message, which will be emitted to\n"
"the current process (and this message can be received using a default.\n"
"handler for messages)\n"
"\n"
"   (begin\n"
"     ; Send a message to yourself (proc-pid), it will be called by\n"
"     ; the Qt event loop in (qt-exec).\n"
"     (mp-send (proc-pid) [after-init:])\n"
"     (mp-add-default-handler\n"
"      (lambda (msg)\n"
"        (when (eqv? (@2 msg) after-init:)\n"
"          #;(called once the main event loop executes.\n"
"             do initialization stuff here!))))\n"
"     (qt-exec))\n"
"\nAlternatively:\n"
"   (begin\n"
"     ; Let qt-exec send the message:\n"
"     (mp-add-default-handler\n"
"      (lambda (msg)\n"
"        (when (eqv? (@2 msg) after-init:)\n"
"          #;(called once the main event loop executes.\n"
"             do initialization stuff here!))))\n"
"     (qt-exec after-init:))\n"
"\nAlternatively:\n"
"   (begin\n"
"     (qt-exec (lambda () #;(called once the main event loop executes.))))\n"
)
{
    if (s_App)
        return vv_undef();
    int argc = 0;
    s_App = new QApplication(argc, nullptr);
    s_App->setApplicationName("LALRTQT");
    m_app_thread_id = std::this_thread::get_id();

    QtAppEventLoop evl;

    LT->set_event_loop(&evl);
    // Trigger event loop to process any messages that arrived before this call:
    singleShot([=, &evl]() {
        evl.unsafe_notify_msg_arrived(LT);
        if (vv_args->_(0)->is_defined())
            LT->call_lua_func_or_emit(vv_args->_(0), vv_undef());
    });
    s_App->exec();

    delete s_App;
    s_App = nullptr;

    return vv_undef();
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(qt_quit,
"@qt procedure (qt-quit)\n"
"Exit the Qt event loop.\n"
)
{
    check_qt_access();
    if (!s_App)
        return vv_undef();
    s_App->quit();
    return vv_undef();
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(qt_splitter_create,
"@qt procedure (qt-splitter-create _parent_ [_widgets_])\n\n"
"Creates a splitter widget on the _parent_, for _widgets_.\n"
"\n")
{
    check_qt_access();

    QWidget *parent = qwidget(vv_obj, "qt-splitter-create", vv_args, 0);
    QSplitter *spl = new QSplitter(parent);
    register_qt_object(LT, spl);

    for (auto child : *(vv_args->_(1)))
    {
        QWidget *child_wid = qwidget(vv_obj, "qt-splitter-create", child);
        spl->addWidget(child_wid);
    }

    return vv_ptr((void *) spl, "Qt_Splitter");
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(qt_line_edit_set_lines,
"@qt procedure (qt-line-edit-set-lines _line-edit-obj_ _list-of-lines_)\n\n"
"Sets the lines which the line editor should show.\n"
"The _list-of-lines_ is a list of line structures, which define how the\n"
"line is displayed. Each line is itself a list of following attributes:\n"
"\n"
"    [\n"
"     _string-special-text_       - Text label on variable width left part.\n"
"     _bool-is-selected_          - Should be true, if the line is selected.\n"
"     _bool-is-highlighted-       - Should be true, if the line is highlighted.\n"
"     _string-special-text-right_ - Text label with fixed width to the right.\n"
"                                   Usabel for meta information.\n"
"                                   Width can be set with\n"
"                                   (qt-line-edit-set-meta-col-width)\n"
"     _string-special-text-left_  - The text in the left most padding area.\n"
"                                   Usable for displaying small icons/marks.\n"
"                                   Width can be set with\n"
"                                   (qt-line-edit-set-pad-col-width)\n"
"    ]\n"
"\n"
"A _special-text_ can be just a simple text string, or list, that can contain\n"
"following values to control how it is displayed:\n"
"\n"
"    _string_    - Will be the displayed text\n"
"    _number_    - A positive number sets the foreground color of the next text\n"
"                   A negative number sets the background color of the next text\n"
"    _list_      - A meta element, the first element defines the type.\n"
"\n"
"Following meta elements are defined:\n"
"\n"
"    [image: _image-index_]     - Displays an inline image, images can be loaded\n"
"                                 using (qt-line-edit-set-image).\n")
{
    check_qt_access();

    LineEditor *le =
        (LineEditor *)
        qwidget(vv_obj, "qt-line-edit-set-lines", vv_args, 0, "Qt_LALQtLineEditor");

    le->set_lines(vv_args->_(1));
    return vv_undef();
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(qt_line_edit_create,
"@qt procedure (qt-line-edit-create _parent_ _size-list_)\n\n"
"Creates a line editor view with window size according to _size-list_.\n"
"It returns an object handle for further reference.\n"
"The handle needs to be destroyed using `(qt-delete)`\n"
"\n"
"    (define line-edit (qt-line-edit-create [600, 300]))\n"
"    (qt-show line-edit)\n"
"    (qt-reg line-edit key-press:\n"
"     (lambda (key) (when (= ($name: key) 'q') (qt-delete line-edit))))\n"
)
{
    check_qt_access();

    auto *o = new LineEditor(qwidget(vv_obj, "qt-line-edit-create", vv_args, 0));
    register_qt_object(LT, o);

    return vv_ptr((void *) o, "Qt_LALQtLineEditor");
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(qt_delete,
"@qt procedure (qt-delete _object_)\n"
"Deletes/Destroys an object/pointer that was created using the\n"
"qt-* API.\n"
"\n"
"   (let ((le (qt-line-edit-create ...)))\n"
"     ...\n"
"     (qt-delete le))\n"
)
{
    check_qt_thread();

    VVal::VV obj = vv_args->_(0);

    void *o = obj->p(obj->type());
    LT->check_resource(obj, obj->type());

    if (obj->type() == "Qt_LALQtLineEditor")
        if (s_App)
            ((LineEditor *) o)->deleteLater();
        else
            delete ((LineEditor *) o);
    else if (obj->type() == "Qt_Splitter")
        if (s_App)
            ((QSplitter *) o)->deleteLater();
        else
            delete ((QSplitter *) o);

    return vv_undef();
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(qt_resize,
"@qt procedure (qt-resize _object_ [_w_ _h_])\n\n"
"Calls QWidget::resize on _object_.\n"
"\n"
"    (define q-wid (qt-splitter-create ...))\n"
"    (qt-resize q-wid [300 300])\n"
)
{
    QWidget *w = qwidget(vv_obj, "qt-resize", vv_args, 0);
    w->resize(
        vv_args->_(1)->_i(0),
        vv_args->_(1)->_i(1));
    return vv_undef();
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(qt_show,
"@qt procedure (qt-show _object_)\n"
"Calls QWidget::show on _object_.\n"
)
{
    QWidget *w = qwidget(vv_obj, "qt-show", vv_args, 0);
    w->show();
    return vv_undef();
}
//---------------------------------------------------------------------------

VV_CLOSURE_DOC(qt_hide,
"@qt procedure (qt-hide _object_)\n"
"Calls QWidget::hide on _object_.\n"
)
{
    check_qt_access();

    LT->check_resource(vv_args->_(0), vv_args->_(0)->type());
    QWidget *w = (QWidget *) vv_args->_(0)->p(vv_args->_(0)->type());
    w->hide();
    return vv_undef();
}
//---------------------------------------------------------------------------

void init_qtlib(lal_rt::LuaThread *t, Lua::Instance &lua)
{
    VV obj(vv_list() << vv_ptr(t, "LuaThread"));

    LUA_REG(lua, "qt", "exec",             obj, qt_exec);
    LUA_REG(lua, "qt", "quit",             obj, qt_quit);
    LUA_REG(lua, "qt", "lineEditCreate",   obj, qt_line_edit_create);
    LUA_REG(lua, "qt", "lineEditSetLines", obj, qt_line_edit_set_lines);
    LUA_REG(lua, "qt", "splitterCreate",   obj, qt_splitter_create);
    LUA_REG(lua, "qt", "delete",           obj, qt_delete);

    LUA_REG(lua, "qt", "show",             obj, qt_show);
    LUA_REG(lua, "qt", "resize",           obj, qt_resize);
    LUA_REG(lua, "qt", "hide",             obj, qt_hide);

//    std::cout << "X" << std::endl;
//    int argc = 0;
//    s_App = new QApplication(argc, nullptr);
//    s_App->setApplicationName("LALRTQT");
//    s_App->setApplicationVersion("1");
//
//    QTimer::singleShot(200, &init);
////    QWindow win;
////    win.addWidget(&spl);
////    win.show();
////    bool ok;
////    QString text =
////        QInputDialog::getText(
////                nullptr,
////                "QInputDialog::getText()",
////                "User name:",
////                QLineEdit::Normal,
////                QDir::home().dirName(),
////                &ok);
//
//    s_App->exec();

    //    LUA_REG(lua, "sys", "fileExistsQ", obj, file_exists_Q);
}
