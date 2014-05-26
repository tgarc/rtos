/* minimaList
 * Author: Jordan Rife
 */

#include "list.h"

/*
 * This should be called on a list_t type
 * before any operations are performed on it
 *
 * list - a pointer to a list_t structure to be initialized
 *
 * preconditions: none
 * postconditions: list size is set to zero and pointers are set to NULL
 */
void list_init(list_t* list) {
	list->head = NULL;
	list->size = 0;
}

/*
 * This moves a list node from one list to another,
 * placing the node in the new list using the given strategy and pivot
 *
 * destination - list to move the node to
 * node - node that will be moved
 * pivot - used with addStrategy to determine where to place the node in its
 *     new list (See append for more detail on pivot)
 * addStrategy - this can be list_append or list_prepend. If NULL, it defaults
 *     to list_append.
 *
 * preconditions: node must have been added to a list already, pivot must be
 *     a member of the list which you are moving the node to. destination
 *     must be the list to which pivot belongs, if pivot is used
 * postconditions: node is removed from its original list and added to the 
 *     destination list using the given strategy
 */
void list_move(list_t* destination, void* node, void* pivot, void (*addStrategy)(list_t*,void*,void*)) {
	list_unlink(node);
	
	if(addStrategy == NULL) {
		addStrategy = list_append;
	}

	addStrategy(destination, node, pivot);
}

/*
 * Appends the given list node to the given list after the
 * specified pivot
 *
 * list - list to append to
 * node - node to append to the list
 * pivot - if this is specified, node is inserted directly after this node
 *     in the list. If NULL, then node will be appended at the end of the list
 *
 * preconditions: If pivot is not null, it must be a member of list.
 * postconditions: node is inserted into the list directly after pivot or
 *     at the end of the list if pivot is not specified
 */
void list_append(list_t* list, void* node, void* pivot) {
	list_node_t* listNode = (list_node_t*)node;
	list_node_t* pivotNode = (list_node_t*)pivot;

	if(list->size == 0) {
		list->head = listNode;

		list->head->next = list->head;
		list->head->prev = list->head;
	}
	else {
		if(pivotNode == NULL) {
			pivotNode = list->head->prev;
		}

		// link new node
		listNode->next = pivotNode->next;
		listNode->prev = pivotNode;

		// add it in
		pivotNode->next->prev = listNode;
		pivotNode->next = listNode;

	}

	listNode->owner = list;
	list->size++;
}

/*
 * See list_append. This works similarly to that except that things
 * are inserted directly before the given pivot and if pivot is NULL
 * the node is inserted at the beginning of the list, becoming the new
 * head
 */
void list_prepend(list_t* list, void* node, void* pivot) {
	if(pivot == NULL) {
		pivot = list->head;
	}

	list_append(list, node, list_prev(pivot));

	if(pivot == list->head) {
		list->head = (list_node_t*)node;
	}
}

/*
 * Removes a node from its list
 * 
 * node - node to unlink from its list
 *
 * preconditions: node must have been added to a list
 * postconditions: the owning list no longer contains node
 */
void list_unlink(void* node) {
	list_node_t* listNode = (list_node_t*)node;
	list_t* list = listNode->owner;

	if(list->size != 0) {
		if(listNode == list->head) {
			list->head = list->head->next;
		}

		listNode->owner = NULL;
		listNode->prev->next = listNode->next;
		listNode->next->prev = listNode->prev;

		list->size--;
	}
}

/*
 * The following are just accessors
 */
UINT list_size(list_t* list) {
	return list->size;
}

list_node_t* list_next(void* node) {
	return ((list_node_t*)node)->next;
}

list_node_t* list_prev(void* node) {
	return ((list_node_t*)node)->prev;
}

list_node_t* list_head(list_t* list) {
	return list->head;
}
