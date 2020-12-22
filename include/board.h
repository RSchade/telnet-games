#ifndef __BOARD_H
#define __BOARD_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "main.h"
#include "lobby.h"


#define CN_BOARD_RA 1
#define CN_BOARD_BA 2
#define CN_BOARD_BYSTANDER 4
#define CN_BOARD_ASSASSIN 8
#define CN_BOARD_PICKED 16

struct cn_board {
    struct cn_board_elem ***grid;
    uint8_t r;
    uint8_t c;
};

struct cn_board_elem {
    char *word;
    uint8_t state;
};

char *get_board_str(struct cn_board *board, uint8_t player);
struct cn_board *gen_board(uint8_t r, uint8_t c, struct lobby *l, char **master_list, size_t list_size);
size_t populate_master_list(char ***in);
struct cn_board_elem *guess_str(struct cn_board *board, char *guess) ;
struct cn_board_elem *get_board_elem(struct cn_board *board, char *guess);

#endif