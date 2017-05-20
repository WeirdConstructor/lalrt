#include "line_editor.h"
#include <QApplication>
#include <QPainter>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QInputDialog>
#include <QLineEdit>
#include <QKeyEvent>
#include <QDir>
#include <QSplitter>
#include <QWindow>
#include <rt/log.h>

using namespace VVal;

LineEditor::LineEditor(QWidget *parent)
    : m_cursor(0), m_scroll(0), m_pad_col_width_chars(2), m_mouse_wheel_delta(0),
      m_line_meta(vv_undef()), m_scroll_indicator_width(4)

{
//    setWindowTitle("Test Wid");
//    resize(300, 500);
    setFocusPolicy(Qt::StrongFocus);
    m_font = QFont("DejaVu Sans Mono", 10);

    m_colors.resize(11);
    m_colors[C_DEFAULT_FG]   = QColor(0xCC, 0xCC, 0xCC);
    m_colors[C_LIGHT_BG]     = QColor(0x33, 0x33, 0x33);
    m_colors[C_DARK_BG]      = QColor(0x22, 0x22, 0x22);
    m_colors[C_CURSOR_FG]    = QColor(0x33, 0x33, 0x33);
    m_colors[C_CURSOR_BG]    = QColor(0x90, 0xEE, 0x90);
    m_colors[C_SELECT_FG]    = QColor(0x00, 0x00, 0x00);
    m_colors[C_SELECT_BG]    = QColor(0xA9, 0xA9, 0xA9);
    m_colors[C_HIGHLIGHT_FG] = QColor(0x00, 0x00, 0x00);
    m_colors[C_HIGHLIGHT_BG] = QColor(0xFF, 0x00, 0x00);
    m_colors[C_DIVIDER_FG]   = QColor(0x90, 0xEE, 0x90);
    m_colors[C_SCROLL_IND]   = QColor(0xEE, 0xEE, 0xEE);
    m_meta_column_width = 0;
    m_meta_column_width_chars = 0;
}

void LineEditor::get_colors_for_line_idx(int idx, QColor &text, QBrush &bg)
{
    if (idx == m_cursor && hasFocus())
    {
        text = m_colors[C_CURSOR_FG];
        bg   = m_colors[C_CURSOR_BG];
    }
    else if (m_line_meta->_(idx)->_b(1))
    {
        text = m_colors[C_SELECT_FG];
        bg   = m_colors[C_SELECT_BG];
    }
    else if (m_line_meta->_(idx)->_b(2))
    {
        text = m_colors[C_HIGHLIGHT_FG];
        bg   = m_colors[C_HIGHLIGHT_BG];
    }
    else
    {
        text = m_colors[C_DEFAULT_FG];
        bg = idx % 2 == 0 ? m_colors[C_LIGHT_BG] : m_colors[C_DARK_BG];
    }
}

void LineEditor::layout_special_text(QPainter &painter, int x, int y, const VVal::VV &text,
                                   QBrush &defaultBg, QColor &defaultFg, int width)
{
    QFontMetrics fm(m_font);
    int h = fm.height();

    painter.setPen(defaultFg);
    QBrush bg = defaultBg; 

    for (auto elem : *text)
    {
        if (elem->is_string())
        {
            std::string s = elem->s();
            QString qs(QString::fromUtf8(s.data(), (int) s.size()));

            if (width > 0)
            {
                if (qs.size() > m_line_column_width_chars)
                    qs = qs.mid(0, m_line_column_width_chars < 4
                                   ? 3
                                   : m_line_column_width_chars - 1)
                         + ">";
            }

            int w = fm.width(qs);

            painter.fillRect(QRect(x, y, w, h), bg);
            painter.drawText(x, y, w, h, 0, qs, nullptr);

            x += w;
            bg = defaultBg;
            painter.setPen(defaultFg);
        }
        else if (elem->is_int() || elem->is_double())
        {
            int clr = elem->i();
            if (clr >= 0)
            {
                if (clr < m_colors.size())
                    painter.setPen(m_colors[clr]);
            }
            else
            {
                if ((-clr) < m_colors.size())
                    bg = QBrush(m_colors[-clr]);
            }
        }
        else if (elem->is_list())
        {
            if (elem->_s(0) == "image")
            {
                if (((size_t) elem->_i(1)) < m_pixmaps.size())
                {
                    QImage *pm = m_pixmaps[elem->_i(1)];
                    painter.drawImage(x, y, *pm);
                    x += pm->width();
                }
            }
        }
    }
}

