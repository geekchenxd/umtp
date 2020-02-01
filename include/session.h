#ifndef _SESSION_H__
#define _SESSION_H__
#include <stdint.h>
#include <stdbool.h>
#include "umtp.h"

#define MAX_UMTP_SESSION 255

struct umtp_session {
	uint8_t session_id;
	bool waiting;
	uint8_t retry;
	uint32_t timer;
	void (*timeout)(struct umtp_addr *, uint8_t *, int);
	struct umtp_addr dest;
	uint8_t pdu[MAX_PDU];
	uint16_t pdu_len;
	void *data;
};

uint8_t umtp_free_session(void);
void umtp_session_set(uint8_t id,
		void (*timeout)(struct umtp_addr *, uint8_t *, int), void *data,
		struct umtp_addr *dest, uint8_t *pdu, uint16_t pdu_len);
bool umtp_session_get(uint8_t id, struct umtp_addr *dest,
		uint8_t *pdu, uint16_t *pdu_len);
void umtp_session_process(uint16_t milliseconds);
void umtp_session_clear(uint8_t id);
struct umtp_session *umtp_get_session(uint8_t id);

#endif

