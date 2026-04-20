#include "editor.h"

editor E;

void editorInit(void) {
    bufferInit(&E.buf);
    cursorInit(&E.cur);
    viewInit(&E.view);
    E.filename = NULL;
    E.dirty = 0;
    E.quit_times = 1;
    E.status[0] = '\0';
    E.status_time = 0;
    historyInit();

    if (termGetSize(&E.view.screen_rows, &E.view.screen_cols) == -1) termDie("termGetSize");
    E.view.screen_rows -= 2;
}

void editorSetStatus(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.status, sizeof(E.status), fmt, ap);
    va_end(ap);
    E.status_time = time(NULL);
}