/*
 * Author: Ben Russell
 * Date: 8/7/2023
 * Purpose: Modify this (https://github.com/snaptoken/kilo-tutorial) text 
 * editor to use the ncurses library and add new features to it
 */

#include "micro.h"

int main(int argc, char *argv[]){
	initscr();
	cbreak();
	noecho();
	start_color();
	keypad(stdscr, true);

	init_pair(HL_NORMAL, COLOR_WHITE, COLOR_BLACK);
	init_pair(HL_NUMBER, COLOR_RED, COLOR_BLACK);
	init_pair(HL_MATCH, COLOR_BLACK, COLOR_BLUE);
	init_pair(HL_STRING, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(HL_COMMENT, COLOR_CYAN, COLOR_BLACK);
	init_pair(HL_MLCOMMENT, COLOR_CYAN, COLOR_BLACK);
	init_pair(HL_KEYWORD1, COLOR_YELLOW, COLOR_BLACK);
	init_pair(HL_KEYWORD2, COLOR_GREEN, COLOR_BLACK);
	init_pair(HL_SYMBOL, COLOR_BLACK, COLOR_WHITE);

	initEditor();
	if(argc >= 2){
		editorOpen(argv[1]);
	}

	editorSetStatusMessage("HELP: Ctrl-W = save | Ctrl-X = quit | Ctrl-F = find");

	editorDrawRows();

	editorRefreshScreen();
	move(0, 0);
	while(1){
		if(editorProcessKeypress()){
			editorDrawRows();
		}
		if(editorScroll()){
			editorDrawRows();
		}
		editorRefreshScreen();
	}
	
	endwin();
	return 0;
}
