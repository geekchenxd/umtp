#ifndef __LIST_H__
#define __LIST_H__

/*
 * This is a simple doubly linked list implementation that matches the
 * way the linux kenel double linked list implementation works.
 */
struct list_head {
	struct list_head *next, *prev;
};

/**
 * list_is_first -- tests whether @list is the first entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int list_is_first(const struct list_head *list,
		const struct list_head *head)
{
	return list->prev == head;
}

/**
 * list_is_last -- tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int list_is_last(const struct list_head *list,
		const struct list_head *head)
{
	return list->next == head;
}

/**
 * Define and initialize the list as an empty list.
 */
#define LIST_HEAD_INIT(name){ &(name), &(name) }
#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */	
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })
#endif

/**
 * list_entry - get the struct for this entry
 * @ptr:the $struct list_head pointer.
 * @type:the type of the struct this is embedded in.
 *@member:the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) container_of(ptr, type, member)

/**
 * list_first_entry - get the first element form a list
 * @ptr:the list head to take the element from.
 * @type:the type of the struct this is embedded in.
 *@member:the name of the list_struct within the struct.
 */
#define list_first_entry(ptr, type, member)\
	list_entry((ptr)->next, type, member)

/**
 * list_last_entry - get the last element form a list
 * @ptr:the list head to take the element from.
 * @type:the type of the struct this is embedded in.
 *@member:the name of the list_struct within the struct.
 */
#define list_last_entry(ptr, type, member)\
	list_entry((ptr)->prev, type, member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos: the &struct list_head to use as a loop cursor.
 * n:	another &struct list_head to use as temporary storage
 * @head: the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head);\
			pos = n, n = pos->next)

/**
 * list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos: the &struct list_head to use as a loop cursor.
 * n:	another &struct list_head to use as temporary storage
 * @head: the head for your list.
 */
#define list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
			pos != (head);\
			pos = n, n = pos->prev)

void INIT_LIST_HEAD(struct list_head *list);
void list_add(struct list_head *new, struct list_head *head);
void list_add_tail(struct list_head *new, struct list_head *head);
int list_empty(const struct list_head *head);
void list_del(struct list_head *entry);


#endif
