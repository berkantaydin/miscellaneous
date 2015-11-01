/* board.c: Functions and structures for representing the state of a chessgame
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
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include "common.h"
#include "board.h"

/* The chess board is represented as a matrix of 8-bit integers, each of which
 * is divided as follows from least to most significant: 3-bit piece type
 * indication, 4-bit piece number, 1-bit color indicator (0 for white, 1 for
 * black). The piece number is unique on the board if the color is included.
 *
 * The codes for piece type are:
 * 	000 (0): Empty
 * 	001 (1): Pawn
 * 	010 (2): Rook
 * 	011 (3): Knight
 * 	100 (4): Bishop
 * 	101 (5): Queen
 * 	110 (6): King
 * 	111 (7): RESERVED
 *
 * In order to make calculations on pieces more efficient, the current (x,y)
 * position of each piece on the board is also maintained in an array indexed
 * by ((color << 4) + number). The (x,y) value (15,15) is used to indicate a
 * piece that no longer exists.
 *
 * Empty squares are always white and numbered zero.
 */

/* I specify the alignment so GCC will use the 256-bit AVX registers to copy
 * the structure around. Eventually need to determine if it's really a win... */
struct chessboard {
	unsigned char b[8][8]; /* [y][x] */
	unsigned char p[32];
} __attribute__((aligned(256)));

/* Helpful macros for working with struct chessboard */
#define P(type, color, number) (((color << 7) | (number << 3) | type))
#define PIECE_NUMBER(p) ((p >> 3))
#define PIECE_COLOR(p) ((p >> 7))
#define PIECE_TYPE(p) ((p & 0x07))

#define B(a,b) (((b << 4) | a))

/* Static definition for a starting chessboard layout
 * Viewed here as though you are sitting as black looking at the board, so the
 * rows at the top are white's pieces. */
static const struct chessboard starting_board = {
	.b = {	{ P(2,0,0x0),P(3,0,0x1),P(4,0,0x2),P(5,0,0x3),P(6,0,0x4),P(4,0,0x5),P(3,0,0x6),P(2,0,0x7) },
		{ P(1,0,0x8),P(1,0,0x9),P(1,0,0xa),P(1,0,0xb),P(1,0,0xc),P(1,0,0xd),P(1,0,0xe),P(1,0,0xf) },
		{ P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0) },
		{ P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0) },
		{ P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0) },
		{ P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0),P(0,0,0x0) },
		{ P(1,1,0x8),P(1,1,0x9),P(1,1,0xa),P(1,1,0xb),P(1,1,0xc),P(1,1,0xd),P(1,1,0xe),P(1,1,0xf) },
		{ P(2,1,0x0),P(3,1,0x1),P(4,1,0x2),P(5,1,0x3),P(6,1,0x4),P(4,1,0x5),P(3,1,0x6),P(2,1,0x7) }},
	.p = {	B(0,0),B(1,0),B(2,0),B(3,0),B(4,0),B(5,0),B(6,0),B(7,0),B(0,1),B(1,1),B(2,1),B(3,1),B(4,1),B(5,1),B(6,1),B(7,1),
		B(0,7),B(1,7),B(2,7),B(3,7),B(4,7),B(5,7),B(6,7),B(7,7),B(0,6),B(1,6),B(2,6),B(3,6),B(4,6),B(5,6),B(6,6),B(7,6)}
};

/* Get the info for the piece at location (x,y) */
static inline unsigned int get_pos_info(struct chessboard *c, int x, int y)
{
	return c->b[y][x];
}

/* Get the (x,y) coordinates for the piece numbered @pnum. */
static inline void get_piece_coordinates(struct chessboard *c, int pnum, unsigned char *x, unsigned char *y)
{
	unsigned char p;

	p = c->p[pnum];
	*x = p & 0x0f;
	*y = p >> 4;
}

void execute_raw_move(struct chessboard *c, struct raw_move *m)
{
	m->sp = c->b[m->sy][m->sx];
	c->b[m->sy][m->sx] = 0;

	m->dp = c->b[m->dy][m->dx];
	c->b[m->dy][m->dx] = m->sp;

	c->p[PIECE_NUMBER(m->sp)] = B(m->dx, m->dy);

	if (m->dp)
		c->p[PIECE_NUMBER(m->dp)] = B(15, 15);
}

