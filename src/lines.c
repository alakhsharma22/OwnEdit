#include "editor.h"

void linesInit(line_index *lines) {
    lines->offsets = NULL;
    lines->count = 0;
    lines->cap = 0;
}

void linesFree(line_index *lines) {
    free(lines->offsets);
    lines->offsets = NULL;
    lines->count = 0;
    lines->cap = 0;
}

void linesClear(line_index *lines) {
    lines->count = 0;
}

void linesPush(line_index *lines, size_t off) {
    if (lines->count == lines->cap) {
        size_t newcap = lines->cap ? lines->cap * 2 : 64;
        size_t *p = realloc(lines->offsets, newcap * sizeof(size_t));
        if (p == NULL) return;
        lines->offsets = p;
        lines->cap = newcap;
    }
    lines->offsets[lines->count++] = off;
}