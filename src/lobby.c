#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "lobby.h"

void add_player(struct lobby *lobby, struct player *new_p) {
    lobby->player_len += 1;
    lobby->player = realloc(lobby->player, sizeof(struct player*) * lobby->player_len);
    lobby->player[lobby->player_len - 1] = new_p;
}

void remove_player(struct lobby *lobby, char *id, int8_t del) {
    // take the end and put it in the middle
    for (size_t i = 0; i < lobby->player_len; i++) {
        if (strcmp(lobby->player[i]->id, id) == 0) {
            if (i != lobby->player_len - 1) {
                // take the end and put it here
                lobby->player[i] = lobby->player[lobby->player_len - 1];
            }
            lobby->player_len--;
            if (del > 0) {
                struct player *p = lobby->player[i];
                close(p->socket);
                // TODO: add more frees for the inner pieces of data
                free(p);
            }
        }
    }
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