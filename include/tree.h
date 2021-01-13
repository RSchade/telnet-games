#ifndef __TREE_H
#define __TREE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct tree {
    void *data;
    char *key;
    struct tree *l;
    struct tree *r;
    struct tree *p;
    int8_t lh;
    int8_t rh;
};

struct tree *avl_search(struct tree *t, char *key);
struct tree *avl_add(struct tree *t, char *key, void *data);
int avl_del(struct tree *t, char *key);
struct tree *avl_make(char *key, void *data);

void test();

#endif