#include "editor.h"

void editorRefreshScreen(void) {
    cursorSync(&E.cur, &E.buf);
    viewScroll(&E.view, &E.buf, &E.cur);

    size_t flat_len = 0;
    char *flat = bufferFlatten(&E.buf, &flat_len);
    size_t line_start = bufferLineStart(&E.buf, E.cur.row);
    size_t line_len = bufferLineLength(&E.buf, E.cur.row);
    size_t rx = viewLineVisualX(flat + line_start, line_len, E.cur.col);

    append_buffer ab;
    abInit(&ab);

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    viewDrawRows(&ab, &E, flat);
    viewDrawStatusBar(&ab, &E);
    viewDrawMessageBar(&ab, &E);

    char buf[32];
    int cy = (int)(E.cur.row - E.view.top_line) + 1;
    int cx = (int)(rx - E.view.left_col) + 1;
    if (cx < 1) cx = 1;
    if (cy < 1) cy = 1;
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", cy, cx);
    abAppend(&ab, buf, (int)strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.data, ab.len);

    abFree(&ab);
    free(flat);
}