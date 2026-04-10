#include <fcntl.h>
#include <sys/stat.h>
#include "editor.h"

static char *dupstr(const char *s) {
    size_t len = strlen(s);
    char *p = malloc(len + 1);
    if (p == NULL) return NULL;
    memcpy(p, s, len + 1);
    return p;
}

int fileReadAll(const char *filename, char **out, size_t *out_len) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) return -1;

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return -1;
    }

    size_t cap = st.st_size > 0 ? (size_t)st.st_size : 1;
    char *data = malloc(cap + 1);
    if (data == NULL) {
        close(fd);
        return -1;
    }

    size_t len = 0;
    for (;;) {
        if (len == cap) {
            size_t newcap = cap * 2;
            char *p = realloc(data, newcap + 1);
            if (p == NULL) {
                free(data);
                close(fd);
                return -1;
            }
            data = p;
            cap = newcap;
        }

        ssize_t n = read(fd, data + len, cap - len);
        if (n == -1) {
            if (errno == EINTR) continue;
            free(data);
            close(fd);
            return -1;
        }
        if (n == 0) break;
        len += (size_t)n;
    }

    close(fd);
    data[len] = '\0';
    *out = data;
    *out_len = len;
    return 0;
}

int fileWriteAll(const char *filename, const char *data, size_t len) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) return -1;

    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, data + off, len - off);
        if (n == -1) {
            if (errno == EINTR) continue;
            close(fd);
            return -1;
        }
        off += (size_t)n;
    }

    if (close(fd) == -1) return -1;
    return 0;
}

void editorOpen(const char *filename) {
    char *newname = dupstr(filename);
    if (newname == NULL) termDie("strdup");

    free(E.filename);
    E.filename = newname;

    if (bufferLoadFile(&E.buf, filename) == -1) {
        if (errno != ENOENT) {
            editorSetStatus("open failed: %s", strerror(errno));
            return;
        }

        bufferFree(&E.buf);
        bufferInit(&E.buf);
        editorSetStatus("new file: %s", filename);
    }

    historyFree();
    historyInit();

    E.cur.pos = 0;
    E.cur.want_col = 0;
    cursorSync(&E.cur, &E.buf);
    E.view.top_line = 0;
    E.view.left_col = 0;
    E.dirty = 0;
}

int editorSave(void) {
    if (E.filename == NULL) {
        editorSetStatus("save failed: no filename");
        return -1;
    }

    if (bufferWriteFile(&E.buf, E.filename) == -1) {
        editorSetStatus("save failed: %s", strerror(errno));
        return -1;
    }

    E.dirty = 0;
    editorSetStatus("saved %s", E.filename);
    return 0;
}

int bufferWriteFile(buffer *buf, const char *filename) {
    size_t len = 0;
    char *flat = bufferFlatten(buf, &len);
    int rc = fileWriteAll(filename, flat, len);
    free(flat);
    return rc;
}