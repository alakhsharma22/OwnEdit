#include "editor.h"

void abInit(append_buffer *ab) {
    ab->data = NULL;
    ab->len = 0;
    ab->cap = 0;
}

void abAppend(append_buffer *ab, const char *s, int len) {
    if (len <= 0) return;
    if (ab->len + len > ab->cap) {
        int newcap = ab->cap ? ab->cap : 128;
        while (newcap < ab->len + len) newcap *= 2;
        char *p = realloc(ab->data, newcap);
        if (p == NULL) return;
        ab->data = p;
        ab->cap = newcap;
    }
    memcpy(ab->data + ab->len, s, len);
    ab->len += len;
}

void abFree(append_buffer *ab) {
    free(ab->data);
    ab->data = NULL;
    ab->len = 0;
    ab->cap = 0;
}