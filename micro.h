#ifndef MICRO_H_INCLUDED
#define MICRO_H_INCLUDED

#include<ncurses.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<time.h>
#include<math.h>
#include<stdarg.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<ctype.h>

#define MICRO_VERSION "0.0.2"
#define WELCOME_MESSAGE "Micro editor -- version"
#define TAB_STOP 8
#define QUIT_TIMES 3

#define CTRL_KEY(x) ((x) & 0x1f)
#define NEW_LINES (LINES - 2)

#define COLOR_BKGD 8

enum editorHighlight {
	HL_NORMAL = 0,
	HL_COMMENT,
	HL_MLCOMMENT,
	HL_KEYWORD1,
	HL_KEYWORD2,
	HL_STRING,
	HL_NUMBER,
	HL_MATCH,
	HL_SYMBOL
};

#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

struct editorSyntax {
	char *fileType;
	char **filematch;
	char **keywords;
	char *singleLineCommentStart;
	char *multilineCommentStart;
	char *multilineCommentEnd;
	int flags;
};

typedef struct erow {
	int idx;
	int size;
	int rSize;
	char *chars;
	char *render;
	char *highlight;
	int hlOpenComment;
} erow;

struct editorConfig {
	int cursorY, cursorX;
	int renderX;
	int rowOffset;
	int colOffset;
	int numRows;
	char *fileName;
	char statusMessage[80];
	time_t statusMesageTime;
	erow *row;
	int dirty;
	struct editorSyntax *syntax;
};

//file types

// syntax highlighting
int isSeparator(int c);
void editorUpdateSyntax(erow *row);
void editorSelectSyntaxHighlight();

//row operations
int editorRowCxToRx(erow *row, int cx);
int editorRowRxToCx(erow *row, int rx);
void editorUpdateRow(erow *row);
void editorInsertRow(int at, char *s, size_t len);	
void editorFreeRow(erow *row);
void editorDelRow(int at);
void editorRowInsertChar(erow *row, int at, int c);
void editorRowAppendString(erow *row, char *s, size_t len);
void editorRowDelChar(erow *row, int at);

//editor operations
void editorInsertNewline();
void editorInsertChar(int c);
void editorDelChar();

//file i/o
void editorSave();
char *editorRowsToString(int *buflen);
void editorOpen(char *fileName);

//find
void editorFind();

//input
char *editorPrompt(char *prompt, void(*callback)(char *, int));
void editorMoveCursor(int key);
int editorProcessKeypress();

//output
int editorScroll();
void editorDrawRows();
void editorDrawStatusBar();
void editorDrawMessageBar();
void editorRefreshScreen();
void editorSetStatusMessage(const char *fmt, ...);

//init
void initEditor();


#endif