void unwind_raw_move(struct chessboard *c, struct raw_move *m)
{
	c->b[m->sy][m->sx] = m->sp;
	c->b[m->dy][m->dx] = m->dp;

	c->p[PIECE_NUMBER(m->sp)] = B(m->sx, m->sy);

	if (m->dp)
		c->p[PIECE_NUMBER(m->dp)] = B(m->dx, m->dy);
}

/* Validation functions
 *
 * Check if a particular move is legal for that type of piece. Returns 0 if valid,
 * a negative error code otherwise. If applicable, they check for pieces in the
 * path of movement and in the case of pawns, validate captures.
 *
 * These are all relatively straightforward except for pawns, which are annoying.
 */

static int validate_pawn_move(struct chessboard *c, int sx, int sy, int dx, int dy)
{
	int dist, starting_rank, direction, tmp;

	/* Black pawn, or white pawn?
	 * Note that we do not use the absolute value of the distance here so
	 * we can validate that the pawn is moving in the proper direction. */
	if (get_pos_info(c, sx, sy) >> 7 == 0) {
		dist = dy - sy;
		starting_rank = 1;
		direction = 1;
	} else {
		dist = sy - dy;
		starting_rank = 6;
		direction = -1;
	}

	if (dx == sx) {
		/* Pawns can only move forward one space, unless in their
		 * starting position, in which case they can move forward one
		 * or two spaces */

		if (sy == starting_rank) {
			if (dist != 1 && dist != 2) {
				return -EINVAL;
			}

			/* Make sure we're not jumping over a piece */
			if (dist == 2) {
				tmp = get_pos_info(c, sx, sy + direction);
				if (tmp) {
					return -EEXIST;
				}
			}
		} else {
			if (dist != 1) {
				return -EINVAL;
			}
		}

		/* Pawns cannot capture going forward, only diagonally */
		if (get_pos_info(c, dx, dy))
			return -EEXIST;

		return 0;
	} else {
		/* Pawns can only move diagonally one space forward */
		if (dist != 1)
			return -EINVAL;
		if (dx != (sx + 1) && dx != (sx - 1))
			return -EINVAL;

		/* Pawns can only move diagonally to capture */
		if (!get_pos_info(c, dx, dy))
			return -EINVAL;

		return 0;
	}
}

static int validate_rook_move(struct chessboard *c, int sx, int sy, int dx, int dy)
{
	int adjx, adjy;

	if (sx != dx && sy != dy)
		return -EINVAL;

	adjx = (sx == dx) ? 0 : ((dx > sx) ? 1 : -1);
	adjy = (sy == dy) ? 0 : ((dy > sy) ? 1 : -1);

	/* Check for pieces in the path of movement */
	while (((sx += adjx) != dx) + ((sy += adjy) != dy)) {
		if (get_pos_info(c, sx, sy))
			return -EEXIST;
	}

	return 0;
}

static int validate_knight_move(struct chessboard *c, int sx, int sy, int dx, int dy)
{
	int mvx, mvy;

	mvx = abs(dx - sx);
	mvy = abs(dy - sy);

	if (!((mvx == 2 && mvy == 1) || (mvx == 1 && mvy == 2)))
		return -EINVAL;

	return 0;
}

static int validate_bishop_move(struct chessboard *c, int sx, int sy, int dx, int dy)
{
	int mvx, mvy, adjx, adjy;

	mvx = dx - sx;
	mvy = dy - sy;
	if (abs(mvx) != abs(mvy))
		return -EINVAL;

	adjx = (mvx > 0) ? 1 : -1;
	adjy = (mvy > 0) ? 1 : -1;

	/* Check for pieces in the path of movement */
	while (((sx += adjx) != dx) + ((sy += adjy) != dy)) {
		if (get_pos_info(c, sx, sy))
			return -EEXIST;
	}

	return 0;
}

static int validate_queen_move(struct chessboard *c, int sx, int sy, int dx, int dy)
{
	int r, b;

	r = validate_rook_move(c, sx, sy, dx, dy);
	if (!r)
		return 0;

	b = validate_bishop_move(c, sx, sy, dx, dy);
	if (!b)
		return 0;

	if (r == b)
		return -EINVAL;
	return -EEXIST;
}

static int validate_king_move(struct chessboard *c, int sx, int sy, int dx, int dy)
{
	int mvx, mvy;

	mvx = dx - sx;
	mvy = dy - sy;

	if (!(abs(mvx) <= 1 && abs(mvy) <= 1))
		return -EINVAL;

	return 0;
}

static int (*const val_funcs[8])(struct chessboard *, int, int, int, int) = {	NULL, &validate_pawn_move,
										&validate_rook_move, &validate_knight_move,
										&validate_bishop_move, &validate_queen_move,
										&validate_king_move, NULL};

