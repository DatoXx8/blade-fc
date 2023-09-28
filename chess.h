#ifndef CHESS_H
#define CHESS_H

#include "nn.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include <stdint.h>

#define BOARDTOP 64
#define BOARDBOTTOM -1
#define MAXM 219 
// Not sure wether this only applies to chess positions reachable from the standard starting positition
#define NOSQ 64

#define BITOFF(cp, c) ((cp) ^= (cp) & (1) << (c - 1))
#define BITON(cp, c) ((cp) |= (1) << (c - 1))
#define BITTOGGLE(cp, c) (cp ^= (1) << (c - 1))
#define BITVAL(cp, c) ((cp) &= (1) << (c - 1))

enum color {
    c_b = -1, c_d = 0, c_w = 1, c_e = 2,
};
enum castle {
    wkc = 1, wqc = 2, bkc = 3, bqc = 4, noc = 5
};
enum pieces {
    bk = -6, bq = -5, br = -4, bb = -3, bn = -2, bp = -1, no = 0, wp = 1, wn = 2, wb = 3, wr = 4, wq = 5, wk = 6
};
// #000 = empty
// #001 = pawn
// #010 = knight
// #011 = bishop
// #100 = rook
// #101 = queen
// #110 = king
enum files {
    f_a = 0, f_b = 1, f_c = 2, f_d = 3, f_e = 4, f_f = 5, f_g = 6, f_h = 7
};
enum ranks {
    r_1 = 0, r_2 = 1, r_3 = 2, r_4 = 3, r_5 = 4, r_6 = 5, r_7 = 6, r_8 = 7
};
enum squares_internal {
    i_a1, i_b1, i_c1, i_d1, i_e1, i_f1, i_g1, i_h1,
    i_a2, i_b2, i_c2, i_d2, i_e2, i_f2, i_g2, i_h2,
    i_a3, i_b3, i_c3, i_d3, i_e3, i_f3, i_g3, i_h3,
    i_a4, i_b4, i_c4, i_d4, i_e4, i_f4, i_g4, i_h4,
    i_a5, i_b5, i_c5, i_d5, i_e5, i_f5, i_g5, i_h5,
    i_a6, i_b6, i_c6, i_d6, i_e6, i_f6, i_g6, i_h6,
    i_a7, i_b7, i_c7, i_d7, i_e7, i_f7, i_g7, i_h7,
    i_a8, i_b8, i_c8, i_d8, i_e8, i_f8, i_g8, i_h8,
};
enum squares_actual {
    a_h1, a_g1, a_f1, a_e1, a_d1, a_c1, a_b1, a_a1,
    a_h2, a_g2, a_f2, a_e2, a_d2, a_c2, a_b2, a_a2,
    a_h3, a_g3, a_f3, a_e3, a_d3, a_c3, a_b3, a_a3,
    a_h4, a_g4, a_f4, a_e4, a_d4, a_c4, a_b4, a_a4,
    a_h5, a_g5, a_f5, a_e5, a_d5, a_c5, a_b5, a_a5,
    a_h6, a_g6, a_f6, a_e6, a_d6, a_c6, a_b6, a_a6,
    a_h7, a_g7, a_f7, a_e7, a_d7, a_c7, a_b7, a_a7,
    a_h8, a_g8, a_f8, a_e8, a_d8, a_c8, a_b8, a_a8,
};
typedef struct {
    int64_t from_square;
    int64_t to_square;
    enum pieces captured_piece;
    enum pieces promoted_piece;
    int64_t castle_perm;
    int64_t past_castle_perm;
    int64_t en_passant_sq;
    int64_t past_en_passant_sq;
    int64_t is_en_passant;
    int64_t fifty_move;
    int64_t past_fifty_move;
    enum castle is_castle;
} move_t;
typedef struct {
    // TODO: Bit stuff
    // TODO: Maybe Keep track of squares in check to make legal move generation faster
    enum pieces *square; 
    enum color stm; 
    move_t *movelist; 
    int64_t white_king_sq; 
    int64_t black_king_sq; 
    int64_t en_passant_sq; 
    int64_t fifty_move; 
    int64_t past_moves; 
    int64_t castle_perm;
} board_t;

extern int64_t chess_rank_of(int64_t square); 
extern int64_t chess_file_of(int64_t square); 
extern int64_t chess_piece_value(const char name); 
extern void chess_name_of(int64_t square, char name[3]); 

extern move_t chess_move_alloc(void); 
extern void chess_move_print(move_t move); 
extern inline void chess_move_set(move_t *move, int64_t from_sq, int64_t to_sq, enum pieces captured_piece, enum pieces promoted_piece, int64_t castle_perm, int64_t past_castle_perm, int64_t en_passant_sq, int64_t past_en_passant_sq, bool is_en_passant, int64_t fifty_move, int64_t past_fifty_move, enum castle is_castle); 
extern void chess_move_reset(move_t *move);
extern bool chess_move_is_empty(move_t *move); 

extern int64_t chess_movelist_count(move_t *movelist); 
extern void chess_movelist_reset(move_t *movelist); 
extern void chess_movelist_print(move_t *movelist); 
extern void chess_movelist_dump(move_t *movelist); 

extern board_t chess_board_alloc(void); 
extern void chess_board_read_fen(board_t *board, const char *fen); 
extern void chess_board_print(board_t board); 
extern void chess_make_move(board_t *board, move_t *move); 
extern void chess_undo_move(board_t *board, move_t *move); 
extern bool chess_board_is_check(board_t *board, enum color stm);
extern void chess_board_pseudolegal_moves(board_t *board, move_t *movelist, enum color stm); 
extern void chess_board_legal_moves(board_t *board, move_t *temp, enum color stm); 
extern enum color chess_board_score(board_t *board); 
extern enum color chess_play_random_game(board_t *board, move_t *temp); 

extern int64_t chess_moveindex_from_ai(board_t *board, neuralnet_t *net); // Returns the index of the move the ai wants to play
extern enum color chess_play_ai_game(board_t *board, neuralnet_t *net1, neuralnet_t *net2, move_t *temp); 

#endif
