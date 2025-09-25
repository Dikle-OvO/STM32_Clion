#ifndef _LIST_H_
#define _LIST_H_

#include <stddef.h>

// Define the list_head structure
struct list_head {
    struct list_head *next, *prev;
};

// Initialize a list head
#define LIST_HEAD_INIT(name) {&(name), &(name)}
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)
#define INIT_LIST_HEAD(ptr)  \
    do {                     \
        (ptr)->next = (ptr); \
        (ptr)->prev = (ptr); \
    } while (0)

// Add a new entry
static inline void list_add(struct list_head *new, struct list_head *head)
{
    new->next = head->next;
    new->prev = head;
    head->next->prev = new;
    head->next = new;
}

// Add a new entry to the tail
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    new->prev = head->prev;
    new->next = head;
    head->prev->next = new;
    head->prev = new;
}

// Delete a list entry
static inline void list_del(struct list_head *entry)
{
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    entry->next = entry->prev = NULL;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

// Iterate over the list
#define list_for_each(pos, head) for (pos = (head)->next; pos != (head); pos = pos->next)

// Iterate safely over the list
#define list_for_each_safe(pos, n, head) for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

// Get the struct for this entry
#define list_entry(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

//  list_for_each_entry	-	iterate over list of given type
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, __typeof__(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.next, __typeof__(*pos), member))

#endif // _LIST_H_
