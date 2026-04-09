#include "editor.h"

static void viewAppendLineSlice(append_buffer *ab, const char *s, size_t len, size_t left_col, int screen_cols) {
    size_t cap = len * TAB_STOP + 1;
    char *render = malloc(cap ? cap : 1);
    if (render == NULL) return;

    size_t rx = 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '\t') {
            render[rx++] = ' ';
            while (rx % TAB_STOP != 0) render[rx++] = ' ';
        } else {
            render[rx++] = s[i];
        }
    }

    if (left_col < rx) {
        size_t avail = rx - left_col;
        if (avail > (size_t)screen_cols) avail = (size_t)screen_cols;
        abAppend(ab, render + left_col, (int)avail);
    }

    free(render);
}

size_t viewLineVisualX(const char *s, size_t len, size_t col) {
    size_t vx = 0;
    if (col > len) col = len;
    for (size_t i = 0; i < col; i++) {
        if (s[i] == '\t') vx += TAB_STOP - (vx % TAB_STOP);
        else vx++;
    }
    return vx;
}

size_t viewLineBufferCol(const char *s, size_t len, size_t goal_vx) {
    size_t vx = 0;
    for (size_t i = 0; i < len; i++) {
        size_t next = vx;
        if (s[i] == '\t') next += TAB_STOP - (next % TAB_STOP);
        else next++;
        if (next > goal_vx) return i;
        vx = next;
    }
    return len;
}

void viewInit(view *view) {
    view->screen_rows = 0;
    view->screen_cols = 0;
    view->top_line = 0;
    view->left_col = 0;
}

void viewScroll(view *view, const buffer *buf, const cursor *cur) {
    size_t start = bufferLineStart(buf, cur->row);
    size_t len = bufferLineLength(buf, cur->row);
    size_t flat_len = 0;
    char *flat = bufferFlatten(buf, &flat_len);
    size_t vx = viewLineVisualX(flat + start, len, cur->col);
    free(flat);

    if (cur->row < view->top_line) view->top_line = cur->row;
    if (cur->row >= view->top_line + (size_t)view->screen_rows) {
        view->top_line = cur->row - view->screen_rows + 1;
    }

    if (vx < view->left_col) view->left_col = vx;
    if (vx >= view->left_col + (size_t)view->screen_cols) {
        view->left_col = vx - view->screen_cols + 1;
    }
}

void viewDrawRows(append_buffer *ab, const editor *ed, const char *flat) {
    size_t line_count = bufferLineCount(&ed->buf);

    for (int y = 0; y < ed->view.screen_rows; y++) {
        size_t filerow = ed->view.top_line + (size_t)y;

        if (filerow >= line_count) {
            if (ed->buf.length == 0 && y == ed->view.screen_rows / 3) {
                char welcome[80];
                int len = snprintf(welcome, sizeof(welcome), "piece editor");
                if (len > ed->view.screen_cols) len = ed->view.screen_cols;
                int pad = (ed->view.screen_cols - len) / 2;
                if (pad > 0) {
                    abAppend(ab, "~", 1);
                    pad--;
                }
                while (pad-- > 0) abAppend(ab, " ", 1);
                abAppend(ab, welcome, len);
            } else {
                abAppend(ab, "~", 1);
            }
        } else {
            size_t start = bufferLineStart(&ed->buf, filerow);
            size_t len = bufferLineLength(&ed->buf, filerow);
            viewAppendLineSlice(ab, flat + start, len, ed->view.left_col, ed->view.screen_cols);
        }

        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
    }
}

void viewDrawStatusBar(append_buffer *ab, const editor *ed) {
    char left[128];
    char right[64];

    int left_len = snprintf(left, sizeof(left), " %.20s%s | %zu lines ",
        ed->filename ? ed->filename : "[No Name]",
        ed->dirty ? " [+]" : "",
        bufferLineCount(&ed->buf));

    int right_len = snprintf(right, sizeof(right), " %zu:%zu ", ed->cur.row + 1, ed->cur.col + 1);

    if (left_len < 0) left_len = 0;
    if (right_len < 0) right_len = 0;
    if (left_len > ed->view.screen_cols) left_len = ed->view.screen_cols;

    abAppend(ab, "\x1b[7m", 4);
    abAppend(ab, left, left_len);

    while (left_len < ed->view.screen_cols) {
        if (ed->view.screen_cols - left_len == right_len) {
            abAppend(ab, right, right_len);
            break;
        }
        abAppend(ab, " ", 1);
        left_len++;
    }

    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void viewDrawMessageBar(append_buffer *ab, const editor *ed) {
    abAppend(ab, "\x1b[K", 3);
    int len = (int)strlen(ed->status);
    if (len > ed->view.screen_cols) len = ed->view.screen_cols;
    if (len > 0 && time(NULL) - ed->status_time < STATUS_MSG_TTL) {
        abAppend(ab, ed->status, len);
    }
}