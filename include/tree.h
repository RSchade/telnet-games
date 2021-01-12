#ifndef __TREE_H
#define __TREE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct tree {
    int8_t height;
    void *data;
    char *key;
    struct tree *l;
    struct tree *r;
    struct tree *p;
};

struct tree *avl_search(struct tree *t, char *key);
struct tree *avl_add(struct tree *t, char *key, void *data);
int avl_del(struct tree *t, char *key);
struct tree *avl_make(char *key, void *data);

void test();

#endif