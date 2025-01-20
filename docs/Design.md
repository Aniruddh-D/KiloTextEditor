# Design of Kilo Text Editor

This document provides a detailed overview of the design and architecture of the Kilo Text Editor. It explains how the editor is structured, the responsibilities of each component, and the rationale behind key design choices. This information is helpful for developers looking to understand, maintain, or extend the project.

---

## Overview

Kilo is a lightweight text editor designed for terminal-based environments. It aims to be small and efficient while providing basic text editing functionality. The editor works in raw terminal mode, allowing it to capture keypresses directly and manage the terminal display.

### Key Features
- File I/O: Open, modify, and save text files.
- Cursor navigation and editing.
- Search functionality.
- Cross-platform compatibility (Unix and Windows).
- Status and message bars for user feedback.

---

## Architecture

Kilo is designed around modular functions, each responsible for a specific task. The architecture can be divided into the following components:

### 1. **Terminal Handling**
Manages the interaction between the editor and the terminal, including:
- Enabling and disabling raw mode for direct terminal input.
- Reading keypresses and translating them into actions.
- Adjusting terminal size dynamically to match the user's screen.

**Key Functions:**
- `enableRawMode`: Activates raw mode for terminal input.
- `disableRawMode`: Restores the terminal to its original state.
- `editorReadKey`: Reads and processes individual keypresses.
- `getWindowSize`: Determines the terminal's dimensions.

### 2. **Editor Core**
Handles the editor's internal state, cursor position, and row operations:
- Tracks the cursor's position (both in the file and on the screen).
- Maintains the file's content as an array of rows.
- Supports text insertion, deletion, and modification.

**Key Data Structures:**
- `struct erow`: Represents a line of text with metadata such as size and rendering data.
- `struct editorConfig`: Holds the global editor state, including cursor position, screen dimensions, and file data.

**Key Functions:**
- `editorInsertChar`: Inserts a character at the current cursor position.
- `editorDelChar`: Deletes the character under the cursor.
- `editorInsertRow`: Adds a new row of text.
- `editorDelRow`: Removes a row from the file.

### 3. **Rendering and Display**
Draws the file contents and interface on the screen. It is responsible for:
- Displaying the file's text within the visible window.
- Rendering the status bar and message bar.
- Scrolling to keep the cursor within the viewport.

**Key Functions:**
- `editorRefreshScreen`: Refreshes the entire screen.
- `editorDrawRows`: Renders the file contents line by line.
- `editorDrawStatusBar`: Displays file information and status messages.
- `editorScroll`: Adjusts the viewport based on the cursor's position.

### 4. **File I/O**
Manages file opening, reading, and saving:
- Reads a file into memory and converts its contents into rows.
- Writes the current editor state back to a file.

**Key Functions:**
- `editorOpen`: Loads a file into the editor.
- `editorSave`: Saves the current content to a file.
- `editorRowsToString`: Converts the in-memory rows into a single string for saving.

### 5. **Search**
Implements text searching:
- Prompts the user for a search query.
- Highlights and navigates to matches within the file.

**Key Functions:**
- `editorFind`: Searches for a string in the file and navigates to the match.
- `editorPrompt`: Collects user input for the search query.

### 6. **Input Handling**
Processes user input and maps it to editor actions:
- Supports commands like save, quit, and find.
- Handles cursor movement and text editing.

**Key Functions:**
- `editorProcessKeypress`: Maps keypresses to specific editor commands.
- `editorMoveCursor`: Updates the cursor's position based on user input.

---

## Design Choices

### 1. **Raw Mode**
- **Why:** Provides full control over keyboard input and terminal output.
- **How:** Disables canonical mode and echo using `termios` (Unix) or `SetConsoleMode` (Windows).

### 2. **Row-Based Storage**
- **Why:** Simplifies operations like insertion, deletion, and rendering.
- **How:** Each line of text is stored as an `erow` structure, which includes both the original and rendered versions of the line.

### 3. **Cross-Platform Compatibility**
- **Why:** Ensures usability across different operating systems.
- **How:** Uses conditional compilation (`#ifdef _WIN32`) to separate Unix and Windows implementations.

---

## Limitations

- **Large Files:** Performance may degrade with very large files due to in-memory storage.
- **Lack of Syntax Highlighting:** This version focuses on plain text editing.
- **Limited Undo/Redo:** Does not support undo/redo operations.

---

## Future Enhancements

- Add syntax highlighting for programming languages.
- Implement undo/redo functionality.
- Optimize performance for large files.
- Add mouse support for navigation.

---

This design provides a clear and maintainable structure for the Kilo Text Editor. Developers can extend it by following the modular approach described above.