/* Execute a chess move.
 * Returns 0 if successful, a negative error code if not:
 * 	-EINVAL:	Move is not valid for that piece
 * 	-EEXIST:	There is a piece in the way
 * 	-ENOENT:	No piece exists at (sx,sy)
 * 	-EACCES:	Cannot capture your own pieces
 * 	-EPERM:		Your king is in check after executing this move
 * 	-EFAULT:	(sx,sy) == (dx,dy)
 * 	-ERANGE:	(sx,sy) or (dx,dy) are not on the board */
static int do_execute_move(struct chessboard *c, struct raw_move *m)
{
	int sp, v;
	unsigned int tmp;

	/* Cannot move a piece off the board */
	if (abs(m->sx) > 7 || abs(m->sy) > 7 || abs(m->dx) > 7 || abs(m->dy) > 7)
		return -ERANGE;

	/* Cannot move piece to it's current location */
	if ((m->sx == m->dx) && (m->sy == m->dy))
		return -EFAULT;

	/* The piece we are moving must exist */
	sp = get_pos_info(c, m->sx, m->sy);
	if (!sp)
		return -ENOENT;

	/* Make sure the move is legal for this particular piece */
	v = (*val_funcs[sp & 0x7])(c, m->sx, m->sy, m->dx, m->dy);
	if (v)
		return v;

	/* Cannot capture pieces of the same color */
	tmp = get_pos_info(c, m->dx, m->dy);
	if (tmp)
		if (!(PIECE_COLOR(sp) ^ PIECE_COLOR(tmp)))
			return -EACCES;

	/* Move is valid, do it */
	execute_raw_move(c, m);

	return 0;
}

int execute_move(struct chessboard *c, int sx, int sy, int dx, int dy)
{
	struct raw_move m = {
		.sx = sx,
		.sy = sy,
		.dx = dx,
		.dy = dy,
		.sp = 0,
		.dp = 0,
	};

	return do_execute_move(c, &m);
}

/* Move Lists
 *
 * These hold the moves enumerated for a given piece as packed 4-bit integers
 * signifying (sx,sy) and (dx,dy). A single move_list can hold 32 moves, which
 * is overkill since the maximum number of moves a given piece can have is 27.
 *
 * The alignment here is to improve cache locality. I'm much more certain this
 * one is worth it, especially since it doesn't result in wasted space
 */

struct move_list {
	char m[64];
} __attribute__((aligned(64)));

void zero_move_list(struct move_list *l)
{
	memset(l, 0x00, sizeof(struct move_list));
}

struct move_list *allocate_move_list(void)
{
	void *ret;

	if (posix_memalign(&ret, 64, 64))
		fatal("-ENOMEM allocating move_list\n");

	return ret;
}

void free_move_list(struct move_list *l)
{
	free(l);
}

void get_move(struct move_list *l, int n, unsigned char *sx, unsigned char *sy, unsigned char *dx, unsigned char *dy)
{
	n <<= 1;
	*sx = l->m[n] & 0x0f;
	*sy = l->m[n] >> 4;
	*dx = l->m[n + 1] & 0x0f;
	*dy = l->m[n + 1] >> 4;
}

/* Function to insert a move into a move_list */
static void set_move(struct move_list *l, unsigned int n, unsigned char sx, unsigned char sy, unsigned char dx, unsigned char dy)
{
	n <<= 1;
	l->m[n] = B(sx,sy);
	l->m[n + 1] = B(dx,dy);
}

/* Enumeration functions
 *
 * These functions enumerate all the legal moves for the piece located at
 * (sx,sy), appending them to the move_list provided.
 *
 * Note that these functions do not verify that a move does not place your king
 * in check or fail to remove your king from check: that is much easier to do
 * later.
 *
 * The functions return the number of moves that were appended to the list.
 *
 * These are much less elegant than the validation functions. Oh well...
 */

