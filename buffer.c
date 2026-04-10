#include "editor.h"

static void storageEnsure(storage *st, size_t need) {
    if (need <= st->cap) return;
    size_t newcap = st->cap ? st->cap : 1024;
    while (newcap < need) newcap *= 2;
    char *p = realloc(st->data, newcap);
    if (p == NULL) termDie("realloc");
    st->data = p;
    st->cap = newcap;
}

static piece *pieceNew(int source, size_t start, size_t length) {
    piece *p = malloc(sizeof(piece));
    if (p == NULL) termDie("malloc");
    p->source = source;
    p->start = start;
    p->length = length;
    p->prev = NULL;
    p->next = NULL;
    return p;
}

static void pieceFreeAll(piece *head) {
    while (head) {
        piece *next = head->next;
        free(head);
        head = next;
    }
}

static void bufferInsertBefore(buffer *buf, piece *at, piece *node) {
    node->next = at;
    node->prev = at ? at->prev : buf->tail;

    if (node->prev) node->prev->next = node;
    else buf->head = node;

    if (at) at->prev = node;
    else buf->tail = node;
}

static void bufferInsertAfter(buffer *buf, piece *at, piece *node) {
    node->prev = at;
    node->next = at ? at->next : NULL;

    if (node->next) node->next->prev = node;
    else buf->tail = node;

    if (at) at->next = node;
    else buf->head = node;
}

static const char *pieceData(const buffer *buf, const piece *p) {
    return p->source == PIECE_ORIGINAL ? buf->original.data : buf->add.data;
}

static piece *bufferFindPiece(buffer *buf, size_t pos, size_t *inner) {
    size_t off = 0;
    piece *p = buf->head;

    while (p) {
        if (pos <= off + p->length) {
            *inner = pos - off;
            return p;
        }
        off += p->length;
        p = p->next;
    }

    *inner = 0;
    return NULL;
}

void bufferInit(buffer *buf) {
    buf->original.data = NULL;
    buf->original.len = 0;
    buf->original.cap = 0;
    buf->add.data = NULL;
    buf->add.len = 0;
    buf->add.cap = 0;
    buf->head = NULL;
    buf->tail = NULL;
    buf->length = 0;
    linesInit(&buf->lines);
    bufferRebuildLines(buf);
}

void bufferFree(buffer *buf) {
    free(buf->original.data);
    free(buf->add.data);
    pieceFreeAll(buf->head);
    linesFree(&buf->lines);
    buf->original.data = NULL;
    buf->add.data = NULL;
    buf->head = NULL;
    buf->tail = NULL;
    buf->length = 0;
}

int bufferLoadFile(buffer *buf, const char *filename) {
    char *data = NULL;
    size_t len = 0;

    if (fileReadAll(filename, &data, &len) == -1) return -1;

    bufferFree(buf);
    bufferInit(buf);

    buf->original.data = data;
    buf->original.len = len;
    buf->original.cap = len;
    buf->length = len;

    if (len > 0) {
        piece *p = pieceNew(PIECE_ORIGINAL, 0, len);
        buf->head = p;
        buf->tail = p;
    }

    bufferRebuildLines(buf);
    return 0;
}

void bufferInsertBytes(buffer *buf, size_t pos, const char *s, size_t len) {
    if (len == 0) return;
    if (pos > buf->length) pos = buf->length;

    storageEnsure(&buf->add, buf->add.len + len);
    memcpy(buf->add.data + buf->add.len, s, len);
    size_t add_start = buf->add.len;
    buf->add.len += len;

    piece *node = pieceNew(PIECE_ADD, add_start, len);

    if (buf->head == NULL) {
        buf->head = node;
        buf->tail = node;
    } else if (pos == buf->length) {
        bufferInsertAfter(buf, buf->tail, node);
    } else {
        size_t inner = 0;
        piece *p = bufferFindPiece(buf, pos, &inner);

        if (p == NULL) {
            bufferInsertAfter(buf, buf->tail, node);
        } else if (inner == 0) {
            bufferInsertBefore(buf, p, node);
        } else if (inner == p->length) {
            bufferInsertAfter(buf, p, node);
        } else {
            piece *right = pieceNew(p->source, p->start + inner, p->length - inner);
            p->length = inner;
            bufferInsertAfter(buf, p, node);
            bufferInsertAfter(buf, node, right);
        }
    }

    buf->length += len;
    bufferRebuildLines(buf);
}

void bufferInsertChar(buffer *buf, size_t pos, char c) {
    bufferInsertBytes(buf, pos, &c, 1);
}

