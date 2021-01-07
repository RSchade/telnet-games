#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "lobby.h"

void remove_disc_players(struct lobby *lobby, uint8_t del_player) {
    uint8_t change = 0;
    struct player **players = lobby->player;
    for(long i = 0; i < lobby->player_len; i++) {
        if (players[i]->disc == 1) {
            printf("Removing id from lobby %s\n", players[i]->id);
            if (del_player > 0) {
                free(players[i]->name);
                free(players[i]->id);
                free(players[i]);
            }
            for(size_t j = i; j < lobby->player_len - 1; j++) {
                players[j] = players[j + 1];
            }
            i--;
            lobby->player_len--;
            change = 0;
        }
    }
    if (change == 1) {
        lobby->player = realloc(lobby->player, 
            lobby->player_len * sizeof(struct player *));
    }
}

void add_player(struct lobby *lobby, struct player *new_p) {
    lobby->player_len += 1;
    lobby->player = realloc(lobby->player, sizeof(struct player*) * lobby->player_len);
    lobby->player[lobby->player_len - 1] = new_p;
}

struct player *get_player(struct lobby *lobby, char *id) {
    // linear probe, TODO: binary search and insert sorted
    // heap?
    for (uint32_t i = 0; i < lobby->player_len; i++) {
        if (strcmp(lobby->player[i]->id, id) == 0) {
            return lobby->player[i];
        }
    }
    return NULL;
}

char *get_lobby_str(struct lobby *lobby) {
    size_t str_size = 30;
    char *out = calloc(str_size, sizeof(char*) * str_size);
    sprintf(out, "Lobby '%s':\n", lobby->name);
    for (size_t i = 0; i < lobby->player_len; i++) {
        size_t added_size = strlen(lobby->player[i]->name + 2);
        size_t cur_size = strlen(out);
        if ((int32_t)str_size - (int32_t)cur_size - (int32_t)added_size < 0) {
            str_size += added_size;
            str_size *= 2;
            out = realloc(out, sizeof(char) * str_size);
        }
        strcat(out, lobby->player[i]->name);
        if (i != lobby->player_len - 1) {
            strcat(out, ", ");
        }
    }
    return out;
}