void LineEditor::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setFont(m_font);

    QRect r(rect());
    painter.fillRect(r, m_colors[C_DARK_BG]);

    int divider_x = m_line_area.x() + m_line_area.width();

    QFontMetrics fm(m_font);

    int x_pad_right = m_scroll_indicator_width;
    int y         = m_line_area.y();
    int line_idx  = 0;
    int line_h    = fm.height();
    for (auto line : *m_line_meta)
    {
        if (m_scroll > line_idx)
        {
            line_idx++;
            continue;
        }

        if ((line_h + y) > (m_line_area.y() + m_line_area.height()))
            break;

        QBrush bg;
        QColor fg;
        get_colors_for_line_idx(line_idx, fg, bg);

        painter.fillRect(QRect(0, y, r.width() - x_pad_right, line_h), bg);
        layout_special_text(
            painter, m_line_area.x(), y, line->_(0), bg, fg, m_line_area.width());

        if (m_line_meta->_(line_idx)->_(4)->is_defined())
            layout_special_text(painter, 0, y,
                              m_line_meta->_(line_idx)->_(4),
                              bg, fg);

        if (m_line_meta->_(line_idx)->_(3)->is_defined())
            layout_special_text(painter, divider_x, y,
                              m_line_meta->_(line_idx)->_(3),
                              bg, fg);

        if (line_idx == m_cursor && !hasFocus())
        {
            QPen p(m_colors[C_CURSOR_BG]);
            p.setWidth(1);
            painter.setPen(p);
            painter.drawRect(QRectF(0.5, y + 0.5, (r.width() - 1) - x_pad_right, line_h - 1));
        }

        y += line_h;
        line_idx++;
    }

    QBrush bg(m_colors[C_DARK_BG]);
    if (m_top_line)
        layout_special_text(painter, 0, 0,
                          m_top_line, bg, m_colors[C_DEFAULT_FG]);

    if (m_bottom_line)
        layout_special_text(painter, 0, m_line_area.y() + m_line_area.height(),
                          m_bottom_line, bg, m_colors[C_DEFAULT_FG]);

    // Then all lines are same height (assumed, because we need monospaced font anyways)
    m_shown_lines = m_line_area.height() / line_h;

    painter.setPen(m_colors[C_DIVIDER_FG]);
    painter.drawLine(QPointF(divider_x - 0.5, m_line_area.y() + 0.5),
                     QPointF(divider_x - 0.5,   (m_line_area.y() - 0.5)
                                              + (line_h * m_shown_lines)));

    if (m_line_meta->size() > 0)
    {
        QPen p(m_colors[C_SCROLL_IND]);
        p.setWidth(m_scroll_indicator_width);
        painter.setPen(p);

        int m_scrol_ind_min_h = 10;
        int m_scrol_ind_h = m_line_area.height() / m_line_meta->size();
        if (m_scrol_ind_h < m_scrol_ind_min_h)
            m_scrol_ind_h = m_scrol_ind_min_h;

        int m_scrol_ind_rh = m_scrol_ind_h + (m_scroll_indicator_width / 2);

        int avail_h = line_h * m_shown_lines - m_scrol_ind_rh;

        double scroll_pos_perc =
             (double) m_cursor / ((double) m_line_meta->size() - 1);

        painter.drawLine(
            QPointF(r.width() - (m_scroll_indicator_width / 2),
                    m_line_area.y()
                    + (scroll_pos_perc * avail_h)
                    + (m_scroll_indicator_width / 2)),
            QPointF(r.width() - (m_scroll_indicator_width / 2),
                    m_line_area.y()
                    + (scroll_pos_perc * avail_h)
                    + m_scrol_ind_h));
//        L_TRACE << "DARW << " << m_cursor << "," << m_line_meta->size() << "," << scroll_pos_perc;
    }

    if (m_update_scroll_after_paint)
        update_cursor_scroll();
}

int LineEditor::calc_meta_column_width()
{
    QFontMetrics fm(m_font);
    QString measureChar("m");
    QString ms = measureChar.repeated(m_meta_column_width_chars);
    return m_meta_column_width = fm.width(ms);
}

int LineEditor::line_column_width()
{
    QFontMetrics fm(m_font);
    return (rect().width()
            - calc_meta_column_width()
            - m_scroll_indicator_width)
           - fm.width(QString("mm"));
}

int LineEditor::line_column_width_chars()
{
    int width = line_column_width();

    QFontMetrics fm(m_font);
    int len = 1;
    QString measureChar("m");
    QString ms = measureChar.repeated(len);

    while (fm.width(ms) < width)
    {
        len++;
        ms = measureChar.repeated(len);
    }

    return len - 1;
}

