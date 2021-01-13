#include "../include/tree.h"

struct tree *avl_search(struct tree *t, char *key) {
    return NULL;
}

struct tree *rotate_l(struct tree *n) {
    // move B to root
    struct tree *b = n->r;
    b->p = n->p;
    n->p = b;
    struct tree *c = b->r;
    // get left subtree of b
    struct tree *temp = b->l;
    // move A to the left of B
    b->l = n;
    // move C to the right of B
    b->r = c;
    // adjust parent
    c->p = b;
    n->r = temp;
    // adjust heights
    if (temp != NULL) {
        n->rh = temp->rh + temp->lh + 1;
    } else {
        n->rh = 0;
    }
    b->lh = n->lh + n->rh;
    b->rh = b->r->rh + b->r->lh + 1;
    if (b->p != NULL) {
        b->p->r = b;
    }
    return b;
}

struct tree *rotate_r(struct tree *n) {
    // move B to root
    struct tree *b = n->l;
    b->p = n->p;
    n->p = b;
    struct tree *c = b->l;
    // get right subtree of b
    struct tree *temp = b->r;
    // move A to the left of B
    b->r = n;
    // move C to the right of B
    b->l = c;
    // adjust parent
    c->p = b;
    n->l = temp;
    // adjust heights
    if (temp != NULL) {
        n->lh = temp->rh + temp->lh + 1;
    } else {
        n->lh = 0;
    }
    b->rh = n->lh + n->rh;
    b->lh = b->l->rh + b->l->lh + 1;
    if (b->p != NULL) {
        b->p->l = b;
    }
    return b;
}

struct tree *rotate_lr(struct tree *n) {
    // a to left
    struct tree *b = n->l->r;
    b->l = n->l;
    b->l->p = b;
    b->l->r = NULL;
    // main nodes parent
    b->p = n->p;
    n->p = b;
    // main node to right
    b->r = n;
    n->l = NULL;
    // adjust heights
    n->lh = 0;
    b->l->rh = 0;
    b->rh = n->rh + 1;
    b->lh = b->l->lh + 1;
    if (b->p != NULL) {
        b->p->r = b;
    }
    return b;
}

struct tree *rotate_rl(struct tree *n) {
    // c to right
    struct tree *b = n->r->l;
    b->r = n->r;
    b->r->p = b;
    b->r->l = NULL;
    // a to left
    b->l = n;
    // main nodes parent
    b->p = n->p;
    n->p = b;
    n->r = NULL;
    // adjust heights
    n->rh = 0;
    b->r->lh = 0;
    b->lh = n->lh + 1;
    b->rh = b->r->rh + 1;
    if (b->p != NULL) {
        b->p->l = b;
    }
    return b;
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
            printf("R ");
            p = p->r;
        } else if (cmp < 0) {
            printf("L ");
            p = p->l;
        } else {
            // key collision
            printf("COLLISION\n");
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

    printf("\nP: %s\n", p->key);
    printf("TRAVERSE DONE\n");

    // n = current node
    // rebalance ancestors
    while (n != NULL) {
        printf("| %s, ", n->key);
        int8_t r = 0;
        int8_t l = 0;
        if (n->l != NULL) {
            l = n->l->lh + n->l->rh + 1;
        }
        if (n->r != NULL) {
            r = n->r->lh + n->r->rh + 1;
        }
        n->lh = l;
        n->rh = r;
        printf("lh: %d rh: %d\n", l, r);
        int8_t h = l - r;
        printf("h = %d ", h);
        if (h >= 2) {
            if (n->l->r != NULL && n->l->l == NULL) {
                // left right rotation
                printf("LR ROTATE\n ");
                n = rotate_lr(n);
            } else {
                // right rotation
                printf("R ROTATE\n ");
                n = rotate_r(n);
            }
        } else if (h <= -2) {
            if (n->r->l != NULL && n->r->r == NULL) {
                // right left rotation
                printf("RL ROTATE\n ");
                n = rotate_rl(n);
            } else {
                // left rotation
                printf("L ROTATE\n ");
                n = rotate_l(n);
            }
        }
        if (n->p == NULL) {
            t = n;
        }
        printf("KEY: %s ", n->key);
        n = n->p;
    }
    printf("\nREBALANCE DONE\n\n\n\n");

    return t;
}

int avl_del(struct tree *t, char *key) {
    return 0;
}

struct tree *avl_make(char *key, void *data) {
    struct tree *n = malloc(sizeof(struct tree));
    n->data = data;
    n->lh = 0;
    n->rh = 0;
    n->key = key;
    n->l = NULL;
    n->r = NULL;
    n->p = NULL;
    return n;
}

void test() {
    struct tree *t = avl_make("c", NULL);
    t = avl_add(t, "a", NULL);
    t = avl_add(t, "b", NULL);
    t = avl_add(t, "d", NULL);
    t = avl_add(t, "e", NULL);

    printf("DONE\n");
}