#include "chess.h"
#include "nn.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

// Just comment out the macros you don't want

// PRINT THE SQUARE NUMBERS WHEN CALLING b_p
//#define CHESS_BOARD_PRINT_DEBUG

// PRINT THE RESULT OF EACH FINISHED GAME IN b_score
#define CHESS_PRINT_GAME_RESULTS

int64_t chess_rank_of(int64_t square) {
    return(square / 8);
}
int64_t chess_file_of(int64_t square) {
    return(square % 8);
}
int64_t chess_piece_value(const char c) {
    switch(c) {
        case 'k':
            return(bk);
        case 'q':
            return(bq);
        case 'r':
            return(br);
        case 'b':
            return(bb);
        case 'n':
            return(bn);
        case 'p':
            return(no);
        case '_':
            return(no);
        case 'P':
            return(wp);
        case 'N':
            return(wn);
        case 'B':
            return(wb);
        case 'R':
            return(wr);
        case 'Q':
            return(wq);
        case 'K':
            return(wk);
        default:
            fprintf(stderr, "ERROR: piece %c is invalid", c);
            exit(1);
    }
}
void chess_name_of(int64_t sq, char n[3]) {
    // The god damned c compiler is so good it optimised my switch case to the new solution... Wow...
    // Old solution for 1_000_000_000 iterations: O0:~1200s O1:~400s O2:~240s O3:~230s
    // New solution for 1_000_000_000 iterations: O0:~360s    O1:~240s O2:~240s O3:~230s
    n[2] = '\0';
    if(sq == NOSQ) {
        n[0] = '_';
        n[1] = '_';
        return;
    }
    assert(sq > BOARDBOTTOM && sq < BOARDTOP);
    n[0] = 'h' - chess_file_of(sq); // Old solution was to use switch cases. Although the speed is the same after optimisations the I still chose to use this one
    n[1] = '1' + chess_rank_of(sq); // because it is so much shorter in terms of implementation.
}
move_t chess_move_alloc(void) {
    move_t move;
    //TODO: Implement somthing like STD_FROM_SQ or STD_FM to make it more understandable
    move.from_square        = NOSQ;
    move.to_square          = NOSQ;
    move.captured_piece     = no;
    move.promoted_piece     = no;
    move.castle_perm        = 0;
    move.past_castle_perm   = 0;
    move.en_passant_sq      = NOSQ;
    move.past_en_passant_sq = NOSQ;
    move.is_en_passant      = false;
    move.fifty_move         = 0;
    move.past_fifty_move    = 0;
    move.is_castle          = noc;
    return(move);
}
void chess_move_print(move_t move) {
    char name[3];
    chess_name_of(move.from_square, name);
    printf("[from_square: %02zu=%s, ", move.from_square, name);
    chess_name_of(move.to_square, name);
    printf("to_square: %02ld=%s, ", move.to_square, name);
    printf("captured_piece: %02d, ", move.captured_piece);
    printf("promoted_piece: %02d, ", move.promoted_piece);
    printf("castle_perm: %02zu, ", move.castle_perm);
    printf("past_castle_perm: %zu, ", move.past_castle_perm);
    printf("is_castle: %d, ", move.is_castle);
    chess_name_of(move.en_passant_sq, name);
    printf("en_passant_sq: %02ld=%s, ", move.en_passant_sq, name);
    printf("past_en_passant_sq: %02ld=%s, ", move.past_en_passant_sq, name);
    printf("is_en_passant: %zu, ", move.is_en_passant);
    printf("fifty_move: %ld, ", move.fifty_move);
    printf("past_fifty_move: %ld]\n", move.past_fifty_move);
}
inline void chess_move_set(move_t *move, int64_t from_sq, int64_t to_sq, enum pieces captured_piece, enum pieces promoted_piece, int64_t castle_perm, int64_t past_castle_perm, int64_t en_passant_sq, int64_t past_en_passant_sq, bool is_en_passant, int64_t fifty_move, int64_t past_fifty_move, enum castle is_castle) {
    move->from_square        = from_sq;
    move->to_square          = to_sq;
    move->captured_piece     = captured_piece;
    move->promoted_piece     = promoted_piece;
    move->castle_perm        = castle_perm;
    move->past_castle_perm   = past_castle_perm;
    move->en_passant_sq      = en_passant_sq;
    move->past_en_passant_sq = past_en_passant_sq;
    move->is_en_passant      = is_en_passant;
    move->fifty_move         = fifty_move;
    move->past_fifty_move    = past_fifty_move;
    move->is_castle          = is_castle;
}
bool chess_move_is_empty(move_t *move) {
    // TODO: Check if there is a way to compare the memory directly as that may improve performance significantly
    if(move->from_square        != NOSQ)  {return(false);} 
    if(move->to_square          != NOSQ)  {return(false);} 
    if(move->captured_piece     != no)    {return(false);}
    if(move->promoted_piece     != no)    {return(false);}
    if(move->castle_perm        != 0)     {return(false);}
    if(move->past_castle_perm   != 0)     {return(false);}
    if(move->en_passant_sq      != NOSQ)  {return(false);}
    if(move->past_en_passant_sq != NOSQ)  {return(false);}
    if(move->is_en_passant      != false) {return(false);}
    if(move->fifty_move         != 0)     {return(false);}
    if(move->past_fifty_move    != 0)     {return(false);}
    if(move->is_castle          != noc)   {return(false);}
    return(true);
}
void chess_move_reset(move_t *move) {
    move->from_square        = NOSQ;
    move->to_square          = NOSQ;
    move->captured_piece     = no;
    move->promoted_piece     = no;
    move->castle_perm        = 0;
    move->past_castle_perm   = 0;
    move->en_passant_sq      = NOSQ;
    move->past_en_passant_sq = NOSQ;
    move->is_en_passant      = false;
    move->fifty_move         = 0;
    move->past_fifty_move    = 0;
    move->is_castle          = noc;
}
int64_t chess_movelist_count(move_t *movelist) {
    int64_t i = 0;
    while(i < MAXM && !(chess_move_is_empty(&movelist[i]))) {
        i++;
    }
    return(i);
}
void chess_movelist_reset(move_t *movelist) {
    for(int64_t i = 0; i < MAXM; i++) {
        chess_move_reset(&movelist[i]);
    }
}
void chess_movelist_print(move_t *movelist) {
    int64_t c = chess_movelist_count(movelist);
    for(int64_t i = 0; i < c; i++) {
        chess_move_print(movelist[i]);
    }
}
void chess_movelist_dump(move_t *movelist) {
    for(int64_t i = 0; i < MAXM; i++) {
        chess_move_print(movelist[i]);
    }
}
board_t chess_board_alloc(void) {
    board_t board;
    board.square = malloc(sizeof(*board.square) *(int64_t)BOARDTOP);
    board.movelist = malloc(sizeof(*board.movelist) * MAXM);

    assert(board.square != NULL);
    assert(board.movelist != NULL);

    for(int64_t i = 0; i < MAXM; i++) {
        board.movelist[i] = chess_move_alloc();
    }
    board.white_king_sq = NOSQ;
    board.black_king_sq = NOSQ;
    board.en_passant_sq    = NOSQ;
    board.fifty_move    = 0;
    board.past_moves    = 0;
    board.castle_perm    = 0;
    return(board);
}
void chess_board_read_fen(board_t *board, const char *fen) {
    for(int64_t i = 0; i <(int64_t)BOARDTOP; i++) {
        board->square[i] = no;
    }
    board->white_king_sq = NOSQ;
    board->black_king_sq = NOSQ;
    board->en_passant_sq = NOSQ;
    board->fifty_move = 0;
    board->past_moves = 0;
    board->castle_perm = 0;
    int64_t bi = BOARDTOP - 1;
    while(*fen != ' ') {
        switch(*fen) {
            case 'k': /*printf("k\n")*/;
                board->square[bi] = bk;
                board->black_king_sq = bi;
                bi--;
                fen++;
                continue;
            case 'q': /*printf("q\n")*/;
                board->square[bi] = bq;
                bi--;
                fen++;
                continue;
            case 'r': /*printf("r\n")*/;
                board->square[bi] = br;
                bi--;
                fen++;
                continue;
            case 'b': /*printf("b\n")*/;
                board->square[bi] = bb;
                bi--;
                fen++;
                continue;
            case 'n': /*printf("n\n")*/;
                board->square[bi] = bn;
                bi--;
                fen++;
                continue;
            case 'p': /*printf("p\n")*/;
                board->square[bi] = bp;
                bi--;
                fen++;
                continue;
            case 'P': /*printf("p\n")*/;
                board->square[bi] = wp;
                bi--;
                fen++;
                continue;
            case 'N': /*printf("n\n")*/;
                board->square[bi] = wn;
                bi--;
                fen++;
                continue;
            case 'B': /*printf("b\n")*/;
                board->square[bi] = wb;
                bi--;
                fen++;
                continue;
            case 'R': /*printf("r\n")*/;
                board->square[bi] = wr;
                bi--;
                fen++;
                continue;
            case 'Q': /*printf("q\n")*/;
                board->square[bi] = wq;
                bi--;
                fen++;
                continue;
            case 'K': /*printf("k\n")*/;
                board->square[bi] = wk;
                board->white_king_sq = bi;
                bi--;
                fen++;
                continue;
            case '1': /*printf("1\n")*/;
                bi -= 1;
                fen++;
                continue;
            case '2': /*printf("2\n")*/;
                bi -= 2;
                fen++;
                continue;
            case '3': /*printf("3\n")*/;
                bi -= 3;
                fen++;
                continue;
            case '4': /*printf("4\n")*/;
                bi -= 4;
                fen++;
                continue;
            case '5': /*printf("5\n")*/;
                bi -= 5;
                fen++;
                continue;
            case '6': /*printf("6\n")*/;
                bi -= 6;
                fen++;
                continue;
            case '7': /*printf("7\n")*/;
                bi -= 7;
                fen++;
                continue;
            case '8': /*printf("8\n")*/;
                bi -= 8;
                fen++;
                continue;
            case '/': /*printf("/\n")*/;
                fen++;
                continue;
            default:
                fprintf(stderr, "ERROR: Piece %c is invalid", *fen);
                exit(1);
        }
        assert(bi == 0);
    }
    fen += 1;
    if(*fen == 'w') {
        board->stm = c_w;
    } else if(*fen == 'b') {
        board->stm = c_b;
    } else {
        fprintf(stderr, "ERROR : Side to move %c is invalid", *fen);
        exit(1);
    }
    fen += 2;
    while(*fen != ' ') {
        switch(*fen) {
            case 'K': {
                board->castle_perm += 1;
                break;
            }
            case 'Q': {
                board->castle_perm += 2;
                break;
            }
            case 'k': {
                board->castle_perm += 4;
                break;
            }
            case 'q': {
                board->castle_perm += 8;
                break;
            }
            default: {
                fprintf(stderr, "ERROR: character %c is invalid. expected K,Q,k,q", *fen);
                exit(1);
            }
        }
        fen++;
    }
    fen++;
    int64_t temp = 0;
    int64_t file = 8;
    int64_t rank = 8;
    while(*(fen + temp) != ' ') {
        if(*(fen + temp) != '-') {
            if(temp == 0) {
                assert(*(fen + temp) >= 'a');
                file = *(fen + temp) - 'a';
                printf("file: %ld\n", file);
            } else if(temp == 1) {
                assert(*(fen + temp) <= '8');
                rank = *(fen + temp) - '1';
                printf("rank: %ld\n", rank);
            } else {
                fprintf(stderr, "ERROR: invalid en passant");
                exit(1);
            }
        }
        temp++;
    }
    if((file < 8) ||(rank < 8)) {
        assert((file < 8) &&(rank < 8)); // If either the rank or file is less than 8 then the other has to be too
    }
    if(file < 8 && rank < 8) {
        board->en_passant_sq = file + 8 * rank;
    } else {
        board->en_passant_sq = NOSQ;
    }
    fen += temp + 1;
    while(*fen != ' ') {
        board->fifty_move *= 10;
        board->fifty_move += *fen - '0';
        assert(*fen >= '0' && *fen <= '9');
        fen++;
    }
    fen++;
    while(*fen != '\0') {
        board->past_moves *= 10;
        board->past_moves += *fen - '0';
        assert(*fen >= '0' && *fen <= '9');
        fen++;
    }
}
void chess_board_print(board_t board) {
    char name[3];
    const char piece_names[] = "kqrbnp_PNBRQK";
#ifdef CHESS_BOARD_PRINT_DEBUG
    printf("     h    g    f    e    d    c    b    a\n");
#else
    printf("  h g f e d c b a\n");
#endif
    for(int64_t i = 0; i < 8; i++) {
        printf("%lu ", i + 1);
        for(int64_t j = 0; j < 8; j++) {
#ifdef CHESS_BOARD_PRINT_DEBUG
            printf("%02lu ", i * 8 + j);
#endif
            printf("%c ", piece_names[board.square[i * 8 + j] + 6]);
        }
        printf("\n");
    }
    printf("INFO: side to move     : %1d\n", board.stm);
    chess_name_of(board.white_king_sq, name);
    printf("INFO: white king square: %lu=%s\n", board.white_king_sq, name);
    chess_name_of(board.black_king_sq, name);
    printf("INFO: black king square: %lu=%s\n", board.black_king_sq, name);
    chess_name_of(board.en_passant_sq, name);
    printf("INFO: en passant square: %lu=%s\n", board.en_passant_sq, name);
    printf("INFO: fifty move count : %lu\n", board.fifty_move);
    printf("INFO: past move count  : %lu\n", board.past_moves);
    printf("INFO: castling perms   : %lu\n", board.castle_perm);
}
void chess_make_move(board_t *board, move_t *move) {
    if(move->from_square <= BOARDBOTTOM || move->from_square >= BOARDTOP) {
        fprintf(stderr, "ERROR: from_sq %lu\n", move->from_square);
        exit(1);
    }
    if(move->to_square <= BOARDBOTTOM || move->to_square >= BOARDTOP) {
        fprintf(stderr, "ERROR: to_sq %lu\n", move->to_square);
        exit(1);
    }
    if(board->square[move->from_square] == wk) {
        board->white_king_sq = move->to_square;
    }
    if(board->square[move->from_square] == bk) {
        board->black_king_sq = move->to_square;
    }
    if(move->promoted_piece == no) {
        board->square[move->to_square] = board->square[move->from_square];
        board->square[move->from_square] = no;
    } else {
        board->square[move->to_square] = move->promoted_piece;
        board->square[move->from_square] = no;
    }
    board->castle_perm = move->castle_perm;
    board->fifty_move = move->fifty_move;
    board->past_moves++;
}
void chess_undo_move(board_t *board, move_t *move) {
    if(move->from_square < BOARDBOTTOM || move->from_square >= BOARDTOP) {
        printf("from_sq: %lu\n", move->from_square);
        exit(1);
    }
    if(move->to_square < BOARDBOTTOM || move->to_square >= BOARDTOP) {
        printf("to_sq: %lu\n", move->to_square);
        exit(2);
    }
    if(board->square[move->to_square] == wk) {
        board->white_king_sq = move->from_square;
    }
    if(board->square[move->to_square] == bk) {
        board->black_king_sq = move->from_square;
    }
    if(move->promoted_piece == no) {
        board->square[move->from_square] = board->square[move->to_square];
        board->square[move->to_square] = move->captured_piece;
    } else {
        board->square[move->from_square] =(int64_t) board->stm; // Kind of hacky but it works cuz the pawn values are -1 and 1 which are also the values for the sides
    }
    board->castle_perm = move->past_castle_perm;
    board->fifty_move = move->past_fifty_move;
    board->past_moves--;
}
// TODO: Implement this when moving the board to bits instead of single values
//void Q_chess_board_pseudolegal_moves(board_t *board, move_t *movelist, enum color stm) {
//    chess_movelist_reset(movelist);
//    int64_t sq = 0;
//}
void chess_board_pseudolegal_moves(board_t *board, move_t *movelist, enum color stm) {
    // This does about ~3e6 per second
    chess_movelist_reset(movelist);
    int64_t sq = 0;
    int64_t mi = 0;
    int64_t off= 0;
    int64_t dir= 0;
    int64_t t_cp = 0;
    while(sq < NOSQ) {
        int64_t piece = board->square[sq];
        switch(piece) {
            case no: {
                break;
            }
            case wp: {
                if(stm == c_b) {
                    break;
                }
                if(chess_rank_of(sq) < r_7) {
                    if(board->square[sq + 8] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + 8, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        if(chess_rank_of(sq) == r_2 && board->square[sq + 16] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 16, no, no, t_cp, board->castle_perm, sq + 8, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_file_of(sq) != f_a) {
                        if(board->square[sq + 7] < no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 7, board->square[sq + 7], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_file_of(sq) != f_h) {
                        if(board->square[sq + 9] < no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 9, board->square[sq + 9], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                } else if(chess_rank_of(sq) == r_7) {
                    if(board->square[sq + 8] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + 8, no, wq, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        chess_move_set(&movelist[mi], sq, sq + 8, no, wn, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        chess_move_set(&movelist[mi], sq, sq + 8, no, wb, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        chess_move_set(&movelist[mi], sq, sq + 8, no, wr, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                    }
                    if(chess_file_of(sq) != f_a) {
                        if(board->square[sq + 7] < no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 7, board->square[sq + 7], wq, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                            chess_move_set(&movelist[mi], sq, sq + 7, board->square[sq + 7], wn, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                            chess_move_set(&movelist[mi], sq, sq + 7, board->square[sq + 7], wb, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                            chess_move_set(&movelist[mi], sq, sq + 7, board->square[sq + 7], wr, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_file_of(sq) != f_h) {
                        if(board->square[sq + 9] < no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 9, board->square[sq + 9], wq, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                            chess_move_set(&movelist[mi], sq, sq + 9, board->square[sq + 9], wn, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                            chess_move_set(&movelist[mi], sq, sq + 9, board->square[sq + 9], wb, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                            chess_move_set(&movelist[mi], sq, sq + 9, board->square[sq + 9], wr, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                }
                break;
            }
            case bp: {
                if(stm == c_w) {
                    break;
                }
                if(chess_rank_of(sq) > r_2) {
                    if(board->square[sq - 8] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq - 8, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        if(chess_rank_of(sq) == r_7 && board->square[sq - 16] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 16, no, no, t_cp, board->castle_perm, sq - 8, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_file_of(sq) != f_a) {
                        if(board->square[sq - 9] > no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 9, board->square[sq - 9], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_file_of(sq) != f_h) {
                        if(board->square[sq - 7] > no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 7, board->square[sq - 7], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                } else if(chess_rank_of(sq) == r_2) {
                    if(board->square[sq - 8] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq - 8, no, bq, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        chess_move_set(&movelist[mi], sq, sq - 8, no, bn, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        chess_move_set(&movelist[mi], sq, sq - 8, no, bb, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        chess_move_set(&movelist[mi], sq, sq - 8, no, br, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                    }
                    if(chess_file_of(sq) != f_a) {
                        if(board->square[sq - 9] > no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 9, board->square[sq - 9], bq, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                            chess_move_set(&movelist[mi], sq, sq - 9, board->square[sq - 9], bn, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                            chess_move_set(&movelist[mi], sq, sq - 9, board->square[sq - 9], bb, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                            chess_move_set(&movelist[mi], sq, sq - 9, board->square[sq - 9], br, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_file_of(sq) != f_h) {
                        if(board->square[sq - 7] > no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 7, board->square[sq - 7], bq, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                            chess_move_set(&movelist[mi], sq, sq - 7, board->square[sq - 7], bn, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                            chess_move_set(&movelist[mi], sq, sq - 7, board->square[sq - 7], bb, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                            chess_move_set(&movelist[mi], sq, sq - 7, board->square[sq - 7], br, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                }
                break;
            }
            case wn: {
                if(stm == c_b) {
                    break;
                }
                if(chess_file_of(sq) > f_a) {
                    if(chess_rank_of(sq) > r_2) {
                        if(board->square[sq - 17] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 17, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq - 17] < no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 17, board->square[sq - 17], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_rank_of(sq) < r_7) {
                        if(board->square[sq + 15] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 15, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq + 15] < no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 15, board->square[sq + 15], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                }
                if(chess_file_of(sq) > f_b) {
                    if(chess_rank_of(sq) > r_1) {
                        if(board->square[sq - 10] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 10, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq - 10] < no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 10, board->square[sq - 10], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_rank_of(sq) < r_8) {
                        if(board->square[sq + 6] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 6, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq + 6] < no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 6, board->square[sq + 6], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                }
                if(chess_file_of(sq) < f_g) {
                    if(chess_rank_of(sq) > r_1) {
                        if(board->square[sq - 6] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 6, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq - 6] < no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 6, board->square[sq - 6], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_rank_of(sq) < r_8) {
                        if(board->square[sq + 10] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 10, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq + 10] < no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 10, board->square[sq + 10], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                }
                if(chess_file_of(sq) < f_h) {
                    if(chess_rank_of(sq) > r_2) {
                        if(board->square[sq - 15] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 15, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq - 15] < no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 15, board->square[sq - 15], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_rank_of(sq) < r_7) {
                        if(board->square[sq + 17] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 17, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq + 17] < no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 17, board->square[sq + 17], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                }
                break;
            }
            case bn: {
                if(stm == c_w) {
                    break;
                }
                if(chess_file_of(sq) > f_a) {
                    if(chess_rank_of(sq) > r_2) {
                        if(board->square[sq - 17] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 17, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq - 17] > no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 17, board->square[sq - 17], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_rank_of(sq) < r_7) {
                        if(board->square[sq + 15] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 15, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq + 15] > no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 15, board->square[sq + 15], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                }
                if(chess_file_of(sq) > f_b) {
                    if(chess_rank_of(sq) > r_1) {
                        if(board->square[sq - 10] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 10, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq - 10] > no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 10, board->square[sq - 10], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_rank_of(sq) < r_8) {
                        if(board->square[sq + 6] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 6, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq + 6] > no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 6, board->square[sq + 6], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                }
                if(chess_file_of(sq) < f_g) {
                    if(chess_rank_of(sq) > r_1) {
                        if(board->square[sq - 6] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 6, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq - 6] > no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 6, board->square[sq - 6], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_rank_of(sq) < r_8) {
                        if(board->square[sq + 10] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 10, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq + 10] > no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 10, board->square[sq + 10], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                }
                if(chess_file_of(sq) < f_h) {
                    if(chess_rank_of(sq) > r_2) {
                        if(board->square[sq - 15] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 15, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq - 15] > no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq - 15, board->square[sq - 15], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                    if(chess_rank_of(sq) < r_7) {
                        if(board->square[sq + 17] == no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 17, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq + 17] > no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 17, board->square[sq + 17], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                }
                break;
            }
            case wb: {
                if(stm == c_b) {
                    break;
                }
                off = 9;
                dir = 9;
                while(sq + off < BOARDTOP) {
                    if(chess_file_of(sq + off) == f_a) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = 7;
                dir = 7;
                while(sq + off < BOARDTOP) {
                    if(chess_file_of(sq + off) == f_h) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -7;
                dir = -7;
                while(sq + off > BOARDBOTTOM) {
                    if(chess_file_of(sq + off) == f_a) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -9;
                dir = -9;
                while(sq + off > BOARDBOTTOM) {
                    if(chess_file_of(sq + off) == f_h) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                break;
            }
            case bb: {
                if(stm == c_w) {
                    break;
                }
                off = 9;
                dir = 9;
                while(sq + off < BOARDTOP) {
                    if(chess_file_of(sq + off) == f_a) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = 7;
                dir = 7;
                while(sq + off < BOARDTOP) {
                    if(chess_file_of(sq + off) == f_h) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -7;
                dir = -7;
                while(sq + off > BOARDBOTTOM) {
                    if(chess_file_of(sq + off) == f_a) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -9;
                dir = -9;
                while(sq + off > BOARDBOTTOM) {
                    if(chess_file_of(sq + off) == f_h) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                break;
            }
            case wr: {
                if(stm == c_b) {
                    break;
                }
                off = 8;
                dir = 8;
                while(sq + off < BOARDTOP) {
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = 1;
                dir = 1;
                while(sq + off < BOARDTOP) {
                    if(chess_file_of(sq + off) == f_a) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -1;
                dir = -1;
                while(sq + off > BOARDBOTTOM) {
                    if(chess_file_of(sq + off) == f_h) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -8;
                dir = -8;
                while(sq + off > BOARDBOTTOM) {
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                break;
            }
            case br: {
                if(stm == c_w) {
                    break;
                }
                off = 8;
                dir = 8;
                while(sq + off < BOARDTOP) {
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = 1;
                dir = 1;
                while(sq + off < BOARDTOP) {
                    if(chess_file_of(sq + off) == f_a) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -1;
                dir = -1;
                while(sq + off > BOARDBOTTOM) {
                    if(chess_file_of(sq + off) == f_h) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -8;
                dir = -8;
                while(sq + off > BOARDBOTTOM) {
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                break;
            }
            case wq: {
                if(stm == c_b) {
                    break;
                }
                off = 9;
                dir = 9;
                while(sq + off < BOARDTOP) {
                    if(chess_file_of(sq + off) == f_a) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = 7;
                dir = 7;
                while(sq + off < BOARDTOP) {
                    if(chess_file_of(sq + off) == f_h) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -7;
                dir = -7;
                while(sq + off > BOARDBOTTOM) {
                    if(chess_file_of(sq + off) == f_a) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -9;
                dir = -9;
                while(sq + off > BOARDBOTTOM) {
                    if(chess_file_of(sq + off) == f_h) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = 8;
                dir = 8;
                while(sq + off < BOARDTOP) {
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = 1;
                dir = 1;
                while(sq + off < BOARDTOP) {
                    if(chess_file_of(sq + off) == f_a) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -1;
                dir = -1;
                while(sq + off > BOARDBOTTOM) {
                    if(chess_file_of(sq + off) == f_h) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -8;
                dir = -8;
                while(sq + off > BOARDBOTTOM) {
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] < no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                break;
            }
            case bq: {
                if(stm == c_w) {
                    break;
                }
                off = 9;
                dir = 9;
                while(sq + off < BOARDTOP) {
                    if(chess_file_of(sq + off) == f_a) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = 7;
                dir = 7;
                while(sq + off < BOARDTOP) {
                    if(chess_file_of(sq + off) == f_h) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -7;
                dir = -7;
                while(sq + off > BOARDBOTTOM) {
                    if(chess_file_of(sq + off) == f_a) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -9;
                dir = -9;
                while(sq + off > BOARDBOTTOM) {
                    if(chess_file_of(sq + off) == f_h) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = 8;
                dir = 8;
                while(sq + off < BOARDTOP) {
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = 1;
                dir = 1;
                while(sq + off < BOARDTOP) {
                    if(chess_file_of(sq + off) == f_a) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -1;
                dir = -1;
                while(sq + off > BOARDBOTTOM) {
                    if(chess_file_of(sq + off) == f_h) {
                        break;
                    }
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                off = -8;
                dir = -8;
                while(sq + off > BOARDBOTTOM) {
                    if(board->square[sq + off] == no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + off] > no) {
                        t_cp = board->castle_perm;
                        chess_move_set(&movelist[mi], sq, sq + off, board->square[sq + off], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                        break;
                    } else {
                        break;
                    }
                    off += dir;
                }
                break;
            }
            case wk: {
                if(stm == c_b) {
                    break;
                }
                if(chess_rank_of(sq) > r_1) {
                    if(board->square[sq - 8] == no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, wkc);
                        BITOFF(t_cp, wqc);
                        chess_move_set(&movelist[mi], sq, sq - 8, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq - 8] < no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, wkc);
                        BITOFF(t_cp, wqc);
                        chess_move_set(&movelist[mi], sq, sq - 8, board->square[sq - 8], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                    }
                }
                if(chess_rank_of(sq) < r_8) {
                    if(board->square[sq + 8] == no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, wkc);
                        BITOFF(t_cp, wqc);
                        chess_move_set(&movelist[mi], sq, sq + 8, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + 8] < no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, wkc);
                        BITOFF(t_cp, wqc);
                        chess_move_set(&movelist[mi], sq, sq + 8, board->square[sq + 8], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                    }
                }
                if(chess_file_of(sq) > f_a) {
                    if(board->square[sq - 1] == no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, wkc);
                        BITOFF(t_cp, wqc);
                        chess_move_set(&movelist[mi], sq, sq - 1, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq - 1] < no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, wkc);
                        BITOFF(t_cp, wqc);
                        chess_move_set(&movelist[mi], sq, sq - 1, board->square[sq - 1], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                    }
                    if(chess_rank_of(sq) > r_1) {
                        if(board->square[sq - 9] == no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, wkc);
                            BITOFF(t_cp, wqc);
                            chess_move_set(&movelist[mi], sq, sq - 9, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq - 9] < no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, wkc);
                            BITOFF(t_cp, wqc);
                            chess_move_set(&movelist[mi], sq, sq - 9, board->square[sq - 9], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    } else if(chess_rank_of(sq) < r_8) {
                        if(board->square[sq + 7] == no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, wkc);
                            BITOFF(t_cp, wqc);
                            chess_move_set(&movelist[mi], sq, sq + 7, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq + 7] < no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, wkc);
                            BITOFF(t_cp, wqc);
                            chess_move_set(&movelist[mi], sq, sq + 7, board->square[sq + 7], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    } 
                }
                if(chess_file_of(sq) < f_h) {
                    if(board->square[sq + 1] == no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, wkc);
                        BITOFF(t_cp, wqc);
                        chess_move_set(&movelist[mi], sq, sq + 1, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + 1] < no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, wkc);
                        BITOFF(t_cp, wqc);
                        chess_move_set(&movelist[mi], sq, sq + 1, board->square[sq + 1], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                    }
                    if(chess_rank_of(sq) > r_1) {
                        if(board->square[sq - 7] == no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, wkc);
                            BITOFF(t_cp, wqc);
                            chess_move_set(&movelist[mi], sq, sq - 7, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq - 7] < no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, wkc);
                            BITOFF(t_cp, wqc);
                            chess_move_set(&movelist[mi], sq, sq - 7, board->square[sq - 7], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    } else if(chess_rank_of(sq) < r_8) {
                        if(board->square[sq + 9] == no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, wkc);
                            BITOFF(t_cp, wqc);
                            chess_move_set(&movelist[mi], sq, sq + 9, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq + 9] < no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, wkc);
                            BITOFF(t_cp, wqc);
                            chess_move_set(&movelist[mi], sq, sq + 9, board->square[sq + 9], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    } 
                }
                break;
            }
            case bk: {
                if(stm == c_w) {
                    break;
                }
                if(chess_rank_of(sq) > r_1) {
                    if(board->square[sq - 8] == no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, bkc);
                        BITOFF(t_cp, bqc);
                        chess_move_set(&movelist[mi], sq, sq - 8, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq - 8] > no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, bkc);
                        BITOFF(t_cp, bqc);
                        chess_move_set(&movelist[mi], sq, sq - 8, board->square[sq - 8], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                    }
                }
                if(chess_rank_of(sq) < r_8) {
                    if(board->square[sq + 8] == no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, bkc);
                        BITOFF(t_cp, bqc);
                        chess_move_set(&movelist[mi], sq, sq + 8, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + 8] > no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, bkc);
                        BITOFF(t_cp, bqc);
                        chess_move_set(&movelist[mi], sq, sq + 8, board->square[sq + 8], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                    }
                }
                if(chess_file_of(sq) > f_a) {
                    if(board->square[sq - 1] == no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, bkc);
                        BITOFF(t_cp, bqc);
                        chess_move_set(&movelist[mi], sq, sq - 1, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq - 1] > no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, bkc);
                        BITOFF(t_cp, bqc);
                        chess_move_set(&movelist[mi], sq, sq - 1, board->square[sq - 1], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                    }
                    if(chess_rank_of(sq) > r_1) {
                        if(board->square[sq - 9] == no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, bkc);
                            BITOFF(t_cp, bqc);
                            chess_move_set(&movelist[mi], sq, sq - 9, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq - 9] > no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, bkc);
                            BITOFF(t_cp, bqc);
                            chess_move_set(&movelist[mi], sq, sq - 9, board->square[sq - 9], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    } else if(chess_rank_of(sq) < r_8) {
                        if(board->square[sq + 7] == no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, bkc);
                            BITOFF(t_cp, bqc);
                            chess_move_set(&movelist[mi], sq, sq + 7, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq + 7] > no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, bkc);
                            BITOFF(t_cp, bqc);
                            chess_move_set(&movelist[mi], sq, sq + 7, board->square[sq + 7], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    } 
                }
                if(chess_file_of(sq) < f_h) {
                    if(board->square[sq + 1] == no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, bkc);
                        BITOFF(t_cp, bqc);
                        chess_move_set(&movelist[mi], sq, sq + 1, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                        mi++;
                    } else if(board->square[sq + 1] > no) {
                        t_cp = board->castle_perm;
                        BITOFF(t_cp, bkc);
                        BITOFF(t_cp, bqc);
                        chess_move_set(&movelist[mi], sq, sq + 1, board->square[sq + 1], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                        mi++;
                    }
                    if(chess_rank_of(sq) > r_1) {
                        if(board->square[sq - 7] == no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, bkc);
                            BITOFF(t_cp, bqc);
                            chess_move_set(&movelist[mi], sq, sq - 7, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq - 7] > no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, bkc);
                            BITOFF(t_cp, bqc);
                            chess_move_set(&movelist[mi], sq, sq - 7, board->square[sq - 7], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    } else if(chess_rank_of(sq) < r_8) {
                        if(board->square[sq + 9] == no) {
                            t_cp = board->castle_perm;
                            BITOFF(t_cp, bkc);
                            BITOFF(t_cp, bqc);
                            chess_move_set(&movelist[mi], sq, sq + 9, no, no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, board->fifty_move + 1, board->fifty_move, noc);
                            mi++;
                        } else if(board->square[sq + 9] > no) {
                            t_cp = board->castle_perm;
                            chess_move_set(&movelist[mi], sq, sq + 9, board->square[sq + 9], no, t_cp, board->castle_perm, NOSQ, board->en_passant_sq, false, 0, board->fifty_move, noc);
                            mi++;
                        }
                    }
                }
                break;
            }
            default: {
                fprintf(stderr, "ERROR: Expected -6 <= i <= 6, but got i = %zu at sq = %lu\n", piece, sq);
                exit(1);
            }
        }
        sq++;
    }
}
bool chess_board_is_check(board_t *board, enum color stm) {
    int64_t r, f, sq;
    int64_t off = 0;
    int64_t dir = 0;
    if(stm == c_w) {
        sq = board->white_king_sq;
        r = chess_rank_of(sq);
        f = chess_file_of(sq);
        dir = 1;
        off = dir;
        while(sq + off < BOARDTOP && chess_file_of(sq + off) != f_a) {
            if((board->square[sq + off] == br) || (board->square[sq + off] == bq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir = 8;
        off = dir;
        while(sq + off < BOARDTOP) {
            if((board->square[sq + off] == br) || (board->square[sq + off] == bq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir =-1;
        off = dir;
        while(sq + off > BOARDBOTTOM && chess_file_of(sq + off) != f_h) {
            if((board->square[sq + off] == br) || (board->square[sq + off] == bq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir = -8;
        off = dir;
        while(sq + off > BOARDBOTTOM) {
            if((board->square[sq + off] == br) || (board->square[sq + off] == bq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir = 9;
        off = dir;
        while(sq + off < BOARDTOP && chess_file_of(sq + off) != f_a) {
            if((board->square[sq + off] == bb) || (board->square[sq + off] == bq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir = 7;
        off = dir;
        while(sq + off < BOARDTOP && chess_file_of(sq + off) != f_h) {
            if((board->square[sq + off] == bb) || (board->square[sq + off] == bq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir =-9;
        off = dir;
        while(sq + off > BOARDBOTTOM && chess_file_of(sq + off) != f_h) {
            if((board->square[sq + off] == bb) || (board->square[sq + off] == bq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir =-7;
        off = dir;
        while(sq + off > BOARDBOTTOM && chess_file_of(sq + off) != f_a) {
            if((board->square[sq + off] == bb) || (board->square[sq + off] == bq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        if(r > r_1) {
            if(board->square[sq - 8] == bk) {return(true);}
        }
        if(r < r_8) {
            if(board->square[sq + 8] == bk) {return(true);}
        }
        if(f > f_a) {
            if(board->square[sq - 1] == bk) {return(true);}
            if(board->square[sq + 7] == bp) {return(true);}
            if(r > r_1) {
                if(board->square[sq - 9] == bk) {return(true);}
            }
            if(r < r_8) {
                if(board->square[sq + 7] == bk) {return(true);}
            }
            if(r > r_2) {
                if(board->square[sq - 17] == bn) {return(true);}
            }
            if(r < r_7) {
                if(board->square[sq + 15] == bn) {return(true);}
            }
        }
        if(f < f_h) {
            if(board->square[sq + 1] == bk) {return(true);}
            if(board->square[sq + 9] == bp) {return(true);}
            if(r > r_1) {
                if(board->square[sq - 7] == bk) {return(true);}
            }
            if(r < r_8) {
                if(board->square[sq + 9] == bk) {return(true);}
            }
            if(r > r_2) {
                if(board->square[sq - 15] == bn) {return(true);}
            }
            if(r < r_7) {
                if(board->square[sq + 17] == bn) {return(true);}
            }
        }
        if(f > f_b) {
            if(r > r_1) {
                if(board->square[sq - 10] == bn) {return(true);}
            }
            if(r < r_8) {
                if(board->square[sq + 6] == bn) {return(true);}
            }
        }
        if(f < f_g) {
            if(r > r_1) {
                if(board->square[sq - 6] == bn) {return(true);}
            }
            if(r < r_8) {
                if(board->square[sq + 10] == bn) {return(true);}
            }
        }
    } else if(stm == c_b) {
        sq = board->black_king_sq;
        r = chess_rank_of(sq);
        f = chess_file_of(sq);
        dir = 1;
        off = dir;
        while(sq + off < BOARDTOP && chess_file_of(sq + off) != f_a) {
            if((board->square[sq + off] == wr) || (board->square[sq + off] == wq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir = 8;
        off = dir;
        while(sq + off < BOARDTOP) {
            if((board->square[sq + off] == wr) || (board->square[sq + off] == wq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir =-1;
        off = dir;
        while(sq + off > BOARDBOTTOM && chess_file_of(sq + off) != f_h) {
            if((board->square[sq + off] == wr) || (board->square[sq + off] == wq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir = -8;
        off = dir;
        while(sq + off > BOARDBOTTOM) {
            if((board->square[sq + off] == wr) || (board->square[sq + off] == wq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir = 9;
        off = dir;
        while(sq + off < BOARDTOP && chess_file_of(sq + off) != f_a) {
            if((board->square[sq + off] == wb) || (board->square[sq + off] == wq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir = 7;
        off = dir;
        while(sq + off < BOARDTOP && chess_file_of(sq + off) != f_h) {
            if((board->square[sq + off] == wb) || (board->square[sq + off] == wq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir =-9;
        off = dir;
        while(sq + off > BOARDBOTTOM && chess_file_of(sq + off) != f_h) {
            if((board->square[sq + off] == wb) || (board->square[sq + off] == wq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        dir =-7;
        off = dir;
        while(sq + off > BOARDBOTTOM && chess_file_of(sq + off) != f_a) {
            if((board->square[sq + off] == wb) || (board->square[sq + off] == wq)) {
                return(true);
            } else if(board->square[sq + off] == no) {
                off += dir;
                continue;
            } else {
                break;
            }
            off += dir;
        }
        if(r > r_1) {
            if(board->square[sq - 8] == wk) {return(true);}
        }
        if(r < r_8) {
            if(board->square[sq + 8] == wk) {return(true);}
        }
        if(f > f_a) {
            if(board->square[sq - 1] == wk) {return(true);}
            if(board->square[sq - 9] == wp) {return(true);}
            if(r > r_1) {
                if(board->square[sq - 9] == wk) {return(true);}
            }
            if(r < r_8) {
                if(board->square[sq + 7] == wk) {return(true);}
            }
            if(r > r_2) {
                if(board->square[sq - 17] == wn) {return(true);}
            }
            if(r < r_7) {
                if(board->square[sq + 15] == wn) {return(true);}
            }
        }
        if(f < f_h) {
            if(board->square[sq + 1] == wk) {return(true);}
            if(board->square[sq - 7] == wp) {return(true);}
            if(r > r_1) {
                if(board->square[sq - 7] == wk) {return(true);}
            }
            if(r < r_8) {
                if(board->square[sq + 9] == wk) {return(true);}
            }
            if(r > r_2) {
                if(board->square[sq - 15] == wn) {return(true);}
            }
            if(r < r_7) {
                if(board->square[sq + 17] == wn) {return(true);}
            }
        }
        if(f > f_b) {
            if(r > r_1) {
                if(board->square[sq - 10] == wn) {return(true);}
            }
            if(r < r_8) {
                if(board->square[sq + 6] == wn) {return(true);}
            }
        }
        if(f < f_g) {
            if(r > r_1) {
                if(board->square[sq - 6] == wn) {return(true);}
            }
            if(r < r_8) {
                if(board->square[sq + 10] == wn) {return(true);}
            }
        }
    } else {
        fprintf(stderr, "ERROR: Unexpected color in Is_check: %d\n", stm);
        exit(1);
    }
    return(false);
}
void chess_board_legal_moves(board_t *board, move_t *temp, enum color stm) {
    /*
     * I meassured that it does 1.2e6 iterations per second, which is like a 40x speedup
     * I missed something major and don't even know what. FUCK!!!! FUCK!!!!
     * Turns out I did miss reseting b->ml and not just t... I am such a dumbazz
     */
    chess_movelist_reset(board->movelist);
    chess_board_pseudolegal_moves(board, temp, stm);
    int64_t c = chess_movelist_count(temp);
    int64_t mli = 0;
    for(int64_t i = 0; i < c; i++) {
        chess_make_move(board, &temp[i]);
        if(!chess_board_is_check(board, stm)) {
            board->movelist[mli] = temp[i];
            mli++;
        }
        chess_undo_move(board, &temp[i]);
    }
}
enum color chess_board_score(board_t *board) {
    // Requires b_lm to have been called before so that b->ml has the legal moves in it and they don't need to be computed again for performance reasons
    if(chess_movelist_count(board->movelist) == 0) {
        if(chess_board_is_check(board, board->stm)) {
#ifdef CHESS_PRINT_GAME_RESULTS
            if(board->stm == c_b) {
                printf("RESULT: 1.0/0.0 = White won by checkmate\n");
            } else {
                printf("RESULT: 0.0/1.0 = Black won by checkmate\n");
            }
#endif
            return(-board->stm); // The side to move is in check and has no legal moves, so the opponent wins
        } else { // No legal moves and no check -> Stalemate
#ifdef CHESS_PRINT_GAME_RESULTS
            printf("RESULT: 0.5/0.5 = Draw by Stalemate\n");
#endif
            return(c_d);
        }
    }
    if(board->fifty_move == 50) {
#ifdef CHESS_PRINT_GAME_RESULTS
        printf("RESULT: 0.5/0.5 = Draw by Fifty-Move rule\n");
#endif
        return(c_d);
    }
    return(c_e);
} 

#define r (rand() % chess_movelist_count(board->movelist))
enum color chess_play_random_game(board_t *board, move_t *temp) {
    /* 
     * It is currently possible to play ~500_000 games per second
     */
    int64_t score = c_e;
    int64_t j;
    chess_board_print(*board);
    while(true) {
        chess_board_legal_moves(board, temp, board->stm);
        if(chess_board_score(board) != c_e) {
            printf("\n\n\n\n");
            score = chess_board_score(board);
            chess_board_print(*board);
            break;
        }
        j = r;
        //chess_move_print(board.movelist[j]);
        chess_make_move(board, &board->movelist[j]);;
        chess_board_print(*board);
        board->stm = -board->stm;
    }
    if(score == c_e) {
        fprintf(stderr, "ERROR: b_play_random_game failed score: %ld\n", score);
        exit(1);
    }
    return(score);
}
int64_t chess_moveindex_from_ai(board_t *board, neuralnet_t *net) {
    /*
     * Requires legalmoves to be in board->movelist
     */
    int64_t count = chess_movelist_count(board->movelist);
    int64_t movelist_i = -1;
    long double best_val = INT32_MAX * -board->stm; // INT32_MAX has no special property it is just a random relatively big number
    for(int64_t i = 0; i < count; i++) {
        chess_make_move(board, &board->movelist[i]);
        for(int64_t j = 0; j < BOARDTOP; j++) {
            MATRIX_AT(net->activations[0], 0, j) = board->square[j];
        }
        neuralnet_forward(*net);;
        chess_undo_move(board, &board->movelist[i]);
    }
    if(movelist_i == -1) {
        exit(1);
    }
    return(movelist_i);
}
enum color chess_play_ai_game(board_t *board, neuralnet_t *net1, neuralnet_t *net2, move_t *temp) {
    return(c_e);
}
