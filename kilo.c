/*-------------------------------------------------------------------------------------------------------------------------------*/
/******************************************************** Includes ***************************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/

#define _DEFAULT_SOURCE        //for getline() function
#define _BSD_SOURCE 
#define _GNU_SOURCE 

#include <ctype.h>            //for iscntrl()
#include <errno.h>
#include <fcntl.h>           //for open()
#include <stdio.h>
#include <stdarg.h>         //for va_list, va_start, va_end
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h> //for time_t
#include <unistd.h>

/*-------------------------------------------------------------------------------------------------------------------------------*/
/******************************************************** Defines ****************************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/

#define KILO_VERSION "0.0.1 By Aniruddh-D"    //version number
#define KILO_TAB_STOP 8        //tab stop is every 8 columns
#define CTRL_KEY(k) ((k) & 0x1f) //bitwise AND operation with 00011111, which strips the 5th and 6th bits of any character and turns it into a control character.
#define KILO_QUIT_TIMES 3       //number of times to press Ctrl-Q before quitting.

#define ABUF_INIT {NULL, 0}    //initialize the abuf struct

enum editorKey 
{
  BACKSPACE = 127,
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

/*-------------------------------------------------------------------------------------------------------------------------------*/
/******************************************************** Data *******************************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/
typedef struct erow             //holds a line of text as a pointer to a character array, a length, and a render field
{
  int size; //size of the chars field
  int rsize; //size of the render field
  char *chars; //a pointer to a character array that holds the actual text of the row
  char *render; //a pointer to a character array that holds the rendered version of the row
} erow;

struct abuf  //append buffer struct for 'Append Buffer' Section
{
  char *b;
  int len;
};


struct editorConfig             //holds all the editor state
{
  int cx, cy; //the position of the cursor on the screen
  int rx;     //the position of the cursor in the render field
  int rowoff; //the row that the user is currently scrolled to
  int coloff; //the column that the user is currently scrolled to
  int screenrows; //the number of rows in the screen
  int screencols; //the number of columns in the screen
  int numrows; //the number of rows in the file
  erow *row; //a pointer to an array of erow structs that holds the rows of the file
  int dirty; //a flag to indicate whether the file has been modified
  char *filename; //the name of the file being edited
  char statusmsg[80]; //a status message to display in the status bar
  time_t statusmsg_time; //the time at which the status message was set
  struct termios orig_termios; //the original terminal attributes
};

struct editorConfig E;          //A global variable to hold the editor state

/*-------------------------------------------------------------------------------------------------------------------------------*/
/******************************************************** Prototypes *************************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/

void editorSetStatusMessage(const char *fmt, ...); 
void editorRefreshScreen(); 
char *editorPrompt(char *prompt);

/*-------------------------------------------------------------------------------------------------------------------------------*/
/******************************************************** Terminal ***************************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/

void die(const char *s) 
{
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

void disableRawMode() 
{
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() 
{
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);

  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }

  if (c == '\x1b') {
    char seq[3];

    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

    if (seq[0] == '[') 
    {
      if (seq[1] >= '0' && seq[1] <= '9') 
      {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') 
        {
          switch (seq[1]) 
          {
            case '1': return HOME_KEY;
            case '3': return DEL_KEY;
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
          }
        }
      } 
      else 
      {
        switch (seq[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
        }
      }
    } 
    else if (seq[0] == 'O') 
    {
      switch (seq[1]) 
      {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
      }
    }

    return '\x1b';
  } 
  else 
  {
    return c;
  }
}

int getCursorPosition(int *rows, int *cols) 
{
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  while (i < sizeof(buf) - 1) 
  {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

  return 0;
}

int getWindowSize(int *rows, int *cols)
{
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) //Terminal IOCtl (stands for Input/Output Control) Get WINdow SiZe.)
  {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1; //
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*-------------------------------------------------------------------------------------------------------------------------------*/
/******************************************************** Row Operations *********************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/

int editorRowCxToRx(erow *row, int cx)    //converts the index of a character in a row to the index of the character in the render field
{
  int rx = 0; //render index
  int j;  //character index
  for (j = 0; j < cx; j++) //
  {
    if (row->chars[j] == '\t') //if the character is a tab, increment rx to the next tab stop
      rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP); 
    rx++; 
  }
  return rx; 
}

int editorRowRxtoCx(erow *row, int rx)
{
  /**To convert an rx into a cx, we do pretty much the same thing when converting the other way: 
  *loop through the chars string, calculating the current rx value (cur_rx) as we go. 
  *But instead of stopping when we hit a particular cx value and returning cur_rx, 
  *we want to stop when cur_rx hits the given rx value and return cx.
  */
  int cur_rx = 0; //current render index
  int cx; //current character index
  for(cx = 0; cx < row->size; cx++) //loop through the chars of the row
  {
    if(row->chars[cx] == '\t')    //if the character is a tab, increment cur_rx to the next tab stop
    {
      cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP); 
    }
    cur_rx++; 
    if(cur_rx > rx) return cx;
  }
  return cx; //return the character index
}

