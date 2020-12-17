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
char *cn_player_turn_msg = "Lines entered will be transmitted to your teammates, when you are ready to guess type an exclamation point (!) before a single word and press enter. Once a consensus is reached that word will be your team's guess\n";
char *cn_spymaster_turn_msg = "Enter in a word comma a digit (0-9) and then press enter to send your team a clue on which word to guess\n";
char *cn_waiting_turn_msg = "While you wait for your team's turn, you can type messages to your teammates\n";
char *cn_guess_recieved_msg = "Your guess has been recieved, it currently is: ";

char *get_lobby_list_str() {
    return NULL;
}

#define CN_START 0
#define CN_PICK_PLAYER_TYPE 1
#define CN_PRINT_BOARD 2
#define CN_TURN 3

#define CN_PLAYER 0
#define CN_SPYMASTER 1

#define CN_RED_TEAM 0
#define CN_BLUE_TEAM 1

void codenames_states(struct player *new_player, char *buf) {
    int socket = new_player->socket;
    uint8_t checkpt = new_player->checkpt;
    uint8_t new_checkpt = checkpt;
    uint8_t prev_checkpt = new_player->prev_checkpt;
    struct lobby *lobby = new_player->lobby;

    switch (checkpt) {
        case CN_START:
            write(socket, cn_start_msg, strlen(cn_start_msg));
            new_checkpt = CN_PICK_PLAYER_TYPE;
            break;
        case CN_PICK_PLAYER_TYPE:
            if (prev_checkpt != CN_PICK_PLAYER_TYPE) {
                write(socket, cn_pick_player_msg, strlen(cn_pick_player_msg));
            }
            if (buf != NULL) {
                if (strcmp(buf, "1") == 0) {
                    new_player->player_type = CN_PLAYER;
                    new_checkpt = CN_PRINT_BOARD;
                } else if (strcmp(buf, "2") == 0) {
                    new_player->player_type = CN_SPYMASTER;
                    new_checkpt = CN_PRINT_BOARD;
                } else {
                    write(socket, cn_retry_msg, strlen(cn_retry_msg));
                }
            }
            break;
        case CN_PRINT_BOARD:
            if (prev_checkpt != CN_PRINT_BOARD) {
                char *board = get_board_str(lobby->r, lobby->c, lobby->board);
                // TODO: add score to top string
                char *top_str = new_player->team == CN_RED_TEAM ? "Red Turn\n" : "Blue Turn\n";
                write(socket, top_str, strlen(top_str));
                write(socket, board, strlen(board));
                write(socket, "\n", 1);
                free(board);
            }
            new_checkpt = CN_TURN;
            break;
        case CN_TURN:
            if (new_player->team != lobby->turn && prev_checkpt != CN_TURN) {
                write(socket, cn_waiting_turn_msg, strlen(cn_waiting_turn_msg));
                // TODO: team chat
                break;
            }
            if (new_player->player_type == CN_PLAYER) {
                if (prev_checkpt != CN_TURN) {
                    write(socket, cn_player_turn_msg, strlen(cn_player_turn_msg));
                }
                // TODO: write the spymaster's guess
                if (buf != NULL && buf[0] == '!') {
                    // parse !<word>
                    char *guess = malloc(sizeof(char) * strlen(buf + 1) + 1);
                    strcpy(guess, buf + 1);
                    if (new_player->cur_guess != NULL) {
                        free(new_player->cur_guess);
                    }
                    // TODO: guess must be on the board
                    new_player->cur_guess = guess;
                    write(socket, cn_guess_recieved_msg, strlen(cn_guess_recieved_msg));
                    write(socket, guess, strlen(guess));
                    write(socket, "\n", strlen("\n"));
                    printf("Guess: %s\n", new_player->cur_guess);
                } else if (buf != NULL) {
                    // TODO: team chat
                }
            } else if(new_player->player_type == CN_SPYMASTER) {
                if (prev_checkpt != CN_TURN) {
                    write(socket, cn_spymaster_turn_msg, strlen(cn_spymaster_turn_msg));
                }
                if (buf != NULL) {
                    size_t len = strlen(buf);
                    // parse <word>,<number> (word must be at least 4 characters, number is > 0 and <= 9)
                    if (len < 6 || buf[len - 2] != ',' || buf[len - 1] > 57 || buf[len - 1] < 49) {
                        write(socket, cn_retry_msg, strlen(cn_retry_msg));
                    }
                    // TODO: put clue into lobby... have it get rendered
                }
            } else {
                printf("Invalid player type %d!\n", new_player->player_type);
            }
            break;
        default:
            printf("Invalid checkpoint %d!\n", checkpt);
            break;
    }

    new_player->prev_checkpt = new_player->checkpt;
    new_player->checkpt = new_checkpt;
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
        lobbys = malloc(sizeof(struct lobby));
        lobbys[0] = l;
        lobby_len++;
        master_lobby.player_len = 0;
    }
    
    struct player *client_player = get_player(&master_lobby, client_id);
    if (client_player == NULL) {
        struct player *p = malloc(sizeof(struct player));
        p->id = client_id;
        p->addr = addr;
        p->socket = new_socket;
        p->checkpt = 0;
        p->lobby = &lobbys[0];
        p->team = CN_RED_TEAM;
        p->cur_guess = NULL;
        add_player(&master_lobby, p);
    }

    write(new_socket, welcome_msg, strlen(welcome_msg));

    struct lobby player_lobby = lobbys[0];
    char *lobby_str = get_lobby_str(&player_lobby);
    write(new_socket, lobby_str, strlen(lobby_str));
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

    // TODO: for each codenames lobby, run the background game rules:
    // start the game when everyone is in (maybe we don't need to)
    // when all the guesses for the current team are in consensus then change the turn
    // team chat
    // display the guess from the spymaster when it is available
}

int main(int argc, char *argv[]) {
    socket_init();
    socket_poll(event_loop);
    return 0;
}

    /*struct lobby l;
    l.name = "test lobby";
    l.player = NULL;
    l.player_len = 0;
    struct player p1;
    p1.id = "abc";
    p1.name = "some name";
    struct player p2;
    p2.id = "def";
    p2.name = "some name 2";
    add_player(&l, &p1);
    add_player(&l, &p2);
    printf("in: %p, out: %p\n", &p1, get_player(&l, "abc"));
    char *lobby_str = get_lobby_str(&l);
    printf("%s\n", lobby_str);
    free(lobby_str);
    free(l.player);*/

    /*uint8_t r = 4, c = 6;
    char ***grid_text = gen_board(r, c);
    for (int i = 0; i < r; i++) {
        for (int j = 0; j < c; j++) {
            grid_text[i][j] = "seventeen";
        }
    }
    grid_text[2][3] = "test12";
    grid_text[0][1] = "test23";
    char *board = get_board_str(r, c, grid_text);

    write(new_socket, board, strlen(board));
    write(new_socket, "\n", 1);

    for (int i = 0; i < r; i++) {
        free(grid_text[i]);
    }
    free(grid_text);
    free(board);*/