void LineEditor::rewrap()
{
    int lcw  = line_column_width();
    m_line_column_width_chars = line_column_width_chars();

    QFontMetrics fm(m_font);

    m_line_area =
        QRect(
            fm.averageCharWidth() * m_pad_col_width_chars,
            fm.height(),
            lcw,
            rect().height() - (2 * fm.height()));

//    QTextOption opt;
//    opt.setWrapMode(QTextOption::NoWrap);
//
//    m_lines.clear();
//    if (!m_line_meta)
//        return;
//
//    for (auto l : *m_line_meta)
//    {
//        std::string s = l->_s(0);
//
//        QString qs(QString::fromUtf8(QByteArray(s.data(), s.size())));
//        if (qs.size() > lcwc)
//            qs = qs.mid(0, lcwc < 4 ? 3 : lcwc - 1) + ">";
//
//        QStaticText st(qs);
//        st.setTextFormat(Qt::PlainText);
//        st.setTextOption(opt);
//        st.setTextWidth(lcw);
//        m_lines.push_back(st);
//    }
}

void LineEditor::set_lines(const VVal::VV &lines)
{
    m_line_meta = lines;
    rewrap();
    update();
}

void LineEditor::resizeEvent(QResizeEvent *event)
{
    m_update_scroll_after_paint = true;
    rewrap();
    update();
}

void LineEditor::update_cursor_scroll()
{
    m_update_scroll_after_paint = false;

//    std::cout << ">CURSOR=" << m_cursor << ", SCROLL=" << m_scroll << ", SL=" << m_shown_lines << std::endl;
    int line_count = m_line_meta->size();

    if (m_cursor < 0)                m_cursor = 0;
    else if (m_cursor >= line_count) m_cursor = line_count - 1;

    int cursor_view_pos = m_cursor - m_scroll;

    if (m_shown_lines < 10)
        m_scroll = m_cursor - (m_shown_lines / 2);
    else
    {
        if (m_cursor < m_scroll + 3)
            m_scroll = m_cursor - 3;
        else if (m_cursor >= m_scroll + (m_shown_lines - 3))
            m_scroll = m_cursor - (m_shown_lines - 3);

        if (m_scroll < 0)
            m_scroll = 0;
        else if (m_scroll > (line_count - m_shown_lines))
            m_scroll = line_count - m_shown_lines;
    }

    if (line_count <= m_shown_lines)
        m_scroll = 0;

//    std::cout << "<CURSOR=" << m_cursor << ", SCROLL=" << m_scroll << std::endl;

    update();
}

void LineEditor::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
//        QString key = QKeySequence(event->key()).toString();

        if (event->key() == Qt::Key_D)
            m_cursor += m_shown_lines / 2;
        else if (event->key() == Qt::Key_U)
            m_cursor -= m_shown_lines / 2;
    }
    else
    {
        if (event->text() == "j")
            m_cursor++;
        else if (event->text() == "k")
            m_cursor--;
    }

    update_cursor_scroll();
}

void LineEditor::mousePressEvent(QMouseEvent *event)
{
    QFontMetrics fm(m_font);
    QRect full_line_area = m_line_area;
    full_line_area.setLeft(0);
    full_line_area.setWidth(rect().width());

    if (full_line_area.contains(event->x(), event->y()))
    {
        int inside_y_offs = event->y() - m_line_area.y();
        int line_idx = inside_y_offs / fm.height();

        m_cursor = line_idx + m_scroll;
//        std::cout << "CUR" << inside_y_offs << ";" << line_idx << ";" << m_scroll << " => " << m_cursor << std::endl;
        update_cursor_scroll();
    }
//    std::cout << "MOUSE" << std::endl;
}

void LineEditor::wheelEvent(QWheelEvent *event)
{
    m_mouse_wheel_delta += event->delta();

    int amount = abs(m_mouse_wheel_delta);
    amount = amount / 8;

    if (amount > 15)
    {
        if (m_mouse_wheel_delta < 0)
        {
            m_mouse_wheel_delta += 15 * 8;
            if (event->modifiers() & Qt::ShiftModifier)
                m_cursor += m_shown_lines / 2;
            else
                m_cursor++;
        }
        else
        {
            m_mouse_wheel_delta -= 15 * 8;
            if (event->modifiers() & Qt::ShiftModifier)
                m_cursor -= m_shown_lines / 2;
            else
                m_cursor--;
        }
    }
    std::cout << "WHEEL" << std::endl;

    update_cursor_scroll();
    event->accept();
}


