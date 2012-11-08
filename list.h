#ifndef __LIST_H__
#define __LIST_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct __list_node {
    int data;
    struct __list_node *next;
};

typedef struct {
    struct __list_node *head;
		pthread_mutex_t lock;
} list_t;

void list_init(list_t *);
void list_clear(list_t *);
void list_enqueue(list_t *, int);
void list_print(list_t *);
int list_size(list_t *);
int list_dequeue(list_t *);

#endif // __LIST_H__
