#include "../include/main.h"

// master lobby (has everyone in it)
struct lobby master_lobby;

// lobby list and length
struct lobby *lobbys = NULL;
uint32_t lobby_len = 0;

// word list and length
char **master_list;
size_t ml_len;

char *welcome_msg = "\r\nConnected to telnet games server\r\n\r\n";
char *lobby_name_msg = "Enter your name and press enter, maximum 10 characters\r\n";
char *lobby_msg = "\r\nPick the lobby you wish to join by number, or type n,<lobby name> to start a new lobby\r\n";
char *cn_start_msg = "Codenames is a game about guessing words, you can either be a spymaster or a player. Each team has one spymaster and many players. The spymaster can see the card's word and what type of card it is, the players can only see the word. Spymasters give their team a guess consisting of one word and a number ranging from 1 through 9 to give them a clue on which cards to pick. This guess cannot include a word on the board. Players then must reach a consensus on which card to flip over, if the card's type matches your team type (B, R) then your team score is deducted by one, if it's the opposite player's card their score is deducted by one and your team's turn ends. If the card is an innocent bystander (BY) then your team's turn ends and nobody scores. If the card is an assassin (A) then your team automatically loses. The team with the lowest score wins.\r\n";
char *cn_pick_player_msg = "Enter 1,R to enter as a red player, 2,R for red spymaster, replace B with R for the blue team.\r\n";
char *cn_retry_msg = "Invalid input, please retry...\r\n";
char *cn_player_turn_msg = "Lines entered will be transmitted to your teammates, when you are ready to guess type an exclamation point (!) before a single word and press enter. Once a consensus is reached that word will be your team's guess. To stop guessing your team must reach consensus on !!\r\n";
char *cn_spymaster_turn_msg = "Enter in a word comma a digit (1-9) and then press enter to send your team a clue on which word to guess\r\n";
char *cn_waiting_turn_msg = "While you wait for your team's turn, you can type messages to your teammates\r\n";
char *cn_guess_recieved_msg = "Your guess has been recieved, it currently is: ";
char *cn_new_clue = "The spymaster clue is: ";
char *cn_invalid_guess_msg = "Invalid guess, please write your guess exactly as it appears in the grid above\r\n";
char *cn_spymaster_rej_msg = "Sorry, there's already a spymaster for that team\r\n";
char *cn_red_team_win_msg = "Congratulations red team, you have won this game!\r\n";
char *cn_blue_team_win_msg = "Congratulations blue team, you have won this game!\r\n";

ssize_t write_str2(int socket, char *buf, char *id, struct lobby *l) {
    ssize_t out = write_str(socket, buf);
    if (out == -1 && errno == EPIPE) {
        // flag player as disconnected, this will
        // cause the rest of the logic to skip that player
        // (until they reconnect)
        struct player *p = get_player(&master_lobby, id);
        p->disc = 1;
        return -2;
    } else if (out == -1) {
        return -1;
    }
    return 0;
}

void escape_str(char *str) {
    // only allow ASCII characters 32 through 126 in the chat msg
    while (str[0] != '\x00') {
        if (str[0] < 32 || str[0] > 126) {
            str[0] = ' '; // replace w/ space
        }
        str++;
    }
}

int8_t print_board_state(struct lobby *lobby, struct player *player) {
    int socket = player->socket;
    char *board = get_board_str(lobby->board, player->player_type);
    char *id = player->id;
    char *top = calloc(sizeof(char), 100); // TODO: exact size?
    sprintf(top, "%s, %s, %dr - %db\r\n", player->team == CN_RED_TEAM ? "Red Team" : "Blue Team", 
                                 player->team == lobby->turn ? "Your Team's Turn" : "Their Team's Turn",
                                 lobby->rt_score, lobby->bt_score);
    ssize_t err = write_str2(socket, top, id, lobby);
    err |= write_str2(socket, board, id, lobby);
    err |= write_str2(socket, "\r\n", id, lobby);
    free(top);
    if (err < 0) {
        return -1;
    }
    free(board);
    return 0;
}

