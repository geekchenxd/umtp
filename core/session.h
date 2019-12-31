#ifndef _SESSION_H__
#define _SESSION_H__
#include <stdint.h>
#include <stdbool.h>

#define MAX_UMTP_SESSION 255

struct umtp_session {
	uint8_t session_id;
	bool waiting;
	uint8_t retry;
	uint32_t timer;
	struct umtp_addr dest;
	uint8_t pdu[MAX_PDU];
	uint16_t pdu_len;
};

uint8_t umtp_free_session(void);
void umtp_session_set(uint8_t id, struct umtp_addr *dest,
		uint8_t *pdu, uint16_t pdu_len);
bool umtp_session_get(uint8_t id, struct umtp_addr *dest,
		uint8_t *pdu, uint16_t *pdu_len);
void umtp_session_process(uint16_t milliseconds);
void umtp_session_clear(uint8_t id);
void umtp_set_session_handle(void (*fun)(uint8_t));

#endif
