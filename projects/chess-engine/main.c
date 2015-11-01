/* chess-engine: A (currently very simplistic) chess AI
 *
 * Copyright (C) 2014 Calvin Owens
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of VERSION 2 ONLY of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THIS SOFTWARE. */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "common.h"
#include "board.h"
#include "negamax.h"

#define MOVE_DEPTH 5

/* I'm being a little silly and using POSIX error codes for these. Meh. */
static char *get_error_string(int error_code)
{
	switch (error_code) {
	case -EINVAL:	return "Invalid move";
	case -EEXIST:	return "Another piece is blocking that move";
	case -ENOENT:	return "No piece exists at the source coordinates";
	case -EPERM:	return "Your king is in check after executing that move";
	case -EFAULT:	return "Pieces cannot capture themselves";
	case -ERANGE:	return "Coordinates are out-of-range";
	case -EACCES:	return "You cannot capture your own pieces";
	default:	return "Unknown error code. Wat.";
	}
}

int main(void)
{
	struct chessboard *c = get_new_board();
	int tmp, sx, sy, dx, dy;

	while (1) {
		print_chessboard(c);
		/* Calcluate white's suggested move */
		tmp = calculate_move(c, 0, MOVE_DEPTH);
		sx = tmp & 0xff;
		sy = (tmp & 0xff00) >> 8;
		dx = (tmp & 0xff0000) >> 16;
		dy = (tmp & 0xff000000) >> 24;
		printf("Computer suggests (%d,%d) -> (%d,%d)\n", sx, sy, dx, dy);

no_clear:
		printf("Enter move: ");
		tmp = scanf("%d %d %d %d", &sx, &sy, &dx, &dy);
		if (tmp != 4)
			goto no_clear;

		if (sx == -1)
			return 0;

		tmp = execute_move(c, sx, sy, dx, dy);
		if (tmp) {
			printf("Error: %s\n", get_error_string(tmp));
			goto no_clear;
		} else {
			printf("Move succeeded!\n");
		}

		/* Calcluate black's move */
		tmp = calculate_move(c, 1, MOVE_DEPTH);
		sx = tmp & 0xff;
		sy = (tmp & 0xff00) >> 8;
		dx = (tmp & 0xff0000) >> 16;
		dy = (tmp & 0xff000000) >> 24;
		printf("Black moves (%d,%d) -> (%d,%d)\n", sx, sy, dx, dy);
		tmp = execute_move(c, sx, sy, dx, dy);
		if (tmp)
			fatal("Computer tried to make an illegal move: %s\n", get_error_string(tmp));
	}

	return 0;
}