void editorUpdateRow(erow *row)           //converts each tab character in a row to spaces, so that when we draw the row to the screen, the tabs will be displayed correctly.
{
  int tabs = 0;
  int j;
  for (j = 0; j < row->size; j++) //loop through the chars of the row again, and this time copy the chars to render, replacing tabs with spaces
    if (row->chars[j] == '\t') tabs++;

  free(row->render);
  row->render = malloc(row->size + tabs*(KILO_TAB_STOP - 1) + 1);

  int idx = 0;
  for (j = 0; j < row->size; j++) //loop through the chars of the row again, and this time copy the chars to render, replacing tabs with spaces
  {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while (idx % KILO_TAB_STOP != 0) row->render[idx++] = ' ';
    } else {
      row->render[idx++] = row->chars[j];
    }
  }
  row->render[idx] = '\0';
  row->rsize = idx;
}

void editorInsertRow(int at, char *s, size_t len) //insert a row at a given index
{
  if(at < 0 || at > E.numrows) return;

  E.row = realloc(E.row, sizeof(erow)*(E.numrows + 1)); 
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));

  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';

  E.row[at].rsize = 0;
  E.row[at].render = NULL;
  editorUpdateRow(&E.row[at]);

  E.numrows++; //increment the number of rows in the editor
  E.dirty++; //increment the dirty flag, to indicate that the file has been modified
}

void editorFreeRow(erow *row) //free the memory used by a row
{
  free(row->render);
  free(row->chars);
}

void editorDelRow(int at) //delete a row at a given index
{
  if(at < 0 || at >= E.numrows) return; 
  editorFreeRow(&E.row[at]);
  memmove(&E.row[at], &E.row[at +1], sizeof(erow)*(E.numrows - at - 1));
  E.numrows--;
  E.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c) //insert a character into a row at a given index 
{
  if(at < 0 || at >row->size) at = row->size; //if at is negative, set it to the end of the row. If it is greater than the size of the row, set it to the end of the row.
  row->chars = realloc(row->chars, row->size + 2); //allocate memory for one more character
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1); //shift the characters after the insertion point to the right by one
  //memmove() comes from <string.h>. It is like memcpy(), but is safe to use when the source and destination arrays overlap.
  row->size++;
  row->chars[at] = c; //insert the character at the insertion point
  editorUpdateRow(row); //update the render field of the row to reflect the change
}

/*-------------------------------------------------------------------------------------------------------------------------------*/
/******************************************************** Editor Operations ******************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/
void editorInsertChar(int c) //insert a character at the cursor position
{
  if(E.cy == E.numrows) //if the cursor is on the last row, append a new row to the end of the file
  {
    editorInsertRow(E.numrows,"", 0); //append a new row to the end of the file
  }
  editorRowInsertChar(&E.row[E.cy], E.cx, c); //insert the character at the cursor position
  E.cx++; //move the cursor to the right
  E.dirty++; //increment the dirty flag, to indicate that the file has been modified
}

void editorRowAppendString(erow *row, char *s, size_t len) //append a string to the end of a row
{
  row->chars = realloc(row->chars, row->size + len + 1); 
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  E.dirty++;
}

/*
*editorInsertChar() doesn’t have to worry about the details of modifying an erow, and editorRowInsertChar() doesn’t have to worry about where the cursor is. 
*That is the difference between functions in the  editor operations  section and functions in the row operations section.
*/
void editorRowDelChar(erow *row, int at) //delete a character from a row
{
  if(at < 0 || at >= row->size) return; //If the cursor’s past the end of the file, then there is nothing to delete, and we return immediately
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at); //shift the characters after the deletion point to the left by one
  row->size--; //decrement the size of the row
  editorUpdateRow(row); //update the render field of the row to reflect the change
  
  E.dirty++; //increment the dirty flag, to indicate that the file has been modified
}

