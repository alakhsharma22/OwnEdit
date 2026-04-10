#include "editor.h"

void editorProcessKeypress(void) {
    int c = termReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            if (E.dirty && E.quit_times > 0) {
                editorSetStatus("unsaved changes, press Ctrl-Q again to quit");
                E.quit_times--;
                return;
            }
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case CTRL_KEY('s'):
            editorSave();
            break;

        case CTRL_KEY('z'):
            if (historyUndo() == -1) editorSetStatus("nothing to undo");
            break;

        case CTRL_KEY('y'):
            if (historyRedo() == -1) editorSetStatus("nothing to redo");
            break;

        case HOME_KEY:
        case END_KEY:
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            commandsMoveCursor(c);
            break;

        case PAGE_UP:
        case PAGE_DOWN: {
            if (c == PAGE_UP) {
                if (E.view.top_line < E.cur.row) E.cur.row = E.view.top_line;
            } else {
                E.cur.row = E.view.top_line + E.view.screen_rows - 1;
                size_t count = bufferLineCount(&E.buf);
                if (E.cur.row >= count) E.cur.row = count - 1;
            }

            E.cur.pos = bufferLineStart(&E.buf, E.cur.row);
            cursorSync(&E.cur, &E.buf);
            E.cur.want_col = 0;

            for (int times = E.view.screen_rows; times > 0; times--) {
                commandsMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            break;
        }

        case '\r':
            commandsInsertNewline();
            break;

        case CTRL_KEY('h'):
        case 127:
            commandsBackspace();
            break;

        case DEL_KEY:
            commandsDelete();
            break;

        case CTRL_KEY('l'):
        case '\x1b':
            break;

        default:
            if (!iscntrl(c) && c < 128) commandsInsertChar(c);
            break;
    }

    E.quit_times = 1;
}