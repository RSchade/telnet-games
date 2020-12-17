#include "../include/board.h"

char *repeat_char(char in, uint32_t times) {
    char *out = malloc(sizeof(char) * times + 1);
    for (uint32_t i = 0; i < times; i++) {
        out[i] = in;
    }
    out[times] = '\x00';
    return out;
}

size_t get_max_width(uint8_t r, uint8_t c, char ***grid_text) {
    size_t max_width = 0;
    for (uint8_t i = 0; i < r; i++) {
        for (uint8_t j = 0; j < c; j++) {
            size_t str_width = strlen(grid_text[i][j]);
            if (str_width > max_width) {
                max_width = str_width;
            }
        }
    }
    return max_width;
}

char *get_blank_row(size_t max_width, uint32_t grid_len) {
    char *blank_row = malloc(sizeof(char) * (grid_len + 1) + 1);
    for (int i = 0; i < grid_len + 1; i++) {
        if (i % (max_width + 3) == 0) {
            blank_row[i] = '|';
        } else {
            blank_row[i] = ' ';
        }
    }
    blank_row[grid_len + 1] = '\x00';
    return blank_row;
}

size_t update_board_len(uint32_t *board_size, uint8_t added_size, char **board) {
    size_t len = strlen(*board);
    if ((int32_t)*board_size - (int32_t)len <  added_size) {
        *board_size += added_size;
        *board_size *= 2;
        *board = realloc(*board, *board_size);
    }
    return len;
}

char *get_board_str(uint8_t r, uint8_t c, char ***grid_text) {
    size_t max_width = get_max_width(r, c, grid_text);
    uint32_t grid_len = (max_width + 3) * c;
    char *blank_row = get_blank_row(max_width, grid_len);
    char *pluses = repeat_char('=', grid_len - 1);
    uint32_t board_size = 50;
    char *board = malloc(sizeof(char) * board_size);
    board[0] = '\x00';
    uint32_t board_len = 0;
    for (uint8_t i = 0; i < r; i++) {
        board_len = update_board_len(&board_size, (grid_len * 2) + 2, &board);
        sprintf(board + board_len, "+%s+\n%s\n", pluses, blank_row);
        for (uint8_t j = 0; j < c; j++) {
            board_len = update_board_len(&board_size, grid_len + 1, &board);
            size_t pad_width = (max_width - strlen(grid_text[i][j])) + 2;
            char *l = repeat_char(' ', pad_width / 2);
            char *r = repeat_char(' ', pad_width - (pad_width / 2));
            sprintf(board + board_len, "|%s%s%s", l, grid_text[i][j], r);
            free(l);
            free(r);
        }
        board_len = update_board_len(&board_size, (grid_len * 2) + 2, &board);
        sprintf(board + board_len, "|\n%s\n", blank_row);
    }
    board_len = update_board_len(&board_size, grid_len + 1, &board);
    sprintf(board + board_len, "+%s+", pluses);
    free(pluses);
    free(blank_row);
    return board;
}

char ***gen_board(uint8_t r, uint8_t c) {
    char ***grid_text = malloc(r * sizeof(char*));
    for (int i = 0; i < r; i++) {
        grid_text[i] = malloc(c * sizeof(char*));
    }
    return grid_text;
}
