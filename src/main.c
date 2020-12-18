#include "../include/lobby.h"
#include "../include/board.h"
#include "../include/net.h"
#include "../include/main.h"

struct lobby master_lobby;
struct lobby *lobbys = NULL;
uint32_t lobby_len = 0;

char *welcome_msg = "\nConnected to telnet games server\n\n";
char *cn_start_msg = "Starting game in current lobby...\n";
char *cn_pick_player_msg = "Enter 1 for player, 2 for spymaster then enter when ready\n";
char *cn_retry_msg = "Invalid input, please retry...\n";
char *cn_player_turn_msg = "Lines entered will be transmitted to your teammates, when you are ready to guess type an exclamation point (!) before a single word and press enter. Once a consensus is reached that word will be your team's guess. To stop guessing your team must reach consensus on !!\n";
char *cn_spymaster_turn_msg = "Enter in a word comma a digit (0-9) and then press enter to send your team a clue on which word to guess\n";
char *cn_waiting_turn_msg = "While you wait for your team's turn, you can type messages to your teammates\n";
char *cn_guess_recieved_msg = "Your guess has been recieved, it currently is: ";
char *cn_new_clue = "The spymaster clue is: ";

char *get_lobby_list_str() {
    return NULL;
}

#define CN_START 0
#define CN_PICK_PLAYER_TYPE 1
#define CN_PRINT_BOARD 2
#define CN_YOUR_TURN 3
#define CN_NOT_YOUR_TURN 4

#define CN_PLAYER 0
#define CN_SPYMASTER 1

#define CN_RED_TEAM 0
#define CN_BLUE_TEAM 1

#define CN_CLUE_UPDATE 1

ssize_t write_str2(int socket, char *buf, char *id, struct lobby *l) {
    ssize_t out = write_str(socket, buf);
    // TODO: disconnects still aren't easy...
    if (out == -1 && errno == EPIPE) {
        // take out of lobby (if not null)
        remove_player(l, id, 0);
        // take out of master lobby
        remove_player(&master_lobby, id, 1);
    }
    return out;
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
            write_str2(socket, cn_start_msg, id, lobby);
            new_checkpt = CN_PICK_PLAYER_TYPE;
            break;
        case CN_PICK_PLAYER_TYPE:
            if (prev_checkpt != CN_PICK_PLAYER_TYPE) {
                write_str2(socket, cn_pick_player_msg, id, lobby);
            }
            if (buf != NULL) {
                if (strcmp(buf, "1") == 0) {
                    new_player->player_type = CN_PLAYER;
                    new_checkpt = CN_PRINT_BOARD;
                } else if (strcmp(buf, "2") == 0) {
                    new_player->player_type = CN_SPYMASTER;
                    new_checkpt = CN_PRINT_BOARD;
                } else {
                    write_str2(socket, cn_retry_msg, id, lobby);
                }
            }
            break;
        case CN_PRINT_BOARD:
            if (prev_checkpt != CN_PRINT_BOARD) {
                char *board = get_board_str(lobby->r, lobby->c, lobby->board);
                // TODO: add score to top string
                char *top_1 = new_player->team == CN_RED_TEAM ? "Red Team, " : "Blue Team, ";
                char *top_2 = lobby->turn == CN_RED_TEAM ? "Red's Turn\n" : "Blue's Turn\n";
                write_str2(socket, top_1, id, lobby);
                write_str2(socket, top_2, id, lobby);
                write_str2(socket, board, id, lobby);
                write_str2(socket, "\n", id, lobby);
                free(board);
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
                    write_str2(socket, cn_player_turn_msg, id, lobby);
                }
                if (buf != NULL && buf[0] == '!') {
                    // parse !<word>
                    char *guess = malloc(sizeof(char) * strlen(buf + 1) + 1);
                    strcpy(guess, buf + 1);
                    if (new_player->cur_guess != NULL) {
                        free(new_player->cur_guess);
                    }
                    new_player->cur_guess = guess;
                    write_str2(socket, cn_guess_recieved_msg, id, lobby);
                    write_str2(socket, guess, id, lobby);
                    write_str2(socket, "\n", id, lobby);
                    printf("Guess: %s\n", new_player->cur_guess);
                } else if (buf != NULL) {
                    // team chat
                    chat_flag = 1;
                }
            } else if(new_player->player_type == CN_SPYMASTER) {
                // initial message spymaster
                if (prev_checkpt != CN_YOUR_TURN) {
                    write_str2(socket, cn_spymaster_turn_msg, id, lobby);
                }
                if (buf != NULL && lobby->cur_clue == NULL) {
                    size_t len = strlen(buf);
                    // parse <word>,<number> (word must be at least 4 characters, number is > 0 and <= 9)
                    if (len < 6 || buf[len - 2] != ',' || buf[len - 1] > 57 || buf[len - 1] < 49) {
                        write_str2(socket, cn_retry_msg, id, lobby);
                    } else {
                        // put clue into lobby
                        char *clue = malloc(sizeof(char) * strlen(buf) + 1);
                        strcpy(clue, buf);
                        lobby->cur_clue = clue;
                        lobby->update_flag |= CN_CLUE_UPDATE;
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
                write_str2(socket, cn_waiting_turn_msg, id, lobby);
            }
            // team chat
            chat_flag = 1;
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
        // transmit player chat (if any...)
        if (p->chat_buffer != NULL) {
            for (size_t j = 0; j < lobby->player_len; j++) {
                struct player *p2 = lobby->player[j];
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
                    write_str2(p2->socket, full_chat, p2->id, lobby);
                    free(full_chat);
                }
            }
            free(p->chat_buffer);
            p->chat_buffer = NULL;
        }
        // display spymaster guess if available
        if (lobby->update_flag && CN_CLUE_UPDATE > 0) {
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
            // TODO: find string in board, remove it and figure out score
        }
        for (size_t i = 0; i < lobby->player_len; i++) {
            struct player *p = lobby->player[i];
            free(p->cur_guess);
            p->cur_guess = NULL;
            // reprint boards for everyone
            if (next_turn) {
                p->checkpt = CN_PRINT_BOARD;
            }
        }
    }
}

