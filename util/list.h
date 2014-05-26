/* minimaList
 * Author: Jordan Rife
 *
 * This is a minimal linked list implementation optimized for statically allocated structures.
 * Hence, it does not require any dynamic memory allocation. List noded contain no data except
 * that related to the linked list structure itself. This means that the list_node_t structure
 * should be embedded in another structure to turn it into a linkable structure. For example:
 * Say I have a struct defined as such:
 *
 * struct MyNormalStruct {
 *     int someBoringValue;
 *     char anotherBoringValue;
 * };
 *
 * and I want to make a linked list of these. I would just embed a list_node_t structure into the
 * MyNormalStruct structure like so:
 *
 * struct MyNormalStruct {
 *     list_node_t _my_linking_info;
 *     int someBoringValue;
 *     char anotherBoringValue;
 * };
 * 
 * By doing this, MyNormalStruct effectively becomes a list_node_t struct as well. This is a sort
 * of primitive polymorphism. MyNormalStruct has become a 'subclass' of list_node_t and can be 
 * passed into the list_* funcitons to manipulate the linked list. For examples on how to do
 * the various operations, see list_examples.c
 *
 * IMPORTANT!!!!: list_node_t must be the FIRST member of the struct for this to work.
 */

#include "util/macros.h"

#ifndef LIST_H
#define LIST_H

typedef struct list_node_t list_node_t;
typedef struct list_t list_t;

struct list_node_t {
	list_node_t* next;
	list_node_t* prev;
	list_t* owner;
};

struct list_t {
	list_node_t* head;
	UINT size;
};

void list_init(list_t* list);
void list_move(list_t* destination, void* node, void* pivot, void (*addStrategy)(list_t*,void*,void*));
void list_append(list_t* list, void* node, void* pivot);
void list_prepend(list_t* list, void* node, void* pivot);
void list_unlink(void* node);
UINT list_size(list_t* list);
list_node_t* list_next(void* node);
list_node_t* list_prev(void* node);
list_node_t* list_head(list_t* list);

#endif
