#ifndef DL_HPP
#define DL_HPP

#include <stdlib.h>

/**
 * DL_HEAD(name) - Declare and initalize global list
 * @name: Variable name of list
 *
 * Add static keyword before macro to declare static global
 * variable.
 */
#define DL_HEAD(name) dl_head name = {&name, &name}

/**
 * container_of() - Get containing structure
 * @ptr: Pointer to member
 * @type: Containing structure
 */
#define container_of(ptr, type, member) \
	((type *) ((char *) ptr - offsetof(type, member)))

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define dl_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define dl_for_each_s(pos, n, head) \
	for (pos = (head)->next; n = pos->next, pos != (head); pos = n)

#define dl_for_each_entry_s(pos, n, head, member) \
	for ( \
		pos = list_entry((head)->next, typeof(*pos), member); \
		n = list_entry(pos->member.next, typeof(*pos), member), \
		&pos->member != (head); \
		pos = n \
	)
/**
 * dl_for_each_entry() - Iterator over entries of list
 * @pos: Variable to use as iterator
 * @head: Head of list
 * @member: Member of each entry that reprsents the node 
 */
#define dl_for_each_entry(pos, head, member) \
	for ( \
		pos = list_entry((head)->next, typeof(*pos), member); \
		&pos->member != (head); \
		pos = list_entry(pos->member.next, typeof(*pos), member) \
	)

struct dl_head;

/**
 * struct dl_head - Head and node struct of doubly linked list
 * @next: Either the next node or the first node of a list
 * @prev: Either the previous node or the last node of a list
 *
 * In case of empty list, the head will point back to itself.
 */
struct dl_head {
	dl_head *next;
	dl_head *prev;
};

/**
 * dl_init() - Initialize head 
 *
 * Creates empty list.
 */
inline void dl_init(dl_head *head)
{
	head->next = head;
	head->prev = head;
}

/**
 * dl_empty() - Check if list is empty
 * @head: Head of list to check
 */
inline bool dl_empty(dl_head *head)
{
	return head->next == head;
}

/**
 * dl_add() - Add node to list
 * @next: Node that will be after the new node 
 * @prev: Node that will be before the new node
 * @n: Node that will be added
 *
 * NOTE: Only used as helper for other functions
 */
inline void dl_add(dl_head *next, dl_head *prev, dl_head *n)
{
	next->prev = n;
	n->next = next;
	n->prev = prev;
	prev->next = n;
}

/**
 * dl_push_front() - Add node after head
 * @head: Head to add after
 * @n: Node to add
 */
inline void dl_push_front(dl_head *head, dl_head *n)
{
	dl_add(head->next, head, n);
}

/**
 * dl_push_back() - Add node before head. 
 * @head: Head to add before
 * @n: Node to add
 */
inline void dl_push_back(dl_head *head, dl_head *n)
{
	dl_add(head, head->prev, n);
}

/**
 * dl_del() - Detach node from list
 * @n: Node to remove
 *
 * Pointers of node are NOT modfied.
 * Only pointers of adjacent nodes
 * are modified.
 */
inline void dl_del(dl_head *n)
{
	dl_head *next, *prev;

	next = n->next;
	prev = n->prev;
	next->prev = prev;
	prev->next = next;
}

#endif
