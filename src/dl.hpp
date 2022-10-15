#ifndef DL_HPP
#define DL_HPP

#include <stdlib.h>

#define DL_HEAD(name) dl_head name = {&name, &name} 

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

#define dl_for_each_entry(pos, head, member) \
	for ( \
		pos = list_entry((head)->next, typeof(*pos), member); \
		&pos->member != (head); \
		pos = list_entry(pos->member.next, typeof(*pos), member) \
	)

struct dl_head;

struct dl_head {
	dl_head *next;
	dl_head *prev;
};

inline void dl_init(dl_head *head)
{
	head->next = head; 
	head->prev = head;
}

inline bool dl_empty(dl_head *head)
{
	return head->next == head;
}

inline void dl_add(dl_head *next, dl_head *prev, dl_head *n)
{
	next->prev = n;
	n->next = next;
	n->prev = prev;
	prev->next = n;
}

inline void dl_push_front(dl_head *head, dl_head *n) 
{
	dl_add(head->next, head, n);
}

inline void dl_push_back(dl_head *head, dl_head *n)
{
	dl_add(head, head->prev, n);
}

inline void dl_del(dl_head *next, dl_head *prev)
{
	next->prev = prev;
	prev->next = next;
}

inline void dl_remove(dl_head *n) 
{
	dl_del(n->next, n->prev);	
	n->next = NULL;
	n->prev = NULL;
}

inline dl_head *dl_pop_front(dl_head *head)
{
	struct dl_head *n;

	if (dl_empty(head)) {
		return NULL;
	}

	n = head->next;
	dl_remove(head->next);
	return n;
}

inline dl_head *dl_pop_back(dl_head *head)
{
	struct dl_head *n;

	if (dl_empty(head)) {
		return NULL;
	}

	n = head->prev;
	dl_remove(head->prev);
	return n;
}

#endif
