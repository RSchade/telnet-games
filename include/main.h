#ifndef __MAIN_H
#define __MAIN_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>
#include <signal.h>
#include <time.h>
#include <signal.h>
#include "lobby.h"
#include "board.h"
#include "net.h"

#define CN_START 0
#define CN_PICK_PLAYER_TYPE 1
#define CN_PRINT_BOARD 2
#define CN_YOUR_TURN 3
#define CN_NOT_YOUR_TURN 4
#define CN_GAME_END 5

#define CN_PLAYER 0
#define CN_SPYMASTER 1

#define CN_RED_TEAM 0
#define CN_BLUE_TEAM 1

#define CN_CLUE_UPDATE 1
#define CN_ASSASSIN_LOSS 2

#define LOBBY_DECIDE 0
#define LOBBY_PROMPT 1

#define BEGIN 0
#define CN_GAME 1

void event_loop();
void new_user(int new_socket, char *client_id, struct sockaddr_in addr);

#endif