int8_t player_guess(struct lobby *lobby, struct player *player, char *buf) {
    int socket = player->socket;
    char *id = player->id;
    // parse !<word>
    char *guess = malloc(sizeof(char) * strlen(buf + 1) + 1);
    strcpy(guess, buf + 1);
    // if guess is not in the board, then display an error
    // and don't adjust the player guess
    struct cn_board_elem *be = get_board_elem(lobby->board, guess);
    if ((guess[0] != '!' || strlen(guess) != 1) 
        && (be == NULL || (be->state & CN_BOARD_PICKED) != 0)) {
        ssize_t err = write_str2(socket, cn_invalid_guess_msg, id, lobby);
        free(guess);
        if (err < 0) {
            return -1;
        }
    } else {
        if (player->cur_guess != NULL) {
            free(player->cur_guess);
        }
        player->cur_guess = guess;
        ssize_t err = write_str2(socket, cn_guess_recieved_msg, id, lobby);
        err |= write_str2(socket, guess, id, lobby);
        err |= write_str2(socket, "\r\n", id, lobby);
        if (err < 0) {
            return -1;
        }
        printf("Guess: %s\r\n", player->cur_guess);
    }
    return 0;
}

int8_t spymaster_guess(struct lobby *lobby, struct player *player, char *buf) {
    int socket = player->socket;
    char *id = player->id;
    size_t len = strlen(buf);
    // parse <word>,<number> (word must be at least 4 characters, number is > 0 and <= 9)
    if (len < 2 || buf[len - 2] != ',' || buf[len - 1] > 57 || buf[len - 1] < 49) {
        if (write_str2(socket, cn_retry_msg, id, lobby) < 0) {
            return -1;
        }
    } else {
        // put clue into lobby
        char *clue = malloc(sizeof(char) * strlen(buf) + 1);
        strcpy(clue, buf);
        lobby->cur_clue = clue;
        lobby->update_flag |= CN_CLUE_UPDATE;
    }
    return 0;
}

void codenames_states(struct player *new_player, char *buf) {
    int socket = new_player->socket;
    uint8_t checkpt = new_player->checkpt;
    uint8_t new_checkpt = checkpt;
    uint8_t prev_checkpt = new_player->prev_checkpt;
    struct lobby *lobby = new_player->lobby;
    char *id = new_player->id;
    uint8_t chat_flag = 0;

    switch (checkpt) {
        case CN_START:
            if (write_str2(socket, cn_start_msg, id, lobby) < 0) {
                return;
            }
            new_checkpt = CN_PICK_PLAYER_TYPE;
            break;
        case CN_PICK_PLAYER_TYPE:
            if (prev_checkpt != CN_PICK_PLAYER_TYPE) {
                if (write_str2(socket, cn_pick_player_msg, id, lobby) < 0) {
                    return;
                }
            }
            // pick blue or red team, only allow 1 spymaster per team
            if (buf != NULL) {
                char type = '\x00', team = '\x00';
                uint8_t err = 0;
                sscanf(buf, "%c,%c", &type, &team);

                if (team == 'R') {
                    new_player->team = CN_RED_TEAM;
                } else if (team == 'B') {
                    new_player->team = CN_BLUE_TEAM;
                } else {
                    err = 1;
                }

                if (type == '1') {
                    new_player->player_type = CN_PLAYER;
                } else if (type == '2') {
                    // check if other people are spymaster
                    for (uint32_t i = 0; i < lobby->player_len; i++) {
                        if (lobby->player[i]->player_type == CN_SPYMASTER &&
                            lobby->player[i]->team == new_player->team) {
                            err = 2;
                        }
                    }
                    new_player->player_type = CN_SPYMASTER;
                } else {
                    err = 1;
                }

                if (err == 0) {
                    new_checkpt = CN_PRINT_BOARD;
                } else {
                    if (write_str2(socket, err == 2 ? cn_spymaster_rej_msg : cn_retry_msg, id, lobby) < 0) {
                        return;
                    }
                }
            }
            break;
        case CN_PRINT_BOARD:
            if (prev_checkpt != CN_PRINT_BOARD) {
                if (print_board_state(lobby, new_player) < 0) {
                    return;
                }
            }
            if (new_player->team != lobby->turn) {
                new_checkpt = CN_NOT_YOUR_TURN;
            } else {
                new_checkpt = CN_YOUR_TURN;
            }
            break;
        case CN_YOUR_TURN:
            if (new_player->player_type == CN_PLAYER) {
                // initial message player
                if (prev_checkpt != CN_YOUR_TURN) {
                    if (write_str2(socket, cn_player_turn_msg, id, lobby) < 0) {
                        return;
                    }
                }
                if (buf != NULL && buf[0] == '!') {
                    if (player_guess(lobby, new_player, buf) < 0) {
                        return;
                    }
                } else if (buf != NULL) {
                    // team chat
                    chat_flag = 1;
                }
            } else if(new_player->player_type == CN_SPYMASTER) {
                // initial message spymaster
                if (prev_checkpt != CN_YOUR_TURN && lobby->cur_clue == NULL) {
                    if (write_str2(socket, cn_spymaster_turn_msg, id, lobby) < 0) {
                        return;
                    }
                }
                if (buf != NULL && lobby->cur_clue == NULL) {
                    if (spymaster_guess(lobby, new_player, buf) < 0) {
                        return;
                    }
                } else {
                    // team chat
                    chat_flag = 1;
                }
            } else {
                printf("Invalid player type %d!\r\n", new_player->player_type);
            }
            break;
        case CN_NOT_YOUR_TURN:
            if (prev_checkpt != CN_NOT_YOUR_TURN) {
                if (write_str2(socket, cn_waiting_turn_msg, id, lobby) < 0) {
                    return;
                }
            }
            // team chat
            chat_flag = 1;
            break;
        case CN_GAME_END:
            printf("Lobby ended\r\n");
            write_str2(socket, lobby->winning_team == CN_RED_TEAM ? 
                cn_red_team_win_msg : cn_blue_team_win_msg, id, lobby);
            // disconnect the player
            new_player->disc = 1;
            disc_socket(socket);
            break;
        default:
            printf("Invalid checkpoint %d!\r\n", checkpt);
            break;
    }

    if (chat_flag > 0 && buf != NULL) {
        char *chat = malloc(sizeof(char) * strlen(buf) + 1);
        strcpy(chat, buf);
        new_player->chat_buffer = chat;
    }

    new_player->prev_checkpt = new_player->checkpt;
    new_player->checkpt = new_checkpt;
}