static int enumerate_pawn_moves(struct chessboard *c, int sx, int sy, struct move_list *l, int loff)
{
	int color, ending_rank, starting_rank, direction, n = loff;

	if (PIECE_COLOR(get_pos_info(c, sx, sy)) == 0) {
		ending_rank = 7;
		starting_rank = 1;
		direction = 1;
		color = 0;
	} else {
		ending_rank = 0;
		starting_rank = 6;
		direction = -1;
		color = 1;
	}

	if (sy != ending_rank && !get_pos_info(c, sx, sy + direction))
		set_move(l, loff++, sx, sy, sx, sy + direction);

	if (sy == starting_rank && !get_pos_info(c, sx, sy + direction + direction))
		set_move(l, loff++, sx, sy, sx, sy + direction + direction);

	if (sx != 0 && sy != ending_rank)
		if (get_pos_info(c, sx - 1, sy + direction) && color ^ PIECE_COLOR(get_pos_info(c, sx - 1, sy + direction)))
			set_move(l, loff++, sx, sy, sx - 1, sy + direction);

	if (sx != 7 && sy != ending_rank)
		if (get_pos_info(c, sx + 1, sy + direction) && color ^ PIECE_COLOR(get_pos_info(c, sx + 1, sy + direction)))
			set_move(l, loff++, sx, sy, sx + 1, sy + direction);

	return loff - n;
}

static int enumerate_rook_moves(struct chessboard *c, int sx, int sy, struct move_list *l, int loff)
{
	int n = loff, tdx, tdy, color;

	color = get_pos_info(c, sx, sy) >> 7;

	/* Walk North */
	tdy = sy;
	while ((tdy += 1) <= 7) {
		if (!get_pos_info(c, sx, tdy)) {
			set_move(l, loff++, sx, sy, sx, tdy);
		} else {
			if (PIECE_COLOR(get_pos_info(c, sx, tdy)) ^ color) {
				set_move(l, loff++, sx, sy, sx, tdy);
			}

			break;
		}
	}

	/* Walk South */
	tdy = sy;
	while ((tdy -= 1) >= 0) {
		if (!get_pos_info(c, sx, tdy)) {
			set_move(l, loff++, sx, sy, sx, tdy);
		} else {
			if (PIECE_COLOR(get_pos_info(c, sx, tdy)) ^ color) {
				set_move(l, loff++, sx, sy, sx, tdy);
			}

			break;
		}
	}

	/* Walk East */
	tdx = sx;
	while ((tdx += 1) <= 7) {
		if (!get_pos_info(c, tdx, sy)) {
			set_move(l, loff++, sx, sy, tdx, sy);
		} else {
			if (PIECE_COLOR(get_pos_info(c, tdx, sy)) ^ color) {
				set_move(l, loff++, sx, sy, tdx, sy);
			}

			break;
		}
	}

	/* Walk West */
	tdx = sx;
	while ((tdx -= 1) >= 0) {
		if (!get_pos_info(c, tdx, sy)) {
			set_move(l, loff++, sx, sy, tdx, sy);
		} else {
			if (PIECE_COLOR(get_pos_info(c, tdx, sy)) ^ color) {
				set_move(l, loff++, sx, sy, tdx, sy);
			}

			break;
		}
	}

	return loff - n;
}

static int enumerate_knight_moves(struct chessboard *c, int sx, int sy, struct move_list *l, int loff)
{
	int n = loff, color, tmp;

	color = PIECE_COLOR(get_pos_info(c, sx, sy));

	if (sx <= 6 && sy <= 5) {
		tmp = get_pos_info(c, sx + 1, sy + 2);
		if (!tmp || PIECE_COLOR(tmp) ^ color)
			set_move(l, loff++, sx, sy, sx + 1, sy + 2);
	}

	if (sx <= 5 && sy <= 6) {
		tmp = get_pos_info(c, sx + 2, sy + 1);
		if (!tmp || PIECE_COLOR(tmp) ^ color)
			set_move(l, loff++, sx, sy, sx + 2, sy + 1);
	}

	if (sx <= 5 && sy >= 1) {
		tmp = get_pos_info(c, sx + 2, sy - 1);
		if (!tmp || PIECE_COLOR(tmp) ^ color)
			set_move(l, loff++, sx, sy, sx + 2, sy - 1);
	}

	if (sx <= 6 && sy >= 2) {
		tmp = get_pos_info(c, sx + 1, sy - 2);
		if (!tmp || PIECE_COLOR(tmp) ^ color)
			set_move(l, loff++, sx, sy, sx + 1, sy - 2);
	}

	if (sx >= 1 && sy >= 2) {
		tmp = get_pos_info(c, sx - 1, sy - 2);
		if (!tmp || PIECE_COLOR(tmp) ^ color)
			set_move(l, loff++, sx, sy, sx - 1, sy - 2);
	}

	if (sx >= 2 && sy >= 1) {
		tmp = get_pos_info(c, sx - 2, sy - 1);
		if (!tmp || PIECE_COLOR(tmp) ^ color)
			set_move(l, loff++, sx, sy, sx - 2, sy - 1);
	}

	if (sx >= 2 && sy <= 6) {
		tmp = get_pos_info(c, sx - 2, sy + 1);
		if (!tmp || PIECE_COLOR(tmp) ^ color)
			set_move(l, loff++, sx, sy, sx - 2, sy + 1);
	}

	if (sx >= 1 && sy <= 5) {
		tmp = get_pos_info(c, sx - 1, sy + 2);
		if (!tmp || PIECE_COLOR(tmp) ^ color)
			set_move(l, loff++, sx, sy, sx - 1, sy + 2);
	}


	return loff - n;
}

