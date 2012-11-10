#include <pthread.h>
#include "list.h"

/* ************************************** 
 * initialize the list
 * ************************************** */
void list_init(list_t *list) {
    list->head = NULL;
		pthread_mutex_init(&list->lock,NULL);
}


/* ************************************** 
 * print the contents of the list
 * ************************************** */
void list_print(list_t *list) {
		pthread_mutex_lock(&list->lock);
    struct __list_node *tmp = list->head;
    while (tmp) {
        printf("%i\n", tmp->data);
        tmp = tmp->next;
    }
		pthread_mutex_unlock(&list->lock);
}

/* ************************************** 
 * add item "val" to the queue
 * ************************************** */
void list_enqueue(list_t *list, int val, char *ip, int port) {
		pthread_mutex_lock(&list->lock);
    struct __list_node *new_node = (struct __list_node *)malloc (sizeof(struct __list_node));
    if (!new_node) {
        fprintf(stderr, "No memory while attempting to create a new list node!\n");
        abort();
    }
		//assign data to new node
    new_node->data = val;
		new_node->ip = ip;
		new_node->port = port;
		//make new node new head
		new_node->next = list->head;
    list->head = new_node;
		pthread_mutex_unlock(&list->lock);
}


/* ************************************** 
 * Pop off an item from the queue
 * ************************************** */
struct __list_node * list_dequeue(list_t *list) {
		pthread_mutex_lock(&list->lock);
		struct __list_node *toreturn = (struct __list_node *)malloc (sizeof(struct __list_node));
    /* short cut: is the list empty? */
    if (list == NULL || list->head == NULL){
			pthread_mutex_unlock(&list->lock);
			//if stuff is broken, return -1
      return NULL;
		}
    struct __list_node *tmp = list->head;
		toreturn->data = tmp->data;
		toreturn->ip = tmp->ip;
		toreturn->port = tmp->port;	
		list->head = tmp->next;
		//free those nodes
		free(tmp);

		pthread_mutex_unlock(&list->lock);
    return toreturn;
}


/* ************************************** 
 * clear out the entire list, freeing all
 * elements.
 * ************************************** */
void list_clear(list_t *list) {
		pthread_mutex_lock(&list->lock);
    struct __list_node *tmp = list->head;
    while (tmp) {
        struct __list_node *tmp2 = tmp->next;
        free(tmp);
        tmp = tmp2;
    }
    list->head = NULL;
		pthread_mutex_unlock(&list->lock);
		pthread_mutex_destroy(&list->lock);
}

int list_size(list_t *list) {
	pthread_mutex_lock(&list->lock);
	int size = 0;
	struct __list_node *tmp = list->head;
  while (tmp) {
      size++;
			tmp = tmp->next;
	}
	pthread_mutex_unlock(&list->lock);
	return size;
}
