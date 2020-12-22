#ifndef __H_LOBBY
#define __H_LOBBY

#include <arpa/inet.h>
#include <stdio.h>

struct player;

struct lobby {
    char *name;
    struct cn_board *board;
    struct player **player;
    size_t player_len;
    uint8_t turn;
    uint8_t rt_score;
    uint8_t bt_score;
    char *cur_clue;
    uint8_t update_flag;
};

struct player {
    char *id;
    char *name;
    int socket;
    uint8_t checkpt;
    uint8_t prev_checkpt;
    uint8_t player_type;
    uint8_t team;
    char *cur_guess;
    struct sockaddr_in addr;
    struct lobby *lobby;
    char *chat_buffer;
    int8_t disc;
    uint8_t game;
};

#include "board.h"


char *get_lobby_str(struct lobby *lobby);
struct player *get_player(struct lobby *lobby, char *id);
void remove_player(struct lobby *lobby, char *id, int8_t del);
void add_player(struct lobby *lobby, struct player *new_p);

#endif