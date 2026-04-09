#include "editor.h"

int main(int argc, char **argv) {
    termEnableRawMode();
    editorInit();

    if (argc > 1) editorOpen(argv[1]);

    editorSetStatus("Ctrl-S = save | Ctrl-Q = quit");

    for (;;) {
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
