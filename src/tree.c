#include "../include/tree.h"

struct tree *avl_search(struct tree *t, char *key) {
    return NULL;
}

struct tree *rotate_l(struct tree *x, struct tree *z) {

}

struct tree *rotate_r(struct tree *x, struct tree *z) {

}

struct tree *rotate_lr(struct tree *x, struct tree *z) {

}

struct tree *rotate_rl(struct tree *x, struct tree *z) {

}

struct tree *avl_add(struct tree *t, char *key, void *data) {
    struct tree *n = avl_make(key, data);

    if (t == NULL) {
        return NULL;
    }

    // add to tree as normal
    struct tree *p = t;
    struct tree *p2 = NULL;
    int cmp = 0;
    while (p != NULL) {
        p2 = p;
        cmp = strcmp(n->key, p->key);
        if (cmp > 0) {
            p = p->r;
        } else if (cmp < 0) {
            p = p->l;
        } else {
            // key collision
            return NULL;
        }
    }
    p = p2;
    n->p = p;

    if (cmp > 0) {
        p->r = n;
    } else {
        p->l = n;
    }

    // p = current node
    // rebalance ancestors

    return t;
}

int avl_del(struct tree *t, char *key) {
    return 0;
}

struct tree *avl_make(char *key, void *data) {
    struct tree *n = malloc(sizeof(struct tree));
    n->data = data;
    n->height = 0;
    n->key = key;
    n->l = NULL;
    n->r = NULL;
    n->p = NULL;
    return n;
}

void test() {
    struct tree *t = avl_make("d", NULL);
    avl_add(t, "c", NULL);
    avl_add(t, "b", NULL);
    avl_add(t, "a", NULL);
}