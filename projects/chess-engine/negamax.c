/* negamax.c: Functions for computing optimal chess moves
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
#include <limits.h>
#include "common.h"
#include "board.h"
#include "negamax.h"

static unsigned long expanded_moves;
static unsigned long evaluated_moves;

static inline int max(int a, int b)
{
	return a > b ? a : b;
}

/* Enumeration of moves is batched by piece. Since we have the oppurtunity to
 * prune after the evaluation of each potential move, it's possible that we
 * end up not examining moves that we wasted time enumerating.
 *
 * Typically this overhead appears to be < 1%, which is probably less than the
 * overhead of the extra logic that would be required to evaluate between the
 * enumeration of each move. However, it can be as great as 10% with certain
 * board setups, so I need to think about this more...
 *
 * (I originally batched them by board, which resulted in waste overheads of
 * as high as 75%... so this is still a gigantic improvement.)
 */

static int negamax_algo(struct chessboard *c, int color, int depth, int alpha, int beta)
{
	struct move_list *l;
	struct raw_move m;
	int n;
	int val, best_val = INT_MIN;

	if (!depth)
		return !color ? calculate_board_heuristic(c) : -calculate_board_heuristic(c);

	l = allocate_move_list();
	for (int i = 0; i < 16; i++) {
		n = enumerate_moves(c, (color << 4) | i, l);
		expanded_moves += n;

		for (int j = 0; j < n; j++) {
			get_move(l, j, &m.sx, &m.sy, &m.dx, &m.dy);

			execute_raw_move(c, &m);
			evaluated_moves++;

			val = -negamax_algo(c, !color, depth - 1, -beta, -alpha);

			unwind_raw_move(c, &m);

			best_val = max(best_val, val);
			alpha = max(alpha, val);
			if (alpha >= beta)
				break;
		}
	}

	free_move_list(l);
	return best_val;
}

/* Returns sx|sy|dx|dy in an integer byte-by-byte from least to most
 * significant, indicating which move should be made next.
 *
 * We seperate the initial iteration of negamax out like this to track the
 * actual move associated with the best score. Doing so during the
 * deeper iterations is a waste of time. */
unsigned int calculate_move(struct chessboard *c, int color, int depth)
{
	struct move_list *l;
	int n, fbsx = -1, fbsy = -1, fbdx = -1, fbdy = -1;
	int val, best_val = INT_MIN, alpha = INT_MIN, beta = INT_MAX;
	struct raw_move m;
	struct timespec *t;

	expanded_moves = 0;
	evaluated_moves = 0;

	t = start_timer();
	l = allocate_move_list();
	for (int i = 0; i < 16; i++) {
		n = enumerate_moves(c, (color << 4) | i, l);
		expanded_moves += n;

		for (int j = 0; j < n; j++) {
			get_move(l, j, &m.sx, &m.sy, &m.dx, &m.dy);
			execute_raw_move(c, &m);
			evaluated_moves++;

			val = -negamax_algo(c, !color, depth - 1, -beta, -alpha);

			alpha = max(alpha, val);
			if (val > best_val) {
				best_val = val;
				fbsx = m.sx;
				fbsy = m.sy;
				fbdx = m.dx;
				fbdy = m.dy;
			}

			debug("Move %d/%d for piece %d (%d,%d) => (%d,%d) has heuristic value %d\n", j + 1, n, (color << 4) | i, m.sx, m.sy, m.dx, m.dy, val);
			unwind_raw_move(c, &m);
		}
	}
	free_move_list(l);
	expanded_moves += n;
	debug("Evaluated %luM/%luM expanded moves in %lu seconds\n", evaluated_moves / 1000000, expanded_moves / 1000000, get_timer_and_free(t) / 1000);

	return (fbsx) | (fbsy << 8) | (fbdx << 16) | (fbdy << 24);
}
