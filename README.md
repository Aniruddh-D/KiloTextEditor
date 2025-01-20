# Kilo Text Editor

**Kilo** is a lightweight, terminal-based text editor. It is a learning project inspired by [Antirez's Kilo editor](https://github.com/antirez/kilo). This editor is minimalistic yet functional, supporting basic text editing features, navigation, and file handling. It is written in C and designed to be portable across Unix-based systems.

---

![image](https://github.com/user-attachments/assets/b0446432-1464-4d1b-ab6f-66f3179460c0)


## Features

- **File Editing:** Open, modify, and save text files.
- **Cross-Platform Compatibility:** Works on Unix-based systems and Windows.
- **Cursor Navigation:** Move the cursor across the text using arrow keys.
- **Line Editing:** Insert, delete, and modify lines and characters.
- **Find/Search:** Search for specific text in the file.
- **Status Bar:** Displays file information, cursor position, and status messages.
- **Raw Mode:** Direct terminal interaction for advanced keyboard input handling.
- **Error Handling:** Gracefully handles errors and provides meaningful messages.

---

## Requirements

### For Unix-based Systems:

- A C compiler (e.g., GCC or Clang)
- Standard Unix libraries (e.g., `termios`, `unistd`, `stdio`, `stdlib`)
- GNOME Terminal


## Installation

1. **Clone the Repository:**

   ```sh
   git clone https://github.com/Aniruddh-D/kilo-text-editor.git
   cd kilo-text-editor
   ```


2. **Compile the Code:**

   - On Unix-based systems:

     ```sh
     gcc -o kilo kilo.c -Wall -Wextra -pedantic -std=c99
     ```

3. **Run the Editor:**

   ```sh
   ./kilo <filename>
   ```

   Replace `<filename>` with the file you want to edit, or leave it blank to create a new file.

---

## Usage

### Keyboard Shortcuts

| Shortcut          | Action                                |
| ----------------- | ------------------------------------- |
| `Ctrl-S`          | Save the current file                 |
| `Ctrl-Q`          | Quit the editor                       |
| `Ctrl-F`          | Search for text in the file           |
| Arrow Keys        | Move the cursor                       |
| `Backspace`/`Del` | Delete the character under the cursor |

### Features Overview

1. **Open a File:**

   - Pass the file name as an argument while starting the editor.

2. **Edit Text:**

   - Use the keyboard to type, delete, or modify text.

3. **Save Changes:**

   - Press `Ctrl-S` to save the file. If the file does not exist, you will be prompted to enter a name.

4. **Search:**

   - Press `Ctrl-F` to search for text. Use the arrow keys to navigate through results.

5. **Quit:**

   - Press `Ctrl-Q` to quit. If there are unsaved changes, the editor will warn you.

---

## Code Structure

The codebase is modular and divided into logical sections for readability and maintainability:

- **Terminal Handling:** Manages raw mode and terminal interactions.
- **File I/O:** Handles file reading and writing.
- **Editor Operations:** Provides editing functionalities like insertion, deletion, and navigation.
- **Output Handling:** Draws the screen and updates the UI.
- **Input Handling:** Processes keyboard inputs.
- **Search Functionality:** Implements the search feature.

---

## Known Issues and Limitations

- Limited support for large files due to memory constraints.
- No syntax highlighting (can be implemented in future versions).
- Only supports basic text editing features.

---

## Contributing

Contributions are welcome! If you have suggestions or want to fix bugs, feel free to:

1. Fork the repository.
2. Create a new branch for your changes.
3. Submit a pull request with a description of your modifications.

---

## License

This project is open-source and distributed under the MIT License. See the `LICENSE` file for details.

---

## Acknowledgments

Special thanks to [Antirez](https://github.com/antirez/kilo) for the inspiration behind this project.

---

## Contact

For issues or questions, please contact the project maintainer:

- Email: [aniruddhdubey2844@gmail.com](mailto\:aniruddhdubey2844@gmail.com)
- GitHub: [Aniruddh-D](https://github.com/Aniruddh-D)

