#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "hook.h"
#include "debug.h"

static unsigned int hook_layer_flag = 0;
static struct hook_priority hooks[HOOK_MAX_LAYER];
static bool init_done = false;

void umtp_hook_init(void)
{
	int i, j;
	struct hook_priority *hp;

	for (i = 0; i < HOOK_MAX_LAYER; i++) {
		hp = &hooks[i];
		hp->pri_flag = 0;
		for (j = 0; j < MAX_HOOK_PRIORITY; j++)
			INIT_LIST_HEAD(&hp->head[j]);
	}

	init_done = true;
}

static bool has_hooks(int hook_layer)
{
	if (hook_layer >= HOOK_MAX_LAYER)
		return false;

	return (hook_layer_flag & (1 << hook_layer));
}

int umtp_do_hooks(int hook_layer, uint8_t *data,
		int data_len, bool dir)
{
	struct hook_priority *hp;
	struct umtp_hook *hook;
	int i;
	int ret;
	struct list_head *pos;

	if (!has_hooks(hook_layer))
		return 0;
	if (!data || data_len <= 0)
		return 0;

	hp = &hooks[hook_layer];

	for (i = 0; i < MAX_HOOK_PRIORITY; i++) {
		if (!((1 << i) & hp->pri_flag))
			continue;
		list_for_each(pos, &hp->head[i]) {
			hook = list_entry(pos, struct umtp_hook, list);
			if (dir != hook->in_and_out)
				continue;
			ret = hook->hook_func(data, data_len, dir);
			if (ret)
				return ret;
		}
	}

	return ret;
}

static bool hook_is_registered(struct umtp_hook *hook)
{
	struct hook_priority *hp;
	struct umtp_hook *tmp;
	struct list_head *pos;
	int i;

	hp = &hooks[hook->hook_point];
	for (i = 0; i < MAX_HOOK_PRIORITY; i++) {
		list_for_each(pos, &hp->head[i]) {
			tmp = list_entry(pos, struct umtp_hook, list);
			if (tmp == hook)
				return true;
		}
	}

	return false;
}

int umtp_hook_register(struct umtp_hook *hook)
{
	struct hook_priority *hp;
	struct list_head *hk_head;

	if (!init_done)
		umtp_hook_init();

	if (!hook)
		return -EINVAL;
	if (hook->priority > MAX_HOOK_PRIORITY ||
			hook->hook_point >= HOOK_MAX_LAYER ||
			!hook->hook_func) {
		debug(ERROR, "hook priority, hook point or hook function invalid\n");
		return -EINVAL;
	}

	if (hook_is_registered(hook))
		return -EEXIST;

	hp = &hooks[hook->hook_point];
	hook_layer_flag |= (1 << hook->hook_point);
	hp->pri_flag |= (1 << hook->priority);

	hk_head = &hp->head[hook->priority];
	list_add_tail(hk_head, &hook->list);

	return 0;
}

int umtp_hook_unregister(struct umtp_hook *hook)
{
	struct hook_priority *hp;

	if (!hook)
		return -EINVAL;
	if (hook->priority > MAX_HOOK_PRIORITY ||
			hook->hook_point >= HOOK_MAX_LAYER ||
			!hook->hook_func) {
		debug(ERROR, "hook priority, hook point or hook function invalid\n");
		return false;
	}

	if (!hook_is_registered(hook))
		return -ENOENT;

	hp = &hooks[hook->hook_point];
	hook_layer_flag &= ~(1 << hook->hook_point);
	hp->pri_flag &= ~(1 << hook->priority);

	list_del(&hook->list);
	return 0;
}

