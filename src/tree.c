#include "../include/tree.h"

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
        n->rh = max(temp->rh, temp->lh) + 1;
    } else {
        n->rh = 0;
    }
    b->lh = max(n->lh, n->rh) + 1;
    b->rh = max(b->r->rh, b->r->lh) + 1;
    if (b->p != NULL) {
        if (b->p->r == n) {
            b->p->r = b;
        } else {
            b->p->l = b;
        }
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
        n->lh = max(temp->rh, temp->lh) + 1;
    } else {
        n->lh = 0;
    }
    b->rh = max(n->lh, n->rh) + 1;
    b->lh = max(b->l->rh, b->l->lh) + 1;
    if (b->p != NULL) {
        if (b->p->r == n) {
            b->p->r = b;
        } else {
            b->p->l = b;
        }
    }
    return b;
}

struct tree *rotate_lr(struct tree *n) {
    // a to left
    struct tree *b = n->l->r;
    struct tree *tempr = b->r;
    struct tree *templ = b->l;
    b->l = n->l;
    b->l->p = b;
    b->l->r = templ;
    if (templ != NULL) {
        templ->p = b->l;
    }
    // main nodes parent
    b->p = n->p;
    n->p = b;
    // main node to right
    b->r = n;
    n->l = tempr;
    if (tempr != NULL) {
        tempr->p = n;
    }
    // adjust heights
    n->lh = 0;
    b->l->rh = 0;
    b->rh = n->rh + 1;
    b->lh = b->l->lh + 1;
    if (b->p != NULL) {
        if (b->p->r == n) {
            b->p->r = b;
        } else {
            b->p->l = b;
        }
    }
    return b;
}

struct tree *rotate_rl(struct tree *n) {
    // c to right
    struct tree *b = n->r->l;
    struct tree *tempr = b->r;
    struct tree *templ = b->l;
    b->r = n->r;
    b->r->p = b;
    b->r->l = tempr;
    if (tempr != NULL) {
        tempr->p = b->r;
    }
    // a to left
    b->l = n;
    // main nodes parent
    b->p = n->p;
    n->p = b;
    n->r = templ;
    if (templ != NULL) {
        templ->p = n;
    }
    // adjust heights
    n->rh = 0;
    b->r->lh = 0;
    b->lh = n->lh + 1;
    b->rh = b->r->rh + 1;
    if (b->p != NULL) {
        if (b->p->r == n) {
            b->p->r = b;
        } else {
            b->p->l = b;
        }
    }
    return b;
}

struct tree *avl_rebalance(struct tree *n, struct tree *t) {
    struct tree *c1 = NULL;
    struct tree *c2 = NULL;

    // n = current node
    // t = tree root
    // rebalance ancestors
    while (n != NULL) {
        printf("| %s, ", n->key);
        int8_t r = 0;
        int8_t l = 0;
        if (n->l != NULL) {
            l = max(n->l->lh, n->l->rh) + 1;
        }
        if (n->r != NULL) {
            r = max(n->r->lh, n->r->rh) + 1;
        }
        n->lh = l;
        n->rh = r;
        int8_t h = l - r;
        printf("lh = %d rh = %d h = %d ", l, r, h);
        if (h >= 2) {
            if (n->l->r == c2) {
                // left right rotation
                printf("LR ROTATE ");
                n = rotate_lr(n);
            } else {
                // right rotation
                printf("R ROTATE ");
                n = rotate_r(n);
            }
        } else if (h <= -2) {
            if (n->r->l == c2) {
                // right left rotation
                printf("RL ROTATE ");
                n = rotate_rl(n);
            } else {
                // left rotation
                printf("L ROTATE ");
                n = rotate_l(n);
            }
        }
        if (n->p == NULL) {
            t = n;
        }
        printf("END KEY: %s ", n->key);
        c2 = c1;
        c1 = n;
        n = n->p;
    }
    printf("\nREBALANCE DONE\n\n\n\n");

    return t;
}

struct tree *avl_add(struct tree *t, char *key, void *data) {
    struct tree *n = avl_make(key, data);

    if (t == NULL) {
        return n;
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

    return avl_rebalance(n, t);
}

int avl_del(struct tree **t, char *key) {
    // remove from tree as normal
    struct tree *n = avl_search(*t, key);
    if (n == NULL) {
        printf("CANNOT FIND DEL\n");
        return -1;
    }
    printf("DEL: %s\n", n->key);
    struct tree **side = NULL;
    if (n->p != NULL) {
        side = n->p->l == n ? &(n->p->l) : &(n->p->r);
    }
    if (n->l == NULL && n->r == NULL) {
        // no children
        // just remove it from the tree
        // TODO: balance factors
        printf("no children\n");
        *side = NULL;
    } else if (n->l == NULL) {
        // right child only
        // replace w/ right child
        // TODO: balance factors
        printf("right child\n");
        if (side == NULL) {
            *t = n->r;
        } else {
            *side = n->r;
        }
    } else if (n->r == NULL) {
        // left child only
        // TODO: balance factors
        printf("left child\n");
        if (side == NULL) {
            *t = n->l;
        } else {
            *side = n->l;
        }
    } else {
        // both children
        printf("both children\n");
        // find in-order successor
        struct tree *ios = n;
        while 
        // replace the in-order successor with its right child (if needed)
        // replace this node with in-order successor
        // balance factors
    }
    n->p = NULL;
    *t = avl_rebalance(n, *t);
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

struct tree *avl_search(struct tree *t, char *key) {
    struct tree *p = t;
    while (p != NULL) {
        int cmp = strcmp(key, p->key);
        if (cmp > 0) {
            p = p->r;
        } else if (cmp < 0) {
            p = p->l;
        } else {
            return p;
        }
        if (p == NULL) {
            return NULL;
        }
    }
    return NULL;
}

void test() {
    /*
    struct tree *t = avl_make("c", NULL);
    t = avl_add(t, "a", NULL);
    t = avl_add(t, "b", NULL);
    t = avl_add(t, "d", NULL);
    t = avl_add(t, "e", NULL);
    */

    //printf("%s\n", avl_search(t, "c")->key);

    struct tree *t = NULL;
    char *test = "qazwsxedcrfv";
    for (int i = 0; i < strlen(test); i++) {
        char *key = malloc(2 * sizeof(char));
        strncpy(key, &test[i], 1);
        printf("Added: %s\n", key);
        t = avl_add(t, key, NULL);
    }

    for (int i = 0; i < strlen(test); i++) {
        char *key = malloc(2 * sizeof(char));
        strncpy(key, &test[i], 1);
        printf("%s ", avl_search(t, key)->key);
    }

    printf("\n");

    for (int i = 0; i < strlen(test); i++) {
        char *key = malloc(2 * sizeof(char));
        strncpy(key, &test[i], 1);
        avl_del(&t, key);
    }

    printf("DONE\n");
}