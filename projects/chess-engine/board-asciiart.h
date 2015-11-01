#pragma once

/* Stuff for printing chessboards.
 *
 * I think this is rather clever: I hardcode the ANSI terminal codes for
 * white (blue) and black (green), and build a chessboard grid with enough
 * space for the ANSI blob and the letter. So to print the board, we simply
 * iterate over the board and copy what we find into the grid with the proper
 * ANSI color blob.
 *
 * There are probably faster ways to do this, but printing the thing isn't at
 * all a performance sensitive part of the program, so I didn't really worry
 * about it.
 */

static const char *asciiart_board_skel = "\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n\
|          |          |          |          |          |          |          |          |\n\
-----------------\n";

static const char *ansi_chess_colors[2] = {"\x1b[36m!\x1b[0m", "\x1b[32m!\x1b[0m"};

static inline char get_character(unsigned char p)
{
	switch (p & 0x7) {
	case 0:	return ' ';
	case 1:	return 'P';
	case 2:	return 'R';
	case 3:	return 'N';
	case 4:	return 'B';
	case 5:	return 'Q';
	case 6:	return 'K';
	}

	return '?';
}
