#pragma once

struct chessboard;
struct move_list;

struct raw_move {
	unsigned char sx;
	unsigned char sy;
	unsigned char dx;
	unsigned char dy;
	unsigned char sp;
	unsigned char dp;
} __attribute__((aligned(8)));

struct chessboard *copy_board(const struct chessboard *c);
struct chessboard *get_new_board(void);
void execute_raw_move(struct chessboard *c, struct raw_move *m);
void unwind_raw_move(struct chessboard *c, struct raw_move *m);

struct move_list *allocate_move_list(void);
void zero_move_list(struct move_list *l);
void free_move_list(struct move_list *l);

void set_list_parent(struct move_list *l, struct move_list *parent);
struct move_list *get_parent(struct move_list *l);
int get_parent_num(struct move_list *l);
void set_parent_num(struct move_list *l, int num);

int enumerate_moves(struct chessboard *c, int piece_number, struct move_list *l);
void get_move(struct move_list *l, int n, unsigned char *sx, unsigned char *sy, unsigned char *dx, unsigned char *dy);

int calculate_board_heuristic(struct chessboard *c);
int execute_move(struct chessboard *c, int sx, int sy, int dx, int dy);
void print_chessboard(struct chessboard *c);