void bufferDeleteRange(buffer *buf, size_t pos, size_t len) {
    if (len == 0 || pos >= buf->length) return;
    if (pos + len > buf->length) len = buf->length - pos;

    size_t del_start = pos;
    size_t del_end = pos + len;
    size_t off = 0;
    piece *old = buf->head;
    piece *new_head = NULL;
    piece *new_tail = NULL;

    while (old) {
        size_t piece_start = off;
        size_t piece_end = off + old->length;

        if (piece_end <= del_start || piece_start >= del_end) {
            piece *copy = pieceNew(old->source, old->start, old->length);
            copy->prev = new_tail;
            if (new_tail) new_tail->next = copy;
            else new_head = copy;
            new_tail = copy;
        } else {
            if (piece_start < del_start) {
                size_t left_len = del_start - piece_start;
                piece *left = pieceNew(old->source, old->start, left_len);
                left->prev = new_tail;
                if (new_tail) new_tail->next = left;
                else new_head = left;
                new_tail = left;
            }
            if (piece_end > del_end) {
                size_t right_skip = del_end - piece_start;
                size_t right_len = piece_end - del_end;
                piece *right = pieceNew(old->source, old->start + right_skip, right_len);
                right->prev = new_tail;
                if (new_tail) new_tail->next = right;
                else new_head = right;
                new_tail = right;
            }
        }

        off = piece_end;
        old = old->next;
    }

    if (new_tail) new_tail->next = NULL;

    pieceFreeAll(buf->head);
    buf->head = new_head;
    buf->tail = new_tail;
    buf->length -= len;
    bufferRebuildLines(buf);
}

char *bufferFlatten(const buffer *buf, size_t *out_len) {
    char *flat = malloc(buf->length + 1);
    if (flat == NULL) termDie("malloc");

    size_t off = 0;
    piece *p = buf->head;
    while (p) {
        const char *src = pieceData(buf, p);
        memcpy(flat + off, src + p->start, p->length);
        off += p->length;
        p = p->next;
    }

    flat[off] = '\0';
    if (out_len) *out_len = off;
    return flat;
}

char bufferCharAt(const buffer *buf, size_t pos) {
    if (pos >= buf->length) return '\0';

    size_t off = 0;
    piece *p = buf->head;
    while (p) {
        if (pos < off + p->length) {
            const char *src = pieceData(buf, p);
            return src[p->start + (pos - off)];
        }
        off += p->length;
        p = p->next;
    }

    return '\0';
}

char *bufferCopyRange(const buffer *buf, size_t pos, size_t len) {
    if (pos > buf->length) pos = buf->length;
    if (pos + len > buf->length) len = buf->length - pos;

    char *copy = malloc(len ? len : 1);
    if (copy == NULL) termDie("malloc");
    if (len == 0) return copy;

    size_t off = 0;
    piece *p = buf->head;
    size_t cur = 0;

    while (p && off < len) {
        size_t piece_start = cur;
        size_t piece_end = cur + p->length;

        if (piece_end > pos && piece_start < pos + len) {
            size_t take_start = pos > piece_start ? pos - piece_start : 0;
            size_t take_end = (pos + len) < piece_end ? (pos + len) - piece_start : p->length;
            size_t take_len = take_end - take_start;
            const char *src = pieceData(buf, p);
            memcpy(copy + off, src + p->start + take_start, take_len);
            off += take_len;
        }

        cur = piece_end;
        p = p->next;
    }

    return copy;
}

void bufferRebuildLines(buffer *buf) {
    linesClear(&buf->lines);
    linesPush(&buf->lines, 0);

    size_t len = 0;
    char *flat = bufferFlatten(buf, &len);
    for (size_t i = 0; i < len; i++) {
        if (flat[i] == '\n') linesPush(&buf->lines, i + 1);
    }
    free(flat);
}

size_t bufferLineCount(const buffer *buf) {
    return buf->lines.count ? buf->lines.count : 1;
}

size_t bufferLineStart(const buffer *buf, size_t row) {
    if (row >= bufferLineCount(buf)) row = bufferLineCount(buf) - 1;
    return buf->lines.offsets[row];
}

size_t bufferLineLength(const buffer *buf, size_t row) {
    size_t count = bufferLineCount(buf);
    if (row >= count) return 0;

    size_t start = buf->lines.offsets[row];
    if (row + 1 < count) {
        size_t next = buf->lines.offsets[row + 1];
        if (next > start) return next - start - 1;
    }
    return buf->length - start;
}

size_t bufferLineFromPos(const buffer *buf, size_t pos) {
    size_t count = bufferLineCount(buf);
    if (count == 0) return 0;
    if (pos > buf->length) pos = buf->length;

    size_t lo = 0;
    size_t hi = count;
    while (lo + 1 < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (buf->lines.offsets[mid] <= pos) lo = mid;
        else hi = mid;
    }
    return lo;
}