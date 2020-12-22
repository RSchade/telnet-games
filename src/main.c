#include "../include/main.h"

// master lobby (has everyone in it)
struct lobby master_lobby;

// lobby list and length
struct lobby *lobbys = NULL;
uint32_t lobby_len = 0;

// word list and length
char **master_list;
size_t ml_len;

char *welcome_msg = "\nConnected to telnet games server\n\n";
char *lobby_msg = "\nPick the lobby you wish to join by number, or type n,<lobby name> to start a new lobby\n";
char *cn_start_msg = "Codenames is a game about guessing words, you can either be a spymaster or a player. Each team has one spymaster and many players. The spymaster can see the card's word and what type of card it is, the players can only see the word. Spymasters give their team a guess consisting of one word and a number ranging from 1 through 9 to give them a clue on which cards to pick. This guess cannot include a word on the board. Players then must reach a consensus on which card to flip over, if the card's type matches your team type (B, R) then your team score is deducted by one, if it's the opposite player's card their score is deducted by one and your team's turn ends. If the card is an innocent bystander (BY) then your team's turn ends and nobody scores. If the card is an assassin (A) then your team automatically loses. The team with the lowest score wins.\n";
char *cn_pick_player_msg = "Enter 1 for player, 2 for spymaster then enter when ready\n";
char *cn_retry_msg = "Invalid input, please retry...\n";
char *cn_player_turn_msg = "Lines entered will be transmitted to your teammates, when you are ready to guess type an exclamation point (!) before a single word and press enter. Once a consensus is reached that word will be your team's guess. To stop guessing your team must reach consensus on !!\n";
char *cn_spymaster_turn_msg = "Enter in a word comma a digit (1-9) and then press enter to send your team a clue on which word to guess\n";
char *cn_waiting_turn_msg = "While you wait for your team's turn, you can type messages to your teammates\n";
char *cn_guess_recieved_msg = "Your guess has been recieved, it currently is: ";
char *cn_new_clue = "The spymaster clue is: ";
char *cn_invalid_guess_msg = "Invalid guess, please write your guess exactly as it appears in the grid above\n";

char *get_lobby_list_str() {
    return NULL;
}

ssize_t write_str2(int socket, char *buf, char *id, struct lobby *l) {
    ssize_t out = write_str(socket, buf);
    if (out == -1 && errno == EPIPE) {
        // remove from lobby (this may cause more issues than it solves?)
        //remove_player(l, id, 0);
        //remove_player(&master_lobby, id, 1);

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
    sprintf(top, "%s, %s, %dr - %db\n", player->team == CN_RED_TEAM ? "Red Team" : "Blue Team", 
                                 player->team == lobby->turn ? "Your Team's Turn" : "Their Team's Turn",
                                 lobby->rt_score, lobby->bt_score);
    ssize_t err = write_str2(socket, top, id, lobby);
    err |= write_str2(socket, board, id, lobby);
    err |= write_str2(socket, "\n", id, lobby);
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
        err |= write_str2(socket, "\n", id, lobby);
        if (err < 0) {
            return -1;
        }
        printf("Guess: %s\n", player->cur_guess);
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
            // TODO: pick blue or red team, only allow 1 spymaster per team
            if (buf != NULL) {
                if (strcmp(buf, "1") == 0) {
                    new_player->player_type = CN_PLAYER;
                    new_checkpt = CN_PRINT_BOARD;
                } else if (strcmp(buf, "2") == 0) {
                    new_player->player_type = CN_SPYMASTER;
                    new_checkpt = CN_PRINT_BOARD;
                } else {
                    if (write_str2(socket, cn_retry_msg, id, lobby) < 0) {
                        return;
                    }
                }
                
                
                lobby->turn = CN_RED_TEAM;
                new_player->team = CN_RED_TEAM;
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
                if (prev_checkpt != CN_YOUR_TURN) {
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
                printf("Invalid player type %d!\n", new_player->player_type);
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
            printf("Game ended... %d\n", lobby->update_flag);
            // TODO: game end text
            // TODO: boot players off server? or back into main lobby?
            break;
        default:
            printf("Invalid checkpoint %d!\n", checkpt);
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
                    strcat(full_chat, "\n");
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
            write_str2(p->socket, "\n", p->id, lobby);
        }
        // check player consensus   
        if (guess_consensus == 0 && p->player_type == CN_PLAYER) {
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
        uint8_t game_ends = 0;
        // move to the next turn
        free(lobby->cur_clue);
        int8_t next_turn = strcmp(consensus, "!") == 0;
        lobby->cur_clue = NULL;
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
                lobby->update_flag |= CN_ASSASSIN_LOSS;
            } else {
                printf("Invalid board state %d\n", be->state);
            }
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
    return l;
}

void matchmaking_states(struct player *p, char *buf) {
    uint8_t new_checkpt = p->checkpt;

    switch(p->checkpt) {
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
            }
            break;
        case LOBBY_PROMPT:
            if (p->prev_checkpt != p->checkpt) {
                write_str(p->socket, "Joining game...\n");
            }
            p->game = CN_GAME;
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
        char *id2 = calloc(strlen(client_id) + 1, sizeof(char));
        strcpy(id2, client_id);
        p->name = id2;
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

    /*
    struct lobby player_lobby = lobbys[0];
    char *lobby_str = get_lobby_str(&player_lobby);
    write_str(new_socket, lobby_str);
    free(lobby_str);
    */
}

void event_loop() {

    for (size_t i = 0; i < master_lobby.player_len; i++) {
        struct player *current_player = master_lobby.player[i];
        if (current_player->disc > 0) {
            continue;
        }
        int socket = current_player->socket;

        // recieve bytes from the client
        ssize_t buf_size = 10;
        ssize_t read_len = 0;
        char *buf = malloc(sizeof(char) * buf_size);
        uint8_t e_again = 0;
        while (!e_again) {
            ssize_t size_read = read(socket, buf + read_len, buf_size - read_len);
            if (size_read > 0) {
                read_len += size_read;
                buf_size *= 2;
                buf = realloc(buf, buf_size);
                for (ssize_t i = read_len; i < buf_size; i++) {
                    buf[i] = '\x00';
                }
            } else if (size_read == 0) {
                e_again = 1;
            } else {
                e_again = errno == (EAGAIN | EWOULDBLOCK);
                if (errno == EPIPE) {
                    printf("Could not read from %s\n", current_player->id);
                    remove_player(current_player->lobby, current_player->id, 0);
                    remove_player(&master_lobby, current_player->id, 1);
                    i--;
                    free(buf);
                    continue;
                }
            }
        }
        
        if (read_len > 0) {
            buf[read_len - 1] = '\x00'; // TODO: test with \n and \r\n
            buf[read_len - 2] = '\x00';
            printf("Recieved from %s: %s\n", current_player->id, buf);
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
            printf("INVALID GAME %d\n", current_player->game);
        }

        if (buf != NULL) {
            free(buf);
        }
    }

    // run the background game rules
    for (size_t i = 0; i < lobby_len; i++) {
        codenames_game_rules(&lobbys[i]);
    }
}

void int_handler() {
    printf("Cleaning up...\n");
    for (size_t i = 0; i < ml_len; i++) {
        free(master_list[i]);
    }
    free(master_list);
    exit(0);
}

int main(int argc, char *argv[]) {
    srand(time(0));
    ml_len = populate_master_list(&master_list);
    socket_init();
    signal(SIGINT, int_handler);
    socket_poll(event_loop);
    // TODO: frees for everything when the server spins down
    return 0;
}
