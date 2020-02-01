#ifndef __HOOK__H__
#define __HOOK__H__
#include "list.h"

#define MAX_HOOK_PRIORITY 3

enum {
	HOOK_LINK_LAYER = 0,
	HOOK_NETWORK_LAYER = 1,
	HOOK_APPLICTION_LAYER = 2,
	HOOK_MAX_LAYER = 3
};

struct hook_priority {
	int pri_flag;
	struct list_head head[MAX_HOOK_PRIORITY];
};

struct umtp_hook {
	struct list_head list;
	bool in_and_out;
	int priority;
	int hook_point;
	int (*hook_func)(uint8_t *data, int data_len, bool dir);
};

void umtp_hook_init(void);
int umtp_hook_register(struct umtp_hook *hook);
int umtp_hook_unregister(struct umtp_hook *hook);
int umtp_do_hooks(int hook_layer, uint8_t *data,
		int data_len, bool dir);

#endif