void codenames_game_rules(struct lobby *lobby) {
    // run the background game rules for this lobby
    // when all the guesses for the current team are in consensus then change the turn
    // team chat
    // display the guess from the spymaster when it is available

    int8_t guess_consensus = 0;
    char *consensus = NULL;

    for (size_t i = 0; i < lobby->player_len; i++) {
        struct player *p = lobby->player[i];
        if (p->disc > 0) {
            continue;
        }
        // transmit player chat (if any...)
        if (p->chat_buffer != NULL) {
            for (size_t j = 0; j < lobby->player_len; j++) {
                struct player *p2 = lobby->player[j];
                if (p2->disc > 0) {
                    continue;
                }
                if ((p->player_type == p2->player_type 
                            || p2->player_type == CN_SPYMASTER)
                            && strcmp(p->id, p2->id) != 0) {
                    size_t full_len = strlen(p->chat_buffer) + strlen(p->name) + 4;
                    char *full_chat = calloc(sizeof(char), full_len + 1);
                    escape_str(p->chat_buffer);
                    strcat(full_chat, p->name);
                    strcat(full_chat, ": ");
                    strcat(full_chat, p->chat_buffer);
                    strcat(full_chat, "\r\n");
                    if (write_str2(p2->socket, full_chat, p2->id, lobby) < 0) {
                        j--;
                        free(full_chat);
                        continue;
                    }
                    free(full_chat);
                }
            }
            free(p->chat_buffer);
            p->chat_buffer = NULL;
        }
        // display spymaster clue if available
        if ((lobby->update_flag & CN_CLUE_UPDATE) > 0) {
            write_str2(p->socket, cn_new_clue, p->id, lobby);
            write_str2(p->socket, lobby->cur_clue, p->id, lobby);
            write_str2(p->socket, "\r\n", p->id, lobby);
        }
        // check player consensus
        if (guess_consensus == 0 && p->player_type == CN_PLAYER && p->team == lobby->turn) {
            if (consensus == NULL && p->cur_guess != NULL) {
                consensus = p->cur_guess;
            }
            if (p->cur_guess == NULL || strcmp(p->cur_guess, consensus) != 0) {
                guess_consensus = 1;
            }
        }
    }

    // turn off the CLUE_UPDATE flag
    lobby->update_flag &= ~CN_CLUE_UPDATE;

    // runs when there is a consensus among players
    if (guess_consensus == 0 && consensus != NULL) {
        printf("GUESS CONSENSUS\r\n");
        uint8_t start_turn = lobby->turn;
        uint8_t game_ends = 0;
        // move to the next turn
        int8_t next_turn = strcmp(consensus, "!") == 0;
        if (next_turn) {
            // switch turn
            if (lobby->turn == CN_BLUE_TEAM) {
                lobby->turn = CN_RED_TEAM;
            } else {
                lobby->turn = CN_BLUE_TEAM;
            }
        } else {
            // guess the string
            struct cn_board_elem *be = guess_str(lobby->board, consensus);
            // adjust score
            if (be->state & CN_BOARD_RA) {
                lobby->rt_score--;
                lobby->turn = CN_RED_TEAM;
            } else if (be->state & CN_BOARD_BA) {
                lobby->bt_score--;
                lobby->turn = CN_BLUE_TEAM;
            } else if (be->state & CN_BOARD_BYSTANDER) {
                // change turn
                if (lobby->turn == CN_RED_TEAM) {
                    lobby->turn = CN_BLUE_TEAM;
                } else {
                    lobby->turn = CN_RED_TEAM;
                }
            } else if (be->state & CN_BOARD_ASSASSIN) {
                // game ends, current team loses
                game_ends = 1;
                lobby->winning_team = lobby->turn == CN_RED_TEAM ? CN_BLUE_TEAM : CN_RED_TEAM;
                lobby->update_flag |= CN_ASSASSIN_LOSS;
            } else {
                printf("Invalid board state %d\r\n", be->state);
            }

            if (lobby->rt_score == 0) {
                lobby->winning_team = CN_RED_TEAM;
                game_ends = 1;
            }

            if (lobby->bt_score == 0) {
                lobby->winning_team = CN_BLUE_TEAM;
                game_ends = 1;
            }
        }

        if (lobby->cur_clue != NULL && start_turn != lobby->turn) {
            free(lobby->cur_clue);
            lobby->cur_clue = NULL;
        }

        for (size_t i = 0; i < lobby->player_len; i++) {
            struct player *p = lobby->player[i];
            if (p->disc > 0) {
                continue;
            }
            free(p->cur_guess);
            p->cur_guess = NULL;
            // reprint boards for everyone
            if (game_ends) {
                p->checkpt = CN_GAME_END;
                lobby->update_flag |= CN_GAME_ENDED;
            } else {
                p->checkpt = CN_PRINT_BOARD;
            }
        }
    }
}

