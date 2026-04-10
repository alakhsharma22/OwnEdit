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

static void applyInsert(size_t pos, const char *data, size_t len) {
    cursor before = E.cur;

    bufferInsertBytes(&E.buf, pos, data, len);
    E.cur.pos = pos + len;
    cursorSync(&E.cur, &E.buf);
    E.cur.want_col = currentVisualX();

    historyRecordInsert(pos, data, len, &before, &E.cur);
    E.dirty = 1;
}

static void applyDelete(size_t pos, size_t len, size_t new_pos) {
    cursor before = E.cur;
    char *deleted = bufferCopyRange(&E.buf, pos, len);

    bufferDeleteRange(&E.buf, pos, len);
    E.cur.pos = new_pos;
    cursorSync(&E.cur, &E.buf);
    E.cur.want_col = currentVisualX();

    historyRecordDelete(pos, deleted, len, &before, &E.cur);
    free(deleted);
    E.dirty = 1;
}

void commandsInsertChar(int c) {
    char ch = (char)c;
    applyInsert(E.cur.pos, &ch, 1);
}

void commandsInsertNewline(void) {
    char nl = '\n';
    applyInsert(E.cur.pos, &nl, 1);
}

void commandsBackspace(void) {
    if (E.cur.pos == 0) return;
    applyDelete(E.cur.pos - 1, 1, E.cur.pos - 1);
}

void commandsDelete(void) {
    if (E.cur.pos >= E.buf.length) return;
    applyDelete(E.cur.pos, 1, E.cur.pos);
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