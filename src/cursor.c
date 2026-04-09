#include "editor.h"

static size_t cursorVisualX(const buffer *buf, const cursor *cur) {
    size_t start = bufferLineStart(buf, cur->row);
    size_t len = bufferLineLength(buf, cur->row);
    size_t flat_len = 0;
    char *flat = bufferFlatten(buf, &flat_len);
    size_t vx = viewLineVisualX(flat + start, len, cur->col);
    free(flat);
    return vx;
}

static void cursorSetRowVisual(cursor *cur, const buffer *buf, size_t row, size_t goal_vx) {
    size_t start = bufferLineStart(buf, row);
    size_t len = bufferLineLength(buf, row);
    size_t flat_len = 0;
    char *flat = bufferFlatten(buf, &flat_len);
    size_t col = viewLineBufferCol(flat + start, len, goal_vx);
    free(flat);

    cur->pos = start + col;
    cur->row = row;
    cur->col = col;
    cur->want_col = goal_vx;
}

void cursorInit(cursor *cur) {
    cur->pos = 0;
    cur->row = 0;
    cur->col = 0;
    cur->want_col = 0;
}

void cursorSync(cursor *cur, const buffer *buf) {
    if (cur->pos > buf->length) cur->pos = buf->length;
    cur->row = bufferLineFromPos(buf, cur->pos);
    cur->col = cur->pos - bufferLineStart(buf, cur->row);
}

void cursorMoveLeft(cursor *cur, const buffer *buf) {
    if (cur->pos > 0) cur->pos--;
    cursorSync(cur, buf);
    cur->want_col = cursorVisualX(buf, cur);
}

void cursorMoveRight(cursor *cur, const buffer *buf) {
    if (cur->pos < buf->length) cur->pos++;
    cursorSync(cur, buf);
    cur->want_col = cursorVisualX(buf, cur);
}

void cursorMoveUp(cursor *cur, const buffer *buf) {
    cursorSync(cur, buf);
    if (cur->row == 0) return;
    cursorSetRowVisual(cur, buf, cur->row - 1, cur->want_col);
}

void cursorMoveDown(cursor *cur, const buffer *buf) {
    cursorSync(cur, buf);
    size_t count = bufferLineCount(buf);
    if (cur->row + 1 >= count) return;
    cursorSetRowVisual(cur, buf, cur->row + 1, cur->want_col);
}

void cursorMoveHome(cursor *cur, const buffer *buf) {
    cursorSync(cur, buf);
    cur->pos = bufferLineStart(buf, cur->row);
    cursorSync(cur, buf);
    cur->want_col = 0;
}

void cursorMoveEnd(cursor *cur, const buffer *buf) {
    cursorSync(cur, buf);
    cur->pos = bufferLineStart(buf, cur->row) + bufferLineLength(buf, cur->row);
    cursorSync(cur, buf);
    cur->want_col = cursorVisualX(buf, cur);
}