struct lobby gen_lobby(char *lobby_name) {
    struct lobby l;
    l.name = lobby_name;
    l.player_len = 0;
    uint8_t r = 5, c = 5;
    struct cn_board *board = gen_board(r, c, &l, master_list, ml_len);
    l.cur_clue = NULL;
    l.player = NULL;
    l.player_len = 0;
    l.update_flag = 0;
    l.board = board;
    l.turn = CN_RED_TEAM;
    return l;
}

void matchmaking_states(struct player *p, char *buf) {
    uint8_t new_checkpt = p->checkpt;

    switch(p->checkpt) {
        case LOBBY_NAME:
            if (p->prev_checkpt != p->checkpt) {
                write_str(p->socket, lobby_name_msg);
                p->name = malloc(sizeof(char) * 11);
            }
            if (buf != NULL) {
                if (sscanf(buf, "%10s", p->name) > 0) {
                    new_checkpt = LOBBY_DECIDE;
                } else {
                    write_str(p->socket, cn_retry_msg);
                }
            }
            break;
        case LOBBY_DECIDE:
            if (p->prev_checkpt != p->checkpt) {
                write_str(p->socket, welcome_msg);
                for (uint32_t i = 0; i < lobby_len; i++) {
                    char *lobby_str = get_lobby_str(&(lobbys[i]));
                    char s[15]; // TODO: actual length
                    sprintf(s, "(%d) ", i);
                    write_str(p->socket, s);
                    write_str(p->socket, lobby_str);
                    write_str(p->socket, " ");
                    free(lobby_str);
                }
                write_str(p->socket, lobby_msg);
            }
            if (buf != NULL) {
                // parse string to pick a lobby/create a new lobby
                int32_t lobby_id = -1;
                char *lobby_name = calloc(sizeof(char), 11);
                if (sscanf(buf, "%d", &lobby_id) == 1 && lobby_id < lobby_len) {
                    p->lobby = &lobbys[lobby_id];
                    new_checkpt = LOBBY_PROMPT;
                } else if (sscanf(buf, "n,%10s", lobby_name) == 1) {
                    struct lobby l = gen_lobby(lobby_name);
                    lobby_len += 1;
                    lobbys = realloc(lobbys, sizeof(struct lobby) * lobby_len);
                    lobbys[lobby_len - 1] = l;
                    p->lobby = &lobbys[lobby_len - 1];
                    new_checkpt = LOBBY_PROMPT;
                } else {
                    write_str(p->socket, cn_retry_msg);
                }
                free(lobby_name);
            }
            break;
        case LOBBY_PROMPT:
            if (p->prev_checkpt != p->checkpt) {
                write_str(p->socket, "Joining game...\r\n");
            }
            p->game = CN_GAME;
            p->player_type = 255;
            // add player to lobby
            add_player(p->lobby, p);
            new_checkpt = CN_START;
            break;
    }

    p->prev_checkpt = p->checkpt;
    p->checkpt = new_checkpt;
}

