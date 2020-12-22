#include "../include/board.h"

char *repeat_char(char in, uint32_t times) {
    char *out = malloc(sizeof(char) * times + 1);
    for (uint32_t i = 0; i < times; i++) {
        out[i] = in;
    }
    out[times] = '\x00';
    return out;
}

size_t get_max_width(struct cn_board *board) {
    size_t max_width = 0;
    for (uint8_t i = 0; i < board->r; i++) {
        for (uint8_t j = 0; j < board->c; j++) {
            size_t str_width = strlen(board->grid[i][j]->word);
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

char *board_flag_str(uint8_t flag) {
    if (flag & CN_BOARD_RA) {
        return "R ";
    } else if (flag & CN_BOARD_BA) {
        return "B ";
    } else if (flag & CN_BOARD_BYSTANDER) {
        return "BY ";
    } else if (flag & CN_BOARD_ASSASSIN) {
        return "A  ";
    }
    return "ER ";
}

char *get_board_str(struct cn_board *in_board, uint8_t player) {
    uint8_t r = in_board->r;
    uint8_t c = in_board->c;
    size_t max_width = get_max_width(in_board) + 3; // add 3 for board flag str max length
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
            struct cn_board_elem *elem = in_board->grid[i][j];
            char *flag_str = board_flag_str(elem->state);
            char *cur_word;
            if (player == CN_SPYMASTER || (player == CN_PLAYER && (elem->state & CN_BOARD_PICKED))) {
                // append agent data to the front of cur_word
                cur_word = malloc(sizeof(char) * strlen(elem->word) + strlen(flag_str) + 1);
                strcpy(cur_word, flag_str);
                strcat(cur_word, elem->word);
            } else if (player == CN_PLAYER) {
                cur_word = malloc(sizeof(char) * strlen(elem->word) + 1);
                strcpy(cur_word, elem->word);
            } else {
                printf("Invalid player type %d\n", player);
            }
            size_t pad_width = (max_width - strlen(cur_word)) + 2;
            char *l = repeat_char(' ', pad_width / 2);
            char *r = repeat_char(' ', pad_width - (pad_width / 2));
            sprintf(board + board_len, "|%s%s%s", l, cur_word, r);
            free(cur_word);
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

size_t populate_master_list(char ***in) {
    size_t buf_size = 100;
    size_t buf_pos = 0;
    char *buf = malloc(buf_size * sizeof(char));
    FILE *f = fopen("wordlist.csv", "r");
    if (f == NULL && errno != 0) {
        printf("Error opening master list file\n%s\n", strerror(errno));
        // TODO: quit?
        return -1;
    }
    while (fgets(buf + buf_pos, buf_size - buf_pos, f) != NULL) {
        buf_pos = buf_size - 1;
        buf_size *= 1.5;
        buf = realloc(buf, buf_size);
    }
    fclose(f);
    size_t len = 0;
    size_t i = 0;
    while (buf[i] != '\x00') {
        len += buf[i] == ',';
        i++;
    }
    *in = malloc(sizeof(char*) * len);
    i = 0;
    char *ptr = strtok(buf, ",");
    for (size_t i = 0; i < len; i++) {
        char *cur = strdup(ptr);
        (*in)[i] = cur;
        ptr = strtok(NULL, ",");
    }
    free(buf);
    return len;
}

struct cn_board *gen_board(uint8_t r, uint8_t c, struct lobby *l, char **master_list, size_t list_size) {
    // 9 of first team to go, 8 of other, 7 bystanders, 1 assassin
    struct cn_board_elem **elems = calloc(sizeof(struct cn_board_elem *), r * c);
    for (uint8_t i = 0; i < r * c; i++) {
        elems[i] = calloc(sizeof(struct cn_board_elem), 1);
        if (i <= 8) {
            elems[i]->state = CN_BOARD_RA;
        } else if (i <= 16) {
            elems[i]->state = CN_BOARD_BA;
        } else if (i <= 23) {
            elems[i]->state = CN_BOARD_BYSTANDER;
        } else {
            elems[i]->state = CN_BOARD_ASSASSIN;
        }
    }
    // randomize elems order
    for (uint8_t i = 0; i < (r * c) * 2; i++) {
        uint8_t n1 = rand() % (r * c);
        uint8_t n2 = rand() % (r * c);
        struct cn_board_elem *swap = elems[n1];
        elems[n1] = elems[n2];
        elems[n2] = swap;
    }
    struct cn_board *board = calloc(sizeof(struct cn_board), 1);
    board->grid = malloc(r * sizeof(struct cn_board_elem*));
    size_t *idxs = calloc(sizeof(size_t), r * c);
    uint8_t idxs_len = 0;
    // TODO: start red/blue starting randomly?
    l->rt_score = 9;
    l->bt_score = 8;
    l->board = board;
    l->turn = CN_RED_TEAM;
    for (uint8_t i = 0; i < r; i++) {
        board->grid[i] = malloc(c * sizeof(struct cn_board_elem*));
        // use master list to populate board
        for (uint8_t j = 0; j < c; j++) {
            // pull from 'elems' above, it's already randomized
            board->grid[i][j] = elems[(i * r) + j];
            struct cn_board_elem *elem = board->grid[i][j];
            size_t idx = rand() % list_size;
            for (size_t k = 0; k < idxs_len; k++) {
                if (idx == idxs[k]) {
                    idx = (idx + 1) % list_size;
                }
            }
            
            elem->word = master_list[idx];
            idxs[idxs_len] = idx;
            idxs_len++;
        }
    }
    free(elems);
    free(idxs);
    board->r = r;
    board->c = c;
    return board;
}

struct cn_board_elem *get_board_elem(struct cn_board *board, char *guess) {
    for (uint8_t i = 0; i < board->r; i++) {
        for (uint8_t j = 0; j < board->c; j++) {
            if (strcmp(board->grid[i][j]->word, guess) == 0) {
                return board->grid[i][j];
            }
        }
    }
    return NULL;
}

struct cn_board_elem *guess_str(struct cn_board *board, char *guess) {
    struct cn_board_elem *be = get_board_elem(board, guess);
    if (be == NULL) {
        return NULL;
    }
    be->state |= CN_BOARD_PICKED;
    return be;
}