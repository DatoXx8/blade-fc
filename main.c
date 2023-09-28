#include <time.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>

#include "chess.h"
#include "nn.h"

struct timespec tstart={0, 0}, tend={0, 0};
#define r (rand() % chess_movelist_count(board.movelist))

int main(void) {
    int64_t seed = time(NULL);
    printf("Seed: %lu\n", seed);
    srand(seed);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tstart);

    board_t board = chess_board_alloc();
    const char *starting_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    move_t temp[MAXM];

    chess_board_read_fen(&board, starting_fen);
    chess_play_random_game(&board, temp);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tend);
    printf("The program runtime is %lf seconds\n", (double) (tend.tv_sec - tstart.tv_sec) + 1.0e-9 * (double) (tend.tv_nsec - tstart.tv_nsec));
    return(0);
}
