#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include "service.h"
#include "session.h"

static LIST_HEAD(service_type_head);

struct service_type *service_type_get(int id)
{
	struct list_head *pos;
	struct service_type *type;

	list_for_each(pos, &service_type_head) {
		type = list_entry(pos, struct service_type, list);
		if (type->id == id)
			return type;
	}

	return NULL;
}

int service_type_register(struct service_type *type)
{
	struct service_type *tmp;

	if (!type || !type->req_handler)
		return -EINVAL;

	tmp = service_type_get(type->id);
	if (tmp)
		return -EEXIST;

	list_add(&type->list, &service_type_head);

	return 0;
}

void service_type_unregister(struct service_type *type)
{
	if (type)
		list_del(&type->list);
}

int service_handler(
		struct umtp_addr *src,
		struct service_data *_service_data,
		uint8_t *data, uint32_t data_len)
{
	struct service_type *type;

	if (!data || !data_len)
		return -EINVAL;

	type = service_type_get(_service_data->service_id);
	if (!type)
		return -ENOENT;

	if (type->req_handler)
		return type->req_handler(src, _service_data, data, data_len);
	else
		return 0;
}

void service_rsp_handler(
		struct umtp_addr *src, int service_id,
		const struct umtp_session *session,
		uint8_t *data, uint32_t data_len)
{
	struct service_type *type;

	if (!data || !data_len)
		return;

	type = service_type_get(service_id);
	if (!type)
		return;

	if (type->rsp_handler)
		type->rsp_handler(src, session, data, data_len);
}


void service_error_handler(struct umtp_addr *src, int service_id,
		int status, const struct umtp_session *session)
{
	struct service_type *type;

	type = service_type_get(service_id);
	if (!type)
		return;

	if (type->error_handler)
		type->error_handler(src, status, session);
}

void service_timeout_handler(struct umtp_addr *dst, int service_id,
		uint8_t *data, int len)
{
	struct service_type *type;

	type = service_type_get(service_id);
	if (!type)
		return;
	if (type->timeout_handler)
		type->timeout_handler(dst, service_id, data, len);
}

