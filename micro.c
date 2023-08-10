#include "micro.h"
/*
 * multi
 * line
 * comment
 * test
 */
char *C_HL_EXTENSIONS[] = { ".c", ".h", ".cpp", NULL };
char *C_HL_KEYWORDS[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",
  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};

struct editorSyntax HLDB[] = {
	{
		"c",
		C_HL_EXTENSIONS,
		C_HL_KEYWORDS,
		"//", "/*","*/",
		HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS,
	}
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))


extern struct editorConfig globalState;
struct editorConfig globalState;

static int quitTimes = QUIT_TIMES;

// syntax highlighting

int isSeparator(int c){
	return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void editorUpdateSyntax(erow *row){
	row->highlight = realloc(row->highlight, row->rSize);
	memset(row->highlight, HL_NORMAL, row->rSize);

	if(globalState.syntax == NULL){
		return;
	}

	char **keywords = globalState.syntax->keywords;

	char *scs = globalState.syntax->singleLineCommentStart;
	char *mcs = globalState.syntax->multilineCommentStart;
	char *mce = globalState.syntax->multilineCommentEnd;

	int scsLen = scs ? strlen(scs) : 0;
	int mcsLen = mcs ? strlen(mcs) : 0;
	int mceLen = mce ? strlen(mce) : 0;

	int prevSep = 1;
	int inString = 0;
	int inComment = (row->idx > 0 && globalState.row[row->idx - 1].hlOpenComment);

	int i = 0;
	while(i < row->rSize){
		char c = row->render[i];
		unsigned char prevHl = (i > 0) ? row->highlight[i - 1] : HL_NORMAL;

		if(scsLen && !inString && !inComment){
			if(!strncmp(&row->render[i], scs, scsLen)){
				memset(&row->highlight[i], HL_COMMENT, row->rSize - i);
				break;
			}
		}

		if(mcsLen && mceLen && !inString){
			if(inComment){
				row->highlight[i] = HL_MLCOMMENT;
				if(!strncmp(&row->render[i], mce, mceLen)){
					memset(&row->highlight[i], HL_MLCOMMENT, mceLen);
					i += mceLen;
					inComment = 0;
					prevSep = 1;
					continue;
				} else {
					i++;
					continue;
				}	
			} else if(!strncmp(&row->render[i], mcs, mcsLen)) {
				memset(&row->highlight[i], HL_MLCOMMENT, mcsLen);
				i += mcsLen;
				inComment = 1;
				continue;
			}
		}

		if(globalState.syntax->flags & HL_HIGHLIGHT_STRINGS){
			if(inString){
				row->highlight[i] = HL_STRING;
				if(c == '\\' && i + 1 < row->rSize){
					row->highlight[i + 1] = HL_STRING;
					i += 2;
					continue;
				}
				if(c == inString){
					inString = 0;
				}
				i++;
				prevSep = 1;
				continue;
			} else {
				if(c == '"' || c == '\''){
					inString = c;
					row->highlight[i] = HL_STRING;
					i++;
					continue;
				}
			}
		}

		if(globalState.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
			if((isdigit(c) && (prevSep || prevHl == HL_NUMBER)) ||
				(c == '.' && prevHl == HL_NUMBER)){
				row->highlight[i] = HL_NUMBER;
				i++;
				prevSep = 0;
				continue;
			}
		}

		if(prevSep) {
			int j;
			for(j = 0; keywords[j]; j++){
				int kLen = strlen(keywords[j]);
				int kw2 = keywords[j][kLen - 1] == '|';
				if(kw2){
					kLen --;
				}

				if(!strncmp(&row->render[i], keywords[j], kLen) &&
				   isSeparator(row->render[i + kLen])) {
					memset(&row->highlight[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, kLen);
					i += kLen;
					break;
				}
			}
			if(keywords[j] != NULL){
				prevSep = 0;
				continue;
			}
		}

		prevSep = isSeparator(c);
		i++;
	}

	int changed = (row->hlOpenComment != inComment);
	row->hlOpenComment = inComment;
	if(changed && row->idx + 1 < globalState.numRows){
		editorUpdateSyntax(&globalState.row[row->idx + 1]);
	}
}

void editorSelectSyntaxHighlight(){
	globalState.syntax = NULL;
	if(globalState.fileName == NULL){
		return;
	}

	char *ext = strrchr(globalState.fileName, '.');

	for(unsigned int j = 0; j < HLDB_ENTRIES; j++){
		struct editorSyntax *s = &HLDB[j];
		unsigned int i = 0;
		while(s->filematch[i]){
			int isExt = (s->filematch[i][0] == '.');
			if((isExt && ext && !strcmp(ext, s->filematch[i])) ||
			   (!isExt && strstr(globalState.fileName, s->filematch[i]))){
				globalState.syntax = s;

				int filerow;
				for(filerow = 0; filerow < globalState.numRows; filerow++){
					editorUpdateSyntax(&globalState.row[filerow]);
				}
				return;

			}
			i++;
		}
	}
}

//row operations
int editorRowCxToRx(erow *row, int cx){
	int rx = 0;
	for(int i = 0; i < cx; i++){
		if(row->chars[i] == '\t'){
			rx += (TAB_STOP - 1) - (rx % TAB_STOP);
		}
		rx++;
	}
	return rx;
}

int editorRowRxToCx(erow *row, int rx){
	int cursRx = 0;
	int cx;
	for(cx = 0; cx < row->size; cx++){
		if(row->chars[cx] == '\t'){
			cursRx += (TAB_STOP - 1) - (cursRx % TAB_STOP);
		}
		cursRx++;

		if(cursRx > rx){
			return cx;
		}
	}
	return cx;
}

void editorUpdateRow(erow *row){
	int tabs = 0;
	for(int i = 0; i < row->size; i++){
		if(row->chars[i] == '\t'){
			tabs++;
		}
	}

	free(row->render);
	row->render = NULL;

	row->render = (char *) malloc(row->size + tabs * (TAB_STOP - 1)  + 1);
		
	int numChars = 0;
	for(int i = 0; i < row->size; i++){
		if(row->chars[i] == '\t'){
			row->render[numChars++] = ' ';
			while(numChars % TAB_STOP != 0){
				row->render[numChars++] = ' ';
			}
		} else {
			row->render[numChars++] = row->chars[i];
		}
	}

	row->render[numChars] = '\0';
	row->rSize = numChars;

	editorUpdateSyntax(row);
}

void editorInsertRow(int at, char *s, size_t len){
	if(at < 0 || at > globalState.numRows){
		return;
	}

	globalState.row = realloc(globalState.row, 
			                  sizeof(erow) * (globalState.numRows + 1));

	memmove(&globalState.row[at + 1], 
			&globalState.row[at], 
			sizeof(erow) * (globalState.numRows - at));

	for(int j = at + 1; j <= globalState.numRows; j++){
		globalState.row[j].idx++;
	}

	globalState.row[at].idx = at;

	globalState.row[at].size = len;
	globalState.row[at].chars = malloc(len + 1);
	memcpy(globalState.row[at].chars, s, len);
	globalState.row[at].chars[len] = '\0';

	globalState.row[at].rSize = 0;
	globalState.row[at].render = NULL;
	globalState.row[at].highlight = NULL;
	globalState.row[at].hlOpenComment = 0;
	editorUpdateRow(&globalState.row[at]);

	globalState.numRows++; 
	globalState.dirty++;
}

void editorFreeRow(erow *row){
	free(row->render);
	free(row->chars);
	free(row->highlight);
}

void editorDelRow(int at){
	if(at < 0 || at >= globalState.numRows){
		return;
	}

	editorFreeRow(&globalState.row[at]);
	memmove(&globalState.row[at],
			&globalState.row[at + 1],
			sizeof(erow) * (globalState.numRows - at - 1));
	for(int j = at; j < globalState.numRows - 1; j++){
		globalState.row[j].idx--;
	}

	globalState.numRows--;
	globalState.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c){
	if(at < 0 || at > row->size){
		at = row->size;
	}
	row->chars = realloc(row->chars, row->size + 2);
	memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
	row->size++;
	row->chars[at] = c;
	editorUpdateRow(row);
	globalState.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len){
	row->chars = realloc(row->chars, row->size + len + 1);
	memcpy(&row->chars[row->size], s, len);
	row->size += len;
	row->chars[row->size] = '\0';
	editorUpdateRow(row);
	globalState.dirty++;
}

void editorRowDelChar(erow *row, int at){
	if(at < 0 || at >= row->size){
		return;
	}
	memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
	row->size--;
	editorUpdateRow(row);
	globalState.dirty++;
}

// editor operations
void editorInsertNewline(){
	if(globalState.cursorX == 0){
		editorInsertRow(globalState.cursorY, "", 0);
	} else {
		erow *row = &globalState.row[globalState.cursorY];
		editorInsertRow(globalState.cursorY + 1, 
						&row->chars[globalState.cursorX], 
						row->size - globalState.cursorX);

		row = &globalState.row[globalState.cursorY];
		row->size = globalState.cursorX;
		row->chars[row->size] = '\0';
		editorUpdateRow(row);
	}

	globalState.cursorY++;
	globalState.cursorX = 0;
}

void editorInsertChar(int c){
	if(globalState.cursorY == globalState.numRows) {
		editorInsertRow(globalState.numRows, "", 0);
	}
	editorRowInsertChar(&globalState.row[globalState.cursorY], globalState.cursorX, c);
	globalState.cursorX++;
}

void editorDelChar(){
	if(globalState.cursorY == globalState.numRows){
		return;
	}
	if(globalState.cursorX == 0 && globalState.cursorY == 0){
		return;
	}

	erow *row = &globalState.row[globalState.cursorY];
	if(globalState.cursorX > 0){
		editorRowDelChar(row, globalState.cursorX - 1);
		globalState.cursorX--;
	} else {
		globalState.cursorX = globalState.row[globalState.cursorY - 1].size;
		editorRowAppendString(&globalState.row[globalState.cursorY - 1],
							  row->chars, row->size);
		editorDelRow(globalState.cursorY);
		globalState.cursorY--;
	}

}

// file io
void editorSave(){
	if(globalState.fileName == NULL){
		globalState.fileName = editorPrompt("Save as: %s (ESC to cancel)", NULL);	
		if(globalState.fileName == NULL){
			editorSetStatusMessage("Save aborted");
			return;
		}
		editorSelectSyntaxHighlight();
	}

	int len;
	char *buf = editorRowsToString(&len);

	int fd = open(globalState.fileName, O_RDWR | O_CREAT, 0644);

	if(fd != -1){
		if(ftruncate(fd, len) != -1){
			if(write(fd, buf, len) == len){
				close(fd);
				free(buf);
				globalState.dirty = 0;
				editorSetStatusMessage("%d bytes written to disk", len);
				return;
			}
		}
		close(fd);
	}

	free(buf);
	editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

char *editorRowsToString(int *buflen){

	int totLen = 0;
	for(int i = 0; i < globalState.numRows; i++){
		totLen += globalState.row[i].size + 1;
	}

	*buflen = totLen;

	char *buf = malloc(totLen);
	char *p = buf;
	for(int i = 0; i < globalState.numRows; i++){
		memcpy(p, globalState.row[i].chars, globalState.row[i].size);
		p += globalState.row[i].size;
		*p = '\n';
		p++;
	}

	return buf;
}

void editorOpen(char *fileName){
	free(globalState.fileName);
	globalState.fileName = strdup(fileName);

	editorSelectSyntaxHighlight();

	FILE *fp = fopen(fileName, "r");
	if(fp == NULL){
		perror("Could not open file");
		exit(1);
	}

	char *line = NULL;
	size_t lineCap = 0;
	ssize_t lineLen;
	
	while((lineLen = getline(&line, &lineCap, fp)) != -1){
		while(lineLen > 0 && (line[lineLen - 1] == '\n' || 
							  line[lineLen - 1] == '\r')){
			lineLen--;
		}
		editorInsertRow(globalState.numRows, line, lineLen);
	}

	free(line);
	fclose(fp);
	globalState.dirty = 0;
}
// find
void editorFindCallback(char *query, int key){
	static int lastMatch = -1;
	static int direction = 1;

	static int savedHighlightLine;
	static char *savedHighlight = NULL;

	if(savedHighlight){
		memcpy(globalState.row[savedHighlightLine].highlight, savedHighlight,
			   globalState.row[savedHighlightLine].rSize);
		free(savedHighlight);
		savedHighlight = NULL;
	}

	if(key == '\n' || key == 27){
		lastMatch = -1;
		direction = 1;

		noecho();
		globalState.mode = MODE_NORMAL;
		return;
	} else if(key == KEY_RIGHT || key == KEY_DOWN){
		direction = 1;
	} else if(key == KEY_LEFT || key == KEY_UP){
		direction = -1;
	} else {
		lastMatch = -1;
		direction = 1;
	}

	if(lastMatch == -1){
		direction = 1;
	}
	int current = lastMatch;
	for(int i = 0; i < globalState.numRows; i++){
		current += direction;
		if(current == -1){
			current = globalState.numRows - 1;
		} else if(current == globalState.numRows){
			current = 0;
		}

		erow *row = &globalState.row[current];
		char *match = strstr(row->render, query);
		if(match){
			lastMatch = current;
			globalState.cursorY = current;
			globalState.cursorX = editorRowRxToCx(row,  match - row->render);


			savedHighlightLine = current;
			savedHighlight = malloc(row->rSize);
			memcpy(savedHighlight, row->highlight, row->rSize);
			memset(&row->highlight[match - row->render], HL_MATCH, strlen(query));

			editorScroll();
			editorDrawRows();
			editorRefreshScreen();
			//globalState.rowOffset = globalState.numRows;
			break;
		}
	}
}

void editorFind(){
	int savedCx = globalState.cursorX;
	int savedCy = globalState.cursorY;
	int savedColOff = globalState.colOffset;
	int savedRowOff = globalState.rowOffset;

	char *query = editorPrompt("Search: %s (Use ESC/Arrows/Enter)", editorFindCallback);

	if(query){	
		free(query);
	} else {
		globalState.cursorX = savedCx;
		globalState.cursorY = savedCy;
		globalState.colOffset = savedColOff;
		globalState.rowOffset = savedRowOff;
			}
}

//command routines
int commandMode(){
	editorSetStatusMessage("");
	globalState.mode = MODE_COMMAND;
	editorRefreshScreen();

	int tempY, tempX;
	tempY = globalState.cursorY;
	tempX = globalState.cursorX;
	// char command[50]; 
	// char garbage[8];

	// getyx(stdscr, tempY, tempX);

	curs_set(0);
	move(LINES - 1, 0);
	curs_set(1);
	printw(":");
	echo();

	//scanw("%s", command);
	
	int c;
	char *command = malloc(50 * sizeof(char));

	int count = 0;	
	while((c = getch())){
		if(c == 27){
			free(command);
			command = NULL;
			break;
		} else if(c == '\n'){
			*(command + count) = '\0';
			break;
		}

		*(command + count) = c;
		count++;
	}
	
	if(command){
		if(!strcmp(command, "q")){
			endwin();
			exit(0);
		} else if(!strcmp(command, "w")){
			editorSave();
		} else if(!strcmp(command, "f")){
			editorFind();
			return 1;
		}
	}

	noecho();

	globalState.mode = MODE_NORMAL;
	globalState.cursorY = tempY;
	globalState.cursorX = tempX;

	return 0;
}

//input
char *editorPrompt(char *prompt,  void(*callback)(char *, int)){
	size_t bufSize = 128;
	char *buf = malloc(bufSize);

	size_t bufLen = 0;
	buf[0]=  '\0';

	while(1){
		editorSetStatusMessage(prompt, buf);
		editorRefreshScreen();

		int c = getch();
		if(c == KEY_DC || c == CTRL_KEY('h') || c == KEY_BACKSPACE){
			if(bufLen != 0){
				buf[--bufLen] = '\0';
			}
		} else if(c == 27){
			editorSetStatusMessage("");
			if(callback){
				callback(buf, c);
			}
			free(buf);
			return NULL;
		} else if(c == '\n'){
			if(bufLen != 0){
				editorSetStatusMessage("");
				if(callback){
					callback(buf, c);
				}
				return buf;
			}
		} else if(!iscntrl(c) && c < 128){
			if(bufLen == bufSize - 1){
				bufSize *= 2;
				buf = realloc(buf, bufSize);
			}
			buf[bufLen++] = c;
			buf[bufLen] = '\0';
		}
		if(callback){
				callback(buf, c);
		}
	}
}

void editorMoveCursor(int key){
	erow *row = (globalState.cursorY >= globalState.numRows) ? NULL : &globalState.row[globalState.cursorY];

	if(key == KEY_LEFT || key == 'h'){
		if(globalState.cursorX != 0){
			globalState.cursorX--;
		} else if(globalState.cursorY > 0){
			globalState.cursorY--;
			globalState.cursorX = globalState.row[globalState.cursorY].size;
		}
	} else if(key == KEY_RIGHT || key == 'l'){
		if(row && globalState.cursorX < row->size){
			globalState.cursorX++;
		} else if (row && globalState.cursorX == row->size){
			globalState.cursorY++;
			globalState.cursorX = 0;
		}
	} else if(key == KEY_UP || key == 'k'){
		if(globalState.cursorY != 0){
			globalState.cursorY--;
		}

	} else if(key == KEY_DOWN || key == 'j'){
		if(globalState.cursorY < globalState.numRows){
			globalState.cursorY++;
		}
	}



	/* switch(key){
		case KEY_LEFT:
			if(globalState.cursorX != 0){
				globalState.cursorX--;
			} else if(globalState.cursorY > 0){
				globalState.cursorY--;
				globalState.cursorX = globalState.row[globalState.cursorY].size;
			}
			break;
		case KEY_RIGHT:
			if(row && globalState.cursorX < row->size){
				globalState.cursorX++;
			} else if (row && globalState.cursorX == row->size){
				globalState.cursorY++;
				globalState.cursorX = 0;
			}
			break;
		case KEY_UP:
			if(globalState.cursorY != 0){
				globalState.cursorY--;
			}
			break;
		case KEY_DOWN:
			if(globalState.cursorY < globalState.numRows){
				globalState.cursorY++;
			}
			break;
	} */

	row = (globalState.cursorY >= globalState.numRows) ? NULL : &globalState.row[globalState.cursorY];
	int rowLen = row ? row->size : 0;
	if(globalState.cursorX > rowLen){
		globalState.cursorX = rowLen;
	}
}

int editorProcessKeypress(){

	int c = getch();

	if(globalState.mode == MODE_NORMAL){
		switch(c){
			case ':':
				return commandMode();
				break;

			case 'i':
				globalState.mode = MODE_INSERT;
				editorDrawStatusBar();
				break;

			 /* case CTRL_KEY('x'):
				if(globalState.dirty && quitTimes > 0){
					editorSetStatusMessage("WARNING!!! File has unsaved changes."
							"Press Ctrl-x %d more times to quit.", quitTimes);
					quitTimes--;
					return 1;
				}
				endwin();
				exit(0);
				break;
			
			case CTRL_KEY('w'):
				editorSave();
				break;

			case CTRL_KEY('f'):
				editorFind();
				return 1;
				break; */

			case KEY_PPAGE:
			case KEY_NPAGE:
				{
					if(c == KEY_PPAGE){
						globalState.cursorY = globalState.rowOffset;
					} else if(c == KEY_NPAGE){
						globalState.cursorY = globalState.rowOffset + NEW_LINES - 1;
						if(globalState.cursorY > globalState.numRows){
							globalState.cursorY = globalState.numRows;	
						}
					}
					int times = NEW_LINES;
					while(times--){
						editorMoveCursor(c == KEY_PPAGE ? KEY_UP : KEY_DOWN);
					}
				}
				break;

			case KEY_UP:
			case 'k':
			case KEY_DOWN:
			case 'j':
			case KEY_LEFT:
			case 'h':
			case KEY_RIGHT:
			case 'l':
				editorMoveCursor(c);
				break;

			default:
				;
		}

	} else if(globalState.mode == MODE_INSERT){
		switch(c){
			case 27:
				globalState.mode = MODE_NORMAL;	
				editorDrawStatusBar();
				break;

			case '\n':
				editorInsertNewline();
				return 1;
				break;

			case KEY_BACKSPACE:
			case CTRL_KEY('h'):
			case KEY_DC:
				if(c == KEY_DC){
					editorMoveCursor(KEY_RIGHT);	
				}
				editorDelChar();
				return 1;
				break;

			default:
				editorInsertChar(c);
				return 1;
				break;
		} 

	} else if(globalState.mode == MODE_COMMAND){
	}

	/*switch(c){
		case '\n':
			editorInsertNewline();
			return 1;
			break;

		case CTRL_KEY('x'):
			if(globalState.dirty && quitTimes > 0){
				editorSetStatusMessage("WARNING!!! File has unsaved changes."
						"Press Ctrl-x %d more times to quit.", quitTimes);
				quitTimes--;
				return 1;
			}
			endwin();
			exit(0);
			break;

		case CTRL_KEY('w'):
			editorSave();
			break;

		case KEY_HOME:
			globalState.cursorX = 0;
			break;
		case KEY_END:
			if(globalState.cursorY < globalState.numRows){
				globalState.cursorX = globalState.row[globalState.cursorY].size;
			}
			break;

		case CTRL_KEY('f'):
			editorFind();
			return 1;
			break;
		
		case 27:
			break;

		case KEY_BACKSPACE:
		case CTRL_KEY('h'):
		case KEY_DC:
			if(c == KEY_DC){
				editorMoveCursor(KEY_RIGHT);	
			}
			editorDelChar();
			return 1;
			break;

		case KEY_PPAGE:
		case KEY_NPAGE:
			{
				if(c == KEY_PPAGE){
					globalState.cursorY = globalState.rowOffset;
				} else if(c == KEY_NPAGE){
					globalState.cursorY = globalState.rowOffset + NEW_LINES - 1;
					if(globalState.cursorY > globalState.numRows){
						globalState.cursorY = globalState.numRows;	
					}
				}
				int times = NEW_LINES;
				while(times--){
					editorMoveCursor(c == KEY_PPAGE ? KEY_UP : KEY_DOWN);
				}
			}
			break;

		case KEY_UP:
		case KEY_DOWN:
		case KEY_LEFT:
		case KEY_RIGHT:
			editorMoveCursor(c);
			break;

		case CTRL_KEY('l'):
			break;

		default:
			editorInsertChar(c);
			return 1;
			break;
	} */

	quitTimes = QUIT_TIMES;
	return 0;
} 

//output

int editorScroll(){
	globalState.renderX = 0;
	if(globalState.cursorY < globalState.numRows){
		globalState.renderX = editorRowCxToRx(&globalState.row[globalState.cursorY], globalState.cursorX);
	}

	if(globalState.cursorY < globalState.rowOffset){
		globalState.rowOffset = globalState.cursorY;
		return 1;
	}

	if(globalState.cursorY >= globalState.rowOffset + NEW_LINES){
		globalState.rowOffset = globalState.cursorY - NEW_LINES + 1;
		return 1;
	}

	if(globalState.renderX < globalState.colOffset){
		globalState.colOffset = globalState.renderX;
		return 1;
	}

	if(globalState.renderX >= globalState.colOffset + COLS){
		globalState.colOffset = globalState.renderX - COLS + 1;
		return 1;
	}

	return 0;
}

void editorDrawRows(){
	curs_set(0);
	clear();

	if(globalState.numRows < 1){
		int messageLen = strlen(WELCOME_MESSAGE) + strlen(MICRO_VERSION);
		mvprintw(NEW_LINES / 3, (COLS - messageLen) / 2, "%s %s", WELCOME_MESSAGE, MICRO_VERSION);
	}

	for(int i = 0; i < NEW_LINES; i++){
		if(i + globalState.rowOffset >= globalState.numRows){
			mvprintw(i, 0, "~");
		} else {
			char *tempRow = globalState.row[i + globalState.rowOffset].render + globalState.colOffset;
			char *highlight = globalState.row[i + globalState.rowOffset].highlight + globalState.colOffset;
			for(int j = 0; j < strlen(tempRow); j++){
				chtype c = tempRow[j];
				if(iscntrl(tempRow[j])){
					char sym = (tempRow[j] <= 26) ? '@' + tempRow[j] : '?';
					c = sym | COLOR_PAIR(HL_SYMBOL);
				} else if(highlight[j] == HL_NORMAL){
					c = tempRow[j] | COLOR_PAIR(HL_NORMAL);
				} else {
					c = tempRow[j] | COLOR_PAIR(highlight[j]);	
				}
				mvaddch(i, j, c);
			}
			
			//mvprintw(i, 0, "%s", globalState.row[i + globalState.rowOffset].render + globalState.colOffset);
		}
	}

	curs_set(1);
}

void editorDrawStatusBar(){
	int tempY, tempX;
	tempY = globalState.cursorY;
	tempX = globalState.cursorX;

	curs_set(0);

	move(NEW_LINES, 0);
	//attron(COLOR_PAIR(1));
	attron(A_REVERSE);
	for(int i = 0; i < COLS; i++){
		mvprintw(NEW_LINES, i, " ");
	}

	char *modeMessage = NULL;

	if(globalState.mode == MODE_NORMAL){
		modeMessage = " NORMAL ";
	} else if(globalState.mode == MODE_INSERT){
		modeMessage = " INSERT ";
	} else if(globalState.mode == MODE_COMMAND){
		modeMessage = " COMMAND ";
	} else {
		modeMessage = " ERROR ";
	}

	int messageLen = strlen(modeMessage);

	attroff(A_REVERSE);
	attron(COLOR_PAIR(globalState.mode));

	mvprintw(NEW_LINES, 0, "%s", modeMessage);

	modeMessage = NULL;

	attroff(COLOR_PAIR(globalState.mode));
	attron(A_REVERSE);

	mvprintw(NEW_LINES, messageLen, " %.20s - %d lines %s", 
			globalState.fileName ? globalState.fileName : "[No Name]", 
			globalState.numRows,
			globalState.dirty ? "(modified)" : "");

	int numDigitsRows = floor(log10(globalState.numRows) + 1); 
	int numDigitsCursor = floor(log10(globalState.cursorY + 1) + 1);
	int ftLen = 0;

	char *fileTypeString = globalState.syntax ? globalState.syntax->fileType : "no ft";

	ftLen = strlen(fileTypeString);

	attroff(A_REVERSE);
	attron(COLOR_PAIR(globalState.mode));
	mvprintw(NEW_LINES, COLS - (numDigitsRows + 1), "/%d", globalState.numRows);

	mvprintw(NEW_LINES, COLS - (numDigitsRows + numDigitsCursor + 2), " %d", globalState.cursorY + 1);
	attroff(COLOR_PAIR(globalState.mode));
	attron(A_REVERSE);

	mvprintw(NEW_LINES, COLS - (numDigitsRows + numDigitsCursor + ftLen + 3),
			 "%s ", fileTypeString);


	//attroff(COLOR_PAIR(1));
	attroff(A_REVERSE);

	move(tempY, tempX);
	curs_set(1);
}

void editorDrawMessageBar(){
	int tempY, tempX;
	tempY = globalState.cursorY;
	tempX = globalState.cursorX;
	
	curs_set(0);

	for(int i = 0; i < COLS; i++){
		mvprintw(LINES - 1, i, " ");
	}

	int msgLen = strlen(globalState.statusMessage);
	
	if(msgLen > COLS){
		msgLen = COLS;
	}

	if(msgLen && time(NULL) - globalState.statusMesageTime < 5){
		mvprintw(LINES - 1, 0, "%s", globalState.statusMessage);
	}

	move(tempY, tempX);
	curs_set(1);

}

void editorRefreshScreen(){
	int tempY, tempX;
	tempY = globalState.cursorY;
	tempX = globalState.cursorX;

	editorDrawStatusBar();

	editorDrawMessageBar();

	move(globalState.cursorY - globalState.rowOffset, globalState.renderX - globalState.colOffset);
	refresh();
}

void editorSetStatusMessage(const char *fmt, ...){
	int tempY, tempX;
	tempY = globalState.cursorY;
	tempX = globalState.cursorX;

	curs_set(0);

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(globalState.statusMessage, sizeof(globalState.statusMessage), fmt, ap);
	va_end(ap);
	globalState.statusMesageTime = time(NULL);

	move(tempY, tempX);
	curs_set(1);
}

//init

void initEditor(){
	globalState.cursorY = 0;
	globalState.cursorX = 0;
	globalState.renderX = 0;
	globalState.rowOffset = 0;
	globalState.colOffset = 0;
	globalState.numRows = 0;
	globalState.mode = MODE_NORMAL;
	globalState.row = NULL;
	globalState.dirty = 0;
	globalState.fileName = NULL;
	globalState.statusMessage[0] = '\0';
	globalState.statusMesageTime = 0;
	globalState.syntax = NULL;
}
