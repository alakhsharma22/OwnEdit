# OwnEdit

OwnEdit is a small terminal text editor written in C.

The project takes inspiration from the basic ideas behind Kilo as a minimal terminal editor, but uses a different internal architecture so the codebase is easier to extend and adapt over time. While the editor keeps the implementation simple, some core design choices are intentionally different to make future features and structural changes easier to add.


## Current updates:
- `editor.h`  
  added history types, `history` field in `editor`, new function declarations, and `bufferCopyRange()`

- `history.c`  
  new undo/redo implementation

- `commands.c`  
  records edits during insert/delete/backspace/newline

- `input.c`  
  adds `Ctrl-Z` and `Ctrl-Y`

- `buffer.c`  
  adds `bufferCopyRange()` for storing deleted text

- `file.c`  
  resets history on open, and keeps the old filename-first flow while handling new files properly