void example_line_editor()
{
    LineEditor *edit = new LineEditor;
    LineEditor *edit2 = new LineEditor;

    QImage *q = new QImage;
    std::cout << "FOO" << std::endl;
    if (!q->load("lalrtlib/res/png/font-awesome/folder-o.png"))
    {
        std::cout << "ERROR LOADING PIXMAP" << std::endl;
    }
    std::cout << "FOO" << std::endl;
    edit->set_pixmap(0, q);

    edit->set_meta_col_width(10);
    std::cout << "FOO" << std::endl;
    edit->set_color(20, 0xff, 0x22, 0xff);
    edit->set_color(21, 0xff, 0xFF, 0xff);
    std::cout << "FOO" << std::endl;
    std::cout << "FOO" << std::endl;
    edit->set_tag_line(LineEditor::TL_TOP,    vv_list() << "XX" << 20 << "Y" << -21 << "FOO");
    edit->set_tag_line(LineEditor::TL_BOTTOM, vv_list() << "FOOBAR");
    std::cout << "FOO" << std::endl;
    VVal::VV lines = (vv_list()
        << (vv_list() << vv("test line 1") << (1) << (0) << (vv_list() << ("Teßt") << ("ABC")))
        << (vv_list() << vv("test line 1") << (0) << (0) << (vv_list() << ("XÄXÜ") << (LineEditor::C_HIGHLIGHT_BG) << ("ABC")))
        << (vv_list() << vv("test line 2") << (0) << (1))
        << (vv_list() << vv("test line 3") << vv_undef() << vv_undef() << vv_undef() << (vv_list() << LineEditor::C_HIGHLIGHT_FG << -LineEditor::C_HIGHLIGHT_BG << (vv_list() << "image" << 0)))
        << (vv_list() << vv("test line 4") << vv_undef() << vv_undef() << vv_undef() << "D")
        << (vv_list() << vv("test line 5"))
        << (vv_list() << vv("test line 6"))
        << (vv_list() << vv("test line 6a"))
        << (vv_list() << vv("test line 6b"))
        << (vv_list() << vv("test line 6c"))
        << (vv_list() << vv("test <b>line</b> 6d"))
        << (vv_list() << vv("test line 6e"))
        << (vv_list() << vv("test line 6f"))
        << (vv_list() << vv("test line 6g"))
        << (vv_list() << vv("test line 6h"))
        << (vv_list() << vv("test line 6i"))
        << (vv_list() << vv("test line 6j"))
        << (vv_list() << vv("test line 6k"))
        << (vv_list() << vv("test line 6l"))
        << (vv_list() << vv("test line 6m"))
        << (vv_list() << vv("test line 6n"))
        << (vv_list() << vv("test line 6o"))
        << (vv_list() << vv("test line 6p"))
        << (vv_list() << vv("test line 6q"))
        << (vv_list() << vv("test line 6r"))
        << (vv_list() << vv("test line 6s"))
        << (vv_list() << vv("test line 7"))
        << (vv_list() << vv("test line 8"))
        << (vv_list() << vv("test line 9"))
        << (vv_list() << vv("test line 10"))
        << (vv_list() << vv("test line 11")));
    std::cout << "FOO" << std::endl;
    for (int i = 0; i < 10000; i++)
    {
        lines << (vv_list() << (format("foo %1%") % i).str());
    }
    std::cout << "FOO" << std::endl;
    edit->set_lines(lines);
    std::cout << "FOO" << std::endl;
    edit2->set_lines(vv_list()
        << (vv_list() << vv("test line 6c"))
        << (vv_list() << vv("test <b>line</b> 6d"))
        << (vv_list() << vv("test line 6e"))
        << (vv_list() << vv("test line 6f"))
        << (vv_list() << vv("test line 6g"))
        << (vv_list() << vv("test line 6h"))
        << (vv_list() << vv("test line 6i"))
        << (vv_list() << vv("test line 6j"))
        << (vv_list() << vv("test line 6k"))
        << (vv_list() << vv("test line 6l"))
        << (vv_list() << vv("test line 6m"))
        << (vv_list() << vv("test line 6n")));
    std::cout << "FOO" << std::endl;
    QSplitter *spl = new QSplitter;
    std::cout << "FOO" << std::endl;
    spl->addWidget(edit);
    std::cout << "FOO" << std::endl;
    spl->addWidget(edit2);
    std::cout << "FOO" << std::endl;
    spl->setWindowTitle("Test Wid");
    std::cout << "FOO" << std::endl;
    spl->resize(300, 500);
    std::cout << "FOO" << std::endl;
    spl->show();
}