static int enumerate_bishop_moves(struct chessboard *c, int sx, int sy, struct move_list *l, int loff)
{
	int n = loff, tdx, tdy, color;

	color = PIECE_COLOR(get_pos_info(c, sx, sy));

	/* Walk Northeast */
	tdx = sx;
	tdy = sy;
	while (((tdx += 1) <= 7) && ((tdy += 1) <= 7)) {
		if (!get_pos_info(c, tdx, tdy)) {
			set_move(l, loff++, sx, sy, tdx, tdy);
		} else {
			if (PIECE_COLOR(get_pos_info(c, tdx, tdy)) ^ color) {
				set_move(l, loff++, sx, sy, tdx, tdy);
			}

			break;
		}
	}

	/* Walk Southeast */
	tdx = sx;
	tdy = sy;
	while (((tdx += 1) <= 7) && ((tdy -= 1) >= 0)) {
		if (!get_pos_info(c, tdx, tdy)) {
			set_move(l, loff++, sx, sy, tdx, tdy);
		} else {
			if (PIECE_COLOR(get_pos_info(c, tdx, tdy)) ^ color) {
				set_move(l, loff++, sx, sy, tdx, tdy);
			}

			break;
		}
	}

	/* Walk Northwest */
	tdx = sx;
	tdy = sy;
	while (((tdx -= 1) >= 0) && ((tdy += 1) <= 7)) {
		if (!get_pos_info(c, tdx, tdy)) {
			set_move(l, loff++, sx, sy, tdx, tdy);
		} else {
			if (PIECE_COLOR(get_pos_info(c, tdx, tdy)) ^ color) {
				set_move(l, loff++, sx, sy, tdx, tdy);
			}

			break;
		}
	}

	/* Walk Southwest */
	tdx = sx;
	tdy = sy;
	while (((tdx -= 1) >= 0) && ((tdy -= 1) >= 0)) {
		if (!get_pos_info(c, tdx, tdy)) {
			set_move(l, loff++, sx, sy, tdx, tdy);
		} else {
			if (PIECE_COLOR(get_pos_info(c, tdx, tdy)) ^ color) {
				set_move(l, loff++, sx, sy, tdx, tdy);
			}

			break;
		}
	}

	return loff - n;
}

static int enumerate_queen_moves(struct chessboard *c, int sx, int sy, struct move_list *l, int loff)
{
	int n = loff;

	loff += enumerate_rook_moves(c, sx, sy, l, loff);
	loff += enumerate_bishop_moves(c, sx, sy, l, loff);

	return loff - n;
}

