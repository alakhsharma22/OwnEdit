#ifndef EDITOR_H
#define EDITOR_H

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define TAB_STOP 4
#define STATUS_MSG_TTL 5

#define PIECE_ORIGINAL 0
#define PIECE_ADD 1

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

typedef struct piece {
    int source;
    size_t start;
    size_t length;
    struct piece *prev;
    struct piece *next;
} piece;

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} storage;

typedef struct {
    size_t *offsets;
    size_t count;
    size_t cap;
} line_index;

typedef struct {
    storage original;
    storage add;
    piece *head;
    piece *tail;
    size_t length;
    line_index lines;
} buffer;

typedef struct {
    size_t pos;
    size_t row;
    size_t col;
    size_t want_col;
} cursor;

typedef struct {
    int screen_rows;
    int screen_cols;
    size_t top_line;
    size_t left_col;
} view;

typedef struct {
    char *data;
    int len;
    int cap;
} append_buffer;

typedef enum {
    EDIT_INSERT,
    EDIT_DELETE
} edit_kind;

typedef struct {
    edit_kind kind;
    size_t pos;
    char *data;
    size_t len;
    cursor before;
    cursor after;
} edit_action;

typedef struct history_node {
    edit_action act;
    struct history_node *next;
} history_node;

typedef struct {
    history_node *undo_top;
    history_node *redo_top;
    int replaying;
} history;

typedef struct {
    struct termios orig_termios;
    int rawmode;
    buffer buf;
    cursor cur;
    view view;
    char *filename;
    int dirty;
    int quit_times;
    char status[160];
    time_t status_time;
    history hist;
} editor;

extern editor E;

void editorInit(void);
void editorSetStatus(const char *fmt, ...);
void editorRefreshScreen(void);
void editorProcessKeypress(void);
void editorOpen(const char *filename);
int editorSave(void);

void termEnableRawMode(void);
void termDisableRawMode(void);
void termDie(const char *s);
int termReadKey(void);
int termGetSize(int *rows, int *cols);

void abInit(append_buffer *ab);
void abAppend(append_buffer *ab, const char *s, int len);
void abFree(append_buffer *ab);

void bufferInit(buffer *buf);
void bufferFree(buffer *buf);
int bufferLoadFile(buffer *buf, const char *filename);
int bufferWriteFile(buffer *buf, const char *filename);
void bufferInsertChar(buffer *buf, size_t pos, char c);
void bufferInsertBytes(buffer *buf, size_t pos, const char *s, size_t len);
void bufferDeleteRange(buffer *buf, size_t pos, size_t len);
char *bufferFlatten(const buffer *buf, size_t *out_len);
char bufferCharAt(const buffer *buf, size_t pos);
char *bufferCopyRange(const buffer *buf, size_t pos, size_t len);
void bufferRebuildLines(buffer *buf);
size_t bufferLineCount(const buffer *buf);
size_t bufferLineStart(const buffer *buf, size_t row);
size_t bufferLineLength(const buffer *buf, size_t row);
size_t bufferLineFromPos(const buffer *buf, size_t pos);

void linesInit(line_index *lines);
void linesFree(line_index *lines);
void linesClear(line_index *lines);
void linesPush(line_index *lines, size_t off);

void cursorInit(cursor *cur);
void cursorSync(cursor *cur, const buffer *buf);
void cursorMoveLeft(cursor *cur, const buffer *buf);
void cursorMoveRight(cursor *cur, const buffer *buf);
void cursorMoveUp(cursor *cur, const buffer *buf);
void cursorMoveDown(cursor *cur, const buffer *buf);
void cursorMoveHome(cursor *cur, const buffer *buf);
void cursorMoveEnd(cursor *cur, const buffer *buf);

void viewInit(view *view);
void viewScroll(view *view, const buffer *buf, const cursor *cur);
void viewDrawRows(append_buffer *ab, const editor *ed, const char *flat);
void viewDrawStatusBar(append_buffer *ab, const editor *ed);
void viewDrawMessageBar(append_buffer *ab, const editor *ed);
size_t viewLineVisualX(const char *s, size_t len, size_t col);
size_t viewLineBufferCol(const char *s, size_t len, size_t goal_vx);

int fileReadAll(const char *filename, char **out, size_t *out_len);
int fileWriteAll(const char *filename, const char *data, size_t len);

void commandsInsertChar(int c);
void commandsInsertNewline(void);
void commandsBackspace(void);
void commandsDelete(void);
void commandsMoveCursor(int key);

void historyInit(void);
void historyFree(void);
void historyRecordInsert(size_t pos, const char *data, size_t len, const cursor *before, const cursor *after);
void historyRecordDelete(size_t pos, const char *data, size_t len, const cursor *before, const cursor *after);
int historyUndo(void);
int historyRedo(void);

#endif