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
        printf("%s\n", tmp->data);
        tmp = tmp->next;
    }
		pthread_mutex_unlock(&list->lock);
}


/* ************************************** 
 * add item "val" to the list
 * ************************************** */
void list_add(list_t *list, const char *val) {
		pthread_mutex_lock(&list->lock);
    struct __list_node *new_node = (struct __list_node *)malloc (sizeof(struct __list_node));
    if (!new_node) {
        fprintf(stderr, "No memory while attempting to create a new list node!\n");
        abort();
    }
		//assign data to new node
    new_node->data = (char *)val;
		//make new node new head
		new_node->next = list->head;
    list->head = new_node;
		pthread_mutex_unlock(&list->lock);
}


/* ************************************** 
 * Remove the item "target" from the list
 * ************************************** */
void list_remove(list_t *list, const char *target) {
		pthread_mutex_lock(&list->lock);
    /* short cut: is the list empty? */
    if (list == NULL || list->head == NULL){
			pthread_mutex_unlock(&list->lock);
      return;
		}
    struct __list_node *tmp = list->head;
		//take care of case where head is deleted
		while(tmp != NULL && strcmp(tmp->data, target) == 0){
			list->head = tmp->next;
			//free those nodes
			struct __list_node *tmp2 = tmp;
			free(tmp2);
			tmp = list->head;
		}
		//take care of everything else
    while (tmp != NULL && tmp->next != NULL) {
				if(strcmp(tmp->next->data, target) == 0){
					struct __list_node *tmp2 = tmp->next;
					tmp->next = tmp->next->next;
					//free node
					free(tmp2);
				}else{
		     	tmp = tmp->next;
				}
    }

		pthread_mutex_unlock(&list->lock);
    return;
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
				free(tmp->data);
        free(tmp);
        tmp = tmp2;
    }
    list->head = NULL;
		pthread_mutex_unlock(&list->lock);
		pthread_mutex_destroy(&list->lock);
}

