#include <QWidget>
#include "base/vval.h"
#include <vector>
#include <string>
#include <QFont>
#include <QStaticText>

class LineEditor : public QWidget
{
    Q_OBJECT

private:
    std::vector<QStaticText>        m_lines;
    VVal::VV                        m_line_meta;
    QFont                           m_font;
    bool                            m_update_scroll_after_paint;
    int                             m_shown_lines;
    int                             m_cursor;
    int                             m_scroll;
    std::vector<QColor>             m_colors;
    int                             m_meta_column_width_chars;
    int                             m_meta_column_width;
    int                             m_line_column_width_chars;
    QRect                           m_line_area;
    int                             m_pad_col_width_chars;
    VVal::VV                        m_top_line;
    VVal::VV                        m_bottom_line;
    int                             m_mouse_wheel_delta;
    int                             m_scroll_indicator_width;

    std::vector<QImage *>          m_pixmaps;

    int  line_column_width();
    int  line_column_width_chars();
    void rewrap();
    void update_cursor_scroll();
    void get_colors_for_line_idx(int idx, QColor &text, QBrush &bg);
    void layout_special_text(QPainter &painter, int x, int y, const VVal::VV &text, QBrush &defaultBg, QColor &defaultFg, int width = -1);
    int calc_meta_column_width();

public:
    enum Colors
    {
        C_DEFAULT_FG,
        C_LIGHT_BG,
        C_DARK_BG,
        C_CURSOR_FG,
        C_CURSOR_BG,
        C_SELECT_FG,
        C_SELECT_BG,
        C_HIGHLIGHT_FG,
        C_HIGHLIGHT_BG,
        C_DIVIDER_FG,
        C_SCROLL_IND,
        C_CUSTOM_COLORS_IDX
    };
    enum TagLine
    {
        TL_TOP,
        TL_BOTTOM
    };
    LineEditor(QWidget *parent = 0);

    VVal::VV lines() { return m_line_meta; }
    void set_lines(const VVal::VV &lines);
    void set_pixmap(int idx, QImage *pixmap)
    {
        if (idx >= m_pixmaps.size())
            m_pixmaps.resize(idx + 1);
        m_pixmaps[idx] = pixmap;
    }
    void set_tag_line(TagLine tl, const VVal::VV &specialText)
    {
        if (tl == TL_TOP)
            m_top_line = specialText;
        else
            m_bottom_line = specialText;
        update();
    }
    void set_pad_col_width(int padChars)
    {
        m_pad_col_width_chars = padChars;
        rewrap();
        update();
    }
    void set_meta_col_width(int widthChars)
    {
        m_meta_column_width_chars = widthChars;
        rewrap();
        update();
    }
    void set_color(int idx, int r, int g, int b)
    {
        if (idx >= m_colors.size())
            m_colors.resize(idx + 1);
        m_colors[idx] = QColor(r, g, b);
    }

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
};