void editorInsertNewline() //insert a newline at the cursor position
{
  if(E.cx == 0)
  {
    editorInsertRow(E.cy, "", 0);
  }
  else
  {
    erow *row = &E.row[E.cy];
    editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
  }
  E.cy++;
  E.cx = 0;
}

void editorDelChar() //delete a character at the cursor position
{
  if(E.cy == E.numrows) return; //if the cursor is past the end of the file, then there is nothing to delete, and we return immediately
  
  if(E.cx == 0 && E.cy == 0) return; // If the cursor is at the beginning of the file (top-left corner), return immediately.

  erow *row = &E.row[E.cy]; //get the row that the cursor is on
  
  if(E.cx > 0)
  {
    editorRowDelChar(row, E.cx - 1); 
    E.cx--; //move the cursor to the left
  }
  else
  {
    E.cx = E.row[E.cy - 1].size;
    editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
    editorDelRow(E.cy);
    E.cy--;    
  } 
}

/*-------------------------------------------------------------------------------------------------------------------------------*/
/******************************************************** File I/O ***************************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/

char *editorRowsToString(int *buflen)      //convert the rows of the file into a single string.
{
  int totlen = 0;
  int j;
  for(j = 0; j < E.numrows; j++) //loop through the rows of the file and add up their sizes
    totlen += E.row[j].size + 1;
  *buflen = totlen; //set buflen to the total size of the file
  char *buf = malloc(totlen); //allocate a buffer of that size
  char *p = buf;
  for(j = 0; j < E.numrows; j++) //loop through the rows of the file again, and copy their contents into the buffer
  {
    memcpy(p, E.row[j].chars, E.row[j].size);
    p += E.row[j].size;
    *p = '\n';
    p++;
  }
  return buf;
}

void editorOpen(char *filename) //open a file and load its contents into the editor
{
  free(E.filename);
  E.filename = strdup(filename);

  FILE *fp = fopen(filename, "r");
  if (!fp) die("fopen");

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1)
  {
    while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
      linelen--;
    
    editorInsertRow(E.numrows, line, linelen);
  }
  free(line);
  fclose(fp);
  E.dirty = 0; //reset the dirty flag
}

void editorSave() //save the contents of the editor to disk
{
  if(E.filename == NULL)
  {
    E.filename = editorPrompt("Save as: %s (ESC to cancel) ");
    if(E.filename == NULL)
    {
      editorSetStatusMessage("Save aborted");
      return;
    }
  }

  int len;
  char *buf = editorRowsToString(&len); //convert the rows of the file into a single string

  int fd = open(E.filename, O_RDWR | O_CREAT, 0644); //open the file for writing -> 0644 is the permission mode -> O_RDWR is read/write mode -> O_CREAT is create the file if it doesn't exist.
  if(fd != -1)
  {
    if(ftruncate(fd, len) != -1)
    {
      if(write(fd, buf, len) == len)
      {
        close(fd);
        free(buf);
        E.dirty = 0; //reset the dirty flag
        editorSetStatusMessage("%d bytes written to disk", len);
        return;
      }
    }
    close(fd);
  }
  free(buf); //free the buffer 
  editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno)); //set the status message to an error message

}


/*-------------------------------------------------------------------------------------------------------------------------------*/
/******************************************************** Find *******************************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/

void editorFind() //search for a query in the file
{
  char *query = editorPrompt("Search: %s (ESC to Cancel) ");
  
  if(query == NULL) return;

  int i;
  for(i = 0; i <E.numrows; i++)//loop through the rows of the file
  {
    erow *row = &E.row[i]; //get the row that the cursor is on
    char *match = strstr(row->render, query); //strstr() is a standard library function that finds the first occurrence of one string in another string.
    
    if(match) //if the query is found in the row
    {
      E.cy = i; //set the cursor to the row that contains the match
      E.cx = editorRowRxtoCx(row, match - row->render); //set the cursor to the column that contains the match
      E.rowoff = E.numrows; //scroll the screen to the bottom of the file
      break;
    }
  }
  free(query);
}
//
/*-------------------------------------------------------------------------------------------------------------------------------*/
/******************************************************** Append Buffer **********************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/
//better to do one big write(), to make sure the whole screen updates at once.
// We want to replace all our write() calls with code that appends the string to a buffer, and then write() this buffer out at the end.

void abAppend(struct abuf *ab, const char *s, int len) //append a string to an abuf struct 
{
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) //free the memory used by an abuf struct
{
  free(ab->b);
}

/*-------------------------------------------------------------------------------------------------------------------------------*/
/******************************************************** Output *****************************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/

void editorScroll() //scroll the screen to keep the cursor visible on the screen
{
  E.rx = 0;
  if (E.cy < E.numrows) //if the cursor is above the last row, set rx to the render index of the cursor
  {
    E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
  }

  if (E.cy < E.rowoff) //checks if the cursor is above the visible window, and if so, scrolls up to where the cursor is
  {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) //checks if the cursor is past the bottom of the visible window
  {
    E.rowoff = E.cy - E.screenrows + 1;
  }
  if (E.rx < E.coloff) //checks if the cursor is to the left of the visible window
  {
    E.coloff = E.rx;
  }
  if (E.rx >= E.coloff + E.screencols) //checks if the cursor is to the right of the visible window
  {
    E.coloff = E.rx - E.screencols + 1;
  }
}

void editorDrawRows(struct abuf *ab) //draw the rows of the file to the screen 
{
  int y;
  for (y = 0; y < E.screenrows; y++) {
    int filerow = y + E.rowoff;
    if (filerow >= E.numrows) 
    {
      if (E.numrows == 0 && y == E.screenrows / 3) 
      {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
          "Kilo editor -- version %s", KILO_VERSION);
        if (welcomelen > E.screencols) welcomelen = E.screencols;
          int padding = (E.screencols - welcomelen) / 2;
        if (padding) 
        {
          abAppend(ab, "~", 1);
          padding--;
        }
        while (padding--) abAppend(ab, " ", 1);
        abAppend(ab, welcome, welcomelen);
      } 
      else 
      {
        abAppend(ab, "~", 1);
      }
    } 
    else 
    {
      int len = E.row[filerow].rsize - E.coloff;
      if (len < 0) len = 0;
      if (len > E.screencols) len = E.screencols;
      abAppend(ab, &E.row[filerow].render[E.coloff], len);
    }
    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
  }
}

void editorDrawStatusBar(struct abuf *ab) //draw the status bar at the bottom of the screen 
{
  abAppend(ab, "\x1b[7m", 4);
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s", E.filename ? E.filename : "[No File Name]", E.numrows, E.dirty ? "(modified)" : ""); //print the filename and the number of lines in the status bar
  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", E.cy + 1, E.numrows); //print the current line number and the total number of lines in the status bar
  if (len > E.screencols) len = E.screencols; //truncate the status bar message if it is too long to fit on the screen
  abAppend(ab, status, len);
  while (len < E.screencols) {
    if (E.screencols - len == rlen) 
    {
      abAppend(ab, rstatus, rlen);
      break;
    } 
    else 
    {
      abAppend(ab, " ", 1);
      len++;
    }
  }
  abAppend(ab, "\x1b[m", 3); //turn off the inverted colors
  abAppend(ab, "\r\n", 2); //move the cursor to the next line
}

void editorDrawMessageBar(struct abuf *ab) //draw the message bar at the bottom of the screen 
{
  abAppend(ab, "\x1b[K", 3);
  int msglen = strlen(E.statusmsg);
  if (msglen > E.screencols) msglen = E.screencols; //  truncate the message if it is too long to fit on the screen
  if (msglen && time(NULL) - E.statusmsg_time < 5) //display the message if it is less than 5 seconds old
    abAppend(ab, E.statusmsg, msglen);
}

void editorRefreshScreen() //refresh the screen and draw the rows of the file
{
  editorScroll();

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) //set the status message
{
  va_list ap; //va_list is a type to hold information about variable arguments
  va_start(ap, fmt); //va_start initializes the va_list
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap); //vsnprintf is the verson of snprintf that takes a va_list
  va_end(ap); //va_end cleans up the va_list
  E.statusmsg_time = time(NULL); //set the status message time to the current time
} 


/*-------------------------------------------------------------------------------------------------------------------------------*/
/******************************************************** Input ******************************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/

char *editorPrompt(char *prompt) //display a prompt and read a response from the user.
{
  size_t bufsize = 128;
  char *buf = malloc(bufsize);

  size_t buflen = 0;
  buf[0] = '\0';

  while(1)
  {
    editorSetStatusMessage(prompt, buf); //display the prompt and the contents of the buffer
    editorRefreshScreen(); //refresh the screen

    int c = editorReadKey(); //read a keypress
    
    if(c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE)
    {
      if(buflen != 0) buf[--buflen] = '\0'; //if the buffer is not empty, remove the last character
    }
    else if(c == '\x1b')
    {
      editorSetStatusMessage("");
      free(buf);
      return NULL;
    }
    else if(c == '\r')
    {
      if(buflen != 0)
      {
        editorSetStatusMessage(""); //clear the status message
        return buf;
      }
    }
    else if (!iscntrl(c) && c < 128) //if it is a printable character, append it to the buffer
    {
      if(buflen == bufsize - 1) //if the buffer is full, double its size
      {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      buf[buflen++] = c; //append the character to the buffer
      buf[buflen] = '\0';
    }
  } 
}

void editorMoveCursor(int key)  //move the cursor
{
  erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy]; //get the row that the cursor is on

  switch (key) 
  {
    case ARROW_LEFT: if (E.cx != 0) 
    { E.cx--; } 
    else if (E.cy > 0) 
    { E.cy--; E.cx = E.row[E.cy].size; }
    break;
    case ARROW_RIGHT:
      if (row && E.cx < row->size) { E.cx++; } 
      else if (row && E.cx == row->size) 
      { E.cy++; E.cx = 0; }
    break;
    case ARROW_UP:
      if (E.cy != 0) { E.cy--; }
    break;
    case ARROW_DOWN: if (E.cy < E.numrows) { E.cy++; }
    break;
  }

  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  int rowlen = row ? row->size : 0;
  if (E.cx > rowlen) 
  {
    E.cx = rowlen;
  }
}

void editorProcessKeypress() //process keypresses from the user and take appropriate action
{
  static int quit_times = KILO_QUIT_TIMES;
  int c = editorReadKey();

  switch (c) {

    case '\r': //Enter key
      editorInsertNewline();
    break;

    case CTRL_KEY('q'):
    if(E.dirty && quit_times > 0)
    {
      editorSetStatusMessage("WARNING!!! File has unsaved changes. Press Ctrl-Q %d more times to quit.", quit_times);
      quit_times--;
      return;
    }
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
    break;

    case CTRL_KEY('s'): //Ctrl-S will save the file
      editorSave();
    break;

    case HOME_KEY:
      E.cx = 0;
    break;

    case END_KEY:
      if (E.cy < E.numrows)
        E.cx = E.row[E.cy].size;
    break;

    case CTRL_KEY('f'):
      editorFind();
    break;

    case BACKSPACE:
    case CTRL_KEY('h'): //Ctrl-H is equivalent to Backspace
    case DEL_KEY:
      if(c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
      editorDelChar();
    break;

    case PAGE_UP:
    case PAGE_DOWN:
      {
        if (c == PAGE_UP) 
        {
          E.cy = E.rowoff;
        } else if (c == PAGE_DOWN) 
        {
          E.cy = E.rowoff + E.screenrows - 1;
          if (E.cy > E.numrows) E.cy = E.numrows; //if the cursor is past the end of the file, move it to the last row
        }
        //simulate the user pressing the ↑ or ↓ keys enough times to move to the top or bottom of the screen. 
        //Implementing Page Up and Page Down in this way will make it a lot easier for us later, when we implement scrolling.
        int times = E.screenrows;
        while (times--)
          editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      }
    break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
    break;

    case CTRL_KEY('l'): //CTRL-L will refresh the screen
    case '\x1b':
    break;
    
    default: editorInsertChar(c);
    break;
  }
  quit_times = KILO_QUIT_TIMES;
}

/*-------------------------------------------------------------------------------------------------------------------------------*/
/******************************************************** Initialization *********************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/
void initEditor() //initialize all fields in the E struct
{
  E.cx = 0;
  E.cy = 0;
  E.rx = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.row = NULL;
  E.dirty = 0;
  E.filename = NULL;
  E.statusmsg[0] = '\0';
  E.statusmsg_time = 0;

  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
  E.screenrows -= 2; //leave room for the status bar
}
/*-------------------------------------------------------------------------------------------------------------------------------*/
/***************************************************** Main **********************************************************************/
/*-------------------------------------------------------------------------------------------------------------------------------*/
int main(int argc, char *argv[]) //main function
{
  enableRawMode(); //enable raw mode
  initEditor(); //initialize the editor
  
  if (argc >= 2) 
  {
    editorOpen(argv[1]);
  }

  editorSetStatusMessage("HELP: Ctrl-S = Save || Ctrl-Q = Quit || Ctrl-F = Find"); //display a help message in the status bar

  while (1) 
  {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}

//Step 118: The editor is now able to open and display text files, and the user can move the cursor around and type text.