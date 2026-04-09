#include "editor.h"

static size_t currentVisualX(void) {
    size_t start = bufferLineStart(&E.buf, E.cur.row);
    size_t len = bufferLineLength(&E.buf, E.cur.row);
    size_t flat_len = 0;
    char *flat = bufferFlatten(&E.buf, &flat_len);
    size_t vx = viewLineVisualX(flat + start, len, E.cur.col);
    free(flat);
    return vx;
}

void commandsInsertChar(int c) {
    bufferInsertChar(&E.buf, E.cur.pos, (char)c);
    E.cur.pos++;
    cursorSync(&E.cur, &E.buf);
    E.cur.want_col = currentVisualX();
    E.dirty = 1;
}

void commandsInsertNewline(void) {
    bufferInsertChar(&E.buf, E.cur.pos, '\n');
    E.cur.pos++;
    cursorSync(&E.cur, &E.buf);
    E.cur.want_col = 0;
    E.dirty = 1;
}

void commandsBackspace(void) {
    if (E.cur.pos == 0) return;
    bufferDeleteRange(&E.buf, E.cur.pos - 1, 1);
    E.cur.pos--;
    cursorSync(&E.cur, &E.buf);
    E.cur.want_col = currentVisualX();
    E.dirty = 1;
}

void commandsDelete(void) {
    if (E.cur.pos >= E.buf.length) return;
    bufferDeleteRange(&E.buf, E.cur.pos, 1);
    cursorSync(&E.cur, &E.buf);
    E.cur.want_col = currentVisualX();
    E.dirty = 1;
}

void commandsMoveCursor(int key) {
    switch (key) {
        case ARROW_LEFT:
            cursorMoveLeft(&E.cur, &E.buf);
            break;
        case ARROW_RIGHT:
            cursorMoveRight(&E.cur, &E.buf);
            break;
        case ARROW_UP:
            cursorMoveUp(&E.cur, &E.buf);
            break;
        case ARROW_DOWN:
            cursorMoveDown(&E.cur, &E.buf);
            break;
        case HOME_KEY:
            cursorMoveHome(&E.cur, &E.buf);
            break;
        case END_KEY:
            cursorMoveEnd(&E.cur, &E.buf);
            break;
    }
}