/* This one is extra shitty */
static int enumerate_king_moves(struct chessboard *c, int sx, int sy, struct move_list *l, int loff)
{
	int tmp, color, n = loff;

	color = PIECE_COLOR(get_pos_info(c, sx, sy));

	if (sx != 0) {
		if ((tmp = get_pos_info(c, sx - 1, sy))) {
			if (PIECE_COLOR(tmp) ^ color) {
				set_move(l, loff++, sx, sy, sx - 1, sy);
			}
		} else {
			set_move(l, loff++, sx, sy, sx - 1, sy);
		}
	}

	if (sx != 7) {
		if ((tmp = get_pos_info(c, sx + 1, sy))) {
			if (PIECE_COLOR(tmp) ^ color) {
				set_move(l, loff++, sx, sy, sx + 1, sy);
			}
		} else {
			set_move(l, loff++, sx, sy, sx + 1, sy);
		}
	}

	if (sy != 0) {
		if ((tmp = get_pos_info(c, sx, sy - 1))) {
			if (PIECE_COLOR(tmp) ^ color) {
				set_move(l, loff++, sx, sy, sx, sy - 1);
			}
		} else {
			set_move(l, loff++, sx, sy, sx, sy - 1);
		}
	}

	if (sy != 7) {
		if ((tmp = get_pos_info(c, sx, sy + 1))) {
			if (PIECE_COLOR(tmp) ^ color) {
				set_move(l, loff++, sx, sy, sx, sy + 1);
			}
		} else {
			set_move(l, loff++, sx, sy, sx, sy + 1);
		}
	}

	if (sx != 7 && sy != 7) {
		if ((tmp = get_pos_info(c, sx + 1, sy + 1))) {
			if (PIECE_COLOR(tmp) ^ color) {
				set_move(l, loff++, sx, sy, sx + 1, sy + 1);
			}
		} else {
			set_move(l, loff++, sx, sy, sx + 1, sy + 1);
		}
	}

	if (sx != 7 && sy != 0) {
		if ((tmp = get_pos_info(c, sx + 1, sy - 1))) {
			if (PIECE_COLOR(tmp) ^ color) {
				set_move(l, loff++, sx, sy, sx + 1, sy - 1);
			}
		} else {
			set_move(l, loff++, sx, sy, sx + 1, sy - 1);
		}
	}

	if (sx != 0 && sy != 7) {
		if ((tmp = get_pos_info(c, sx - 1, sy + 1))) {
			if (PIECE_COLOR(tmp) ^ color) {
				set_move(l, loff++, sx, sy, sx - 1, sy + 1);
			}
		} else {
			set_move(l, loff++, sx, sy, sx - 1, sy + 1);
		}
	}

	if (sx != 0 && sy != 0) {
		if ((tmp = get_pos_info(c, sx - 1, sy - 1))) {
			if (PIECE_COLOR(tmp) ^ color) {
				set_move(l, loff++, sx, sy, sx - 1, sy - 1);
			}
		} else {
			set_move(l, loff++, sx, sy, sx - 1, sy - 1);
		}
	}

	return loff - n;
}

static int (*const enum_funcs[8])(struct chessboard *, int, int, struct move_list *, int) = {
										NULL, &enumerate_pawn_moves,
										&enumerate_rook_moves, &enumerate_knight_moves,
										&enumerate_bishop_moves, &enumerate_queen_moves,
										&enumerate_king_moves, NULL};

/* Enumerate all possible moves for a given piece. The piece_number must
 * include the color bit. */
int enumerate_moves(struct chessboard *c, int piece_number, struct move_list *l)
{
	unsigned char x, y;
	unsigned int r;

	get_piece_coordinates(c, piece_number, &x, &y);
	if (x == 15)
		return 0;

	r = get_pos_info(c, x, y);
	return (*enum_funcs[r & 0x7])(c, x, y, l, 0);
}

struct chessboard *copy_board(const struct chessboard *c)
{
	void *ret;
	if (posix_memalign(&ret, 256, sizeof(struct chessboard)))
		fatal("-ENOMEM allocating chessboard struct\n");

	memcpy(ret, c, sizeof(struct chessboard));
	return ret;
}

struct chessboard *get_new_board(void)
{
	return copy_board(&starting_board);
}

/* Always returns a heuristic such that higher is better for white and lower
 * is better for black. */
static const int piece_values[8] = {0, 12, 60, 36, 36, 108, 240, 0};
int calculate_board_heuristic(struct chessboard *c)
{
	unsigned char x, y, tmp;
	int white_h = 0;
	int black_h = 0;

	for (int i = 0; i < 16; i++) {
		get_piece_coordinates(c, i, &x, &y);
		if (x == 15)
			continue;

		tmp = get_pos_info(c, x, y);
		white_h += piece_values[PIECE_TYPE(tmp)];
	}

	for (int i = 16; i < 32; i++) {
		get_piece_coordinates(c, i, &x, &y);
		if (x == 15)
			continue;

		tmp = get_pos_info(c, x, y);
		black_h += piece_values[PIECE_TYPE(tmp)];
	}

	return white_h - black_h;
}

#include "board-asciiart.h"
void print_chessboard(struct chessboard *c)
{
	int r;
	char *tmp, *board = strdup(asciiart_board_skel);

	for (int i = 0, j = 63; i < 64; i++, j = 63 - i) {
		r = c->b[j >> 3][j & 0x7];
		r = (r & 0x7) | ((r & 0x80) >> 4);
		tmp = &board[18 + ((i >> 3) * 18) + (90 * (i >> 3)) + ((7 - (i & 0x7)) * 11) + 1];
		memcpy(tmp, ansi_chess_colors[(r >> 3) & 1], 10);
		tmp[5] = get_character(r);
	}
	printf("%s", board);
	free(board);
}