void new_user(int new_socket, char *client_id, struct sockaddr_in addr) {
    if (lobbys == NULL) {
        struct lobby l;
        l.name = "first lobby";
        l.player_len = 0;
        uint8_t r = 4, c = 6;
        char ***board = gen_board(r, c);
        for (int i = 0; i < r; i++) {
            for (int j = 0; j < c; j++) {
                board[i][j] = "seventeen";
            }
        }
        l.r = r;
        l.c = c;
        l.board = board;
        l.turn = CN_RED_TEAM;
        l.cur_clue = NULL;
        l.player = NULL;
        l.player_len = 0;
        l.update_flag = 0;
        lobbys = malloc(sizeof(struct lobby));
        lobbys[0] = l;
        lobby_len++;
        master_lobby.player_len = 0;
    }
    
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
        p->lobby = &lobbys[0];
        p->team = CN_RED_TEAM;
        p->cur_guess = NULL;
        p->chat_buffer = NULL;
        add_player(&master_lobby, p);
        add_player(&lobbys[0], p);
    }

    write_str(new_socket, welcome_msg);

    // TODO: matchmaking
    struct lobby player_lobby = lobbys[0];
    char *lobby_str = get_lobby_str(&player_lobby);
    write_str(new_socket, lobby_str);
    free(lobby_str);
}

void event_loop() {

    for (size_t i = 0; i < master_lobby.player_len; i++) {
        struct player *current_player = master_lobby.player[i];
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
            }
        }
        if (read_len > 0) {
            buf[read_len - 1] = '\x00'; // TODO: test with \n and \r\n
            buf[read_len - 2] = '\x00';
            printf("Recieved from %s: %s\n", current_player->id, buf);
            // parse input based on player's state
            codenames_states(current_player, buf);
        } else {
            codenames_states(current_player, NULL);
        }

        free(buf);
    }

    // run the background game rules
    for (size_t i = 0; i < lobby_len; i++) {
        codenames_game_rules(&lobbys[i]);
    }
}

int main(int argc, char *argv[]) {
    socket_init();
    socket_poll(event_loop);
    return 0;
}