void new_user(int new_socket, char *client_id, struct sockaddr_in addr) {
    // server initialization (1st player joins)
    if (lobbys == NULL) {
        lobbys = calloc(sizeof(struct lobby), 1);
        lobby_len = 1;
        lobbys[0] = gen_lobby("lobby #1");
        master_lobby.player_len = 0;
    }

    // initialize player in master lobby (if they have been disconnected for too long)
    struct player *client_player = get_player(&master_lobby, client_id);
    if (client_player == NULL) {
        struct player *p = malloc(sizeof(struct player));
        p->id = client_id;
        p->name = NULL;
        p->addr = addr;
        p->socket = new_socket;
        p->checkpt = 0;
        p->prev_checkpt = 255;
        p->lobby = NULL;
        p->cur_guess = NULL;
        p->chat_buffer = NULL;
        p->disc = 0;
        p->game = BEGIN;
        add_player(&master_lobby, p);
    }
}

void player_event_loop() {
    for (size_t i = 0; i < master_lobby.player_len; i++) {
        struct player *current_player = master_lobby.player[i];
        if (current_player->disc > 0) {
            continue;
        }
        uint8_t first = 0;
        while ((current_player->disc == 0 && current_player->checkpt != current_player->prev_checkpt) || first == 0) {
            first = 1;
            //printf("STATE\r\n");
            int socket = current_player->socket;
            char *buf;

            ssize_t read_len = read_str(socket, &buf);

            if (read_len <= -1) {
                printf("Could not read from %s\r\n", current_player->id);
                current_player->disc = 1;
                continue;
            }
            
            if (read_len > 1) {
                buf[read_len - 1] = '\x00'; // TODO: test with \r\n and \r\r\n
                buf[read_len - 2] = '\x00';
                printf("Recieved from %s: %s\r\n", current_player->id, buf);
            } else {
                free(buf);
                buf = NULL;
            }

            // parse input based on player's state
            if (current_player->game == BEGIN) {
                matchmaking_states(current_player, buf);
            } else if (current_player->game == CN_GAME) {
                codenames_states(current_player, buf);
            } else {
                printf("INVALID GAME %d\r\n", current_player->game);
            }

            if (buf != NULL) {
                free(buf);
            }
        }
    }
}

void bg_event_loop() {
    // run the background game rules
    for (size_t i = 0; i < lobby_len; i++) {
        codenames_game_rules(&lobbys[i]);
    }
}

void cleanup_lobbys() {
    // clean up disconnected people out of the lobbys
    remove_disc_players(&master_lobby, 0);
    for (size_t i = 0; i < lobby_len; i++) {
        remove_disc_players(&lobbys[i], 1);
    }
    // remove lobbys from the list that have ended
    uint8_t change = 0;
    for (long i = 0; i < lobby_len; i++) {
        if ((lobbys[i].update_flag & CN_GAME_ENDED) > 0) {
            for (size_t j = i; j < lobby_len - 1; j++) {
                lobbys[j] = lobbys[j + 1];
            }
            i--;
            lobby_len--;
        }
    }
    if (change == 1) {
        lobbys = realloc(lobbys, lobby_len * sizeof(struct player));
    }
}

void event_loop() {
    player_event_loop();
    bg_event_loop();
    player_event_loop();
    cleanup_lobbys();
}

void int_handler() {
    printf("Cleaning up...\r\n");
    for (size_t i = 0; i < ml_len; i++) {
        free(master_list[i]);
    }
    free(master_list);
    exit(0);
}

int main(int argc, char *argv[]) {

    //test();

    srand(time(0));
    ml_len = populate_master_list(&master_list);
    socket_init();
    signal(SIGINT, int_handler);
    socket_poll(event_loop);
    return 0;

}
