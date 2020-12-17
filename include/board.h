#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

char *get_board_str(uint8_t r, uint8_t c, char ***grid_text);
char ***gen_board(uint8_t r, uint8_t c);