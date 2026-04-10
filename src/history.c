#include "editor.h"

static char *memdupBytes(const char *src, size_t len) {
    char *copy = malloc(len ? len : 1);
    if (copy == NULL) termDie("malloc");
    if (len > 0) memcpy(copy, src, len);
    return copy;
}

static void freeAction(edit_action *act) {
    free(act->data);
    act->data = NULL;
    act->len = 0;
}

static void freeStack(history_node *top) {
    while (top) {
        history_node *next = top->next;
        freeAction(&top->act);
        free(top);
        top = next;
    }
}

static void clearRedo(void) {
    freeStack(E.hist.redo_top);
    E.hist.redo_top = NULL;
}

static history_node *nodeNew(edit_kind kind, size_t pos, const char *data, size_t len,
    const cursor *before, const cursor *after) {
    history_node *node = malloc(sizeof(history_node));
    if (node == NULL) termDie("malloc");

    node->act.kind = kind;
    node->act.pos = pos;
    node->act.data = memdupBytes(data, len);
    node->act.len = len;
    node->act.before = *before;
    node->act.after = *after;
    node->next = NULL;
    return node;
}

static void pushUndo(history_node *node) {
    node->next = E.hist.undo_top;
    E.hist.undo_top = node;
}

static void pushRedo(history_node *node) {
    node->next = E.hist.redo_top;
    E.hist.redo_top = node;
}

static history_node *popUndo(void) {
    history_node *node = E.hist.undo_top;
    if (node) E.hist.undo_top = node->next;
    return node;
}

static history_node *popRedo(void) {
    history_node *node = E.hist.redo_top;
    if (node) E.hist.redo_top = node->next;
    return node;
}

void historyInit(void) {
    E.hist.undo_top = NULL;
    E.hist.redo_top = NULL;
    E.hist.replaying = 0;
}

void historyFree(void) {
    freeStack(E.hist.undo_top);
    freeStack(E.hist.redo_top);
    E.hist.undo_top = NULL;
    E.hist.redo_top = NULL;
    E.hist.replaying = 0;
}

void historyRecordInsert(size_t pos, const char *data, size_t len,
    const cursor *before, const cursor *after) {
    if (E.hist.replaying) return;
    clearRedo();
    pushUndo(nodeNew(EDIT_INSERT, pos, data, len, before, after));
}

void historyRecordDelete(size_t pos, const char *data, size_t len,
    const cursor *before, const cursor *after) {
    if (E.hist.replaying) return;
    clearRedo();
    pushUndo(nodeNew(EDIT_DELETE, pos, data, len, before, after));
}

int historyUndo(void) {
    history_node *node = popUndo();
    if (node == NULL) return -1;

    E.hist.replaying = 1;

    if (node->act.kind == EDIT_INSERT) {
        bufferDeleteRange(&E.buf, node->act.pos, node->act.len);
        E.cur = node->act.before;
    } else {
        bufferInsertBytes(&E.buf, node->act.pos, node->act.data, node->act.len);
        E.cur = node->act.before;
    }

    cursorSync(&E.cur, &E.buf);
    E.hist.replaying = 0;
    pushRedo(node);
    E.dirty = 1;
    return 0;
}

int historyRedo(void) {
    history_node *node = popRedo();
    if (node == NULL) return -1;

    E.hist.replaying = 1;

    if (node->act.kind == EDIT_INSERT) {
        bufferInsertBytes(&E.buf, node->act.pos, node->act.data, node->act.len);
        E.cur = node->act.after;
    } else {
        bufferDeleteRange(&E.buf, node->act.pos, node->act.len);
        E.cur = node->act.after;
    }

    cursorSync(&E.cur, &E.buf);
    E.hist.replaying = 0;
    pushUndo(node);
    E.dirty = 1;
    return 0;
}