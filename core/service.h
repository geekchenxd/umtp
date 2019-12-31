#ifndef __SERVICE__H__
#define __SERVICE__H__

struct service_data {
	int service_id;
	uint32_t data_len;
	uint8_t *data;
};

struct service_type {
	struct list_head list;
	int id;
	char *name;
	int (*req_handler)(
			struct umtp_addr *, struct service_data *, 
			uint8_t *, uint32_t *);
	void (*rsp_handler)(
			struct umtp_addr *, const umtp_session *,
			uint8_t *, uint32_t *);
	void (*error_handler)(
			struct umtp_addr *, int, int,
			const struct umtp_session *session);
};

int service_type_register(struct service_type *type);
void service_type_unregister(struct service_type *type);
int service_handler(
		struct umtp_addr *src,
		struct service_data *_service_data,
		uint8_t *data, uint32_t data_len);
void service_rsp_handler(
		struct umtp_addr *src, int service_id,
		const struct umtp_session *session,
		uint8_t *data, uint32_t data_len);
void service_error_handler(
		struct umtp_addr *src, int service_id, int status,
		const struct umtp_session *session);

#endif
