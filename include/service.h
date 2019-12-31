#include <stdint.h>

struct service_type {
	struct list_head list;
	uint8_t id;
	char *name;
	int (*handler)();
};
