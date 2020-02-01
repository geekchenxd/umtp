#ifndef __UMTP_APDU_H__
#define __UMTP_APDU_H__
#include <stdint.h>
#include <stdbool.h>
#include "umtp.h"

#define UMTP_SESSION_ID_MASK (1 << 0)

enum {
	UMTP_PDU_REQUEST = 0,
	UMTP_PDU_RESPONSE,
	UMTP_PDU_ERROR,
	UMTP_PDU_REJECT,
	UMTP_PDU_TYPE_MAX,
};

struct apdu_data {
	int service;
	bool confirmed;
	uint8_t data[MAX_APDU];
	int data_len;
};

uint8_t apdu_retry(void);
void apdu_retry_set(uint8_t count);
uint32_t apdu_timeout(void);
void apdu_timeout_set(uint32_t timeout);
int apdu_handler(struct umtp *umtp, struct umtp_addr *src, 
		uint8_t *apdu, uint16_t pdu_len);
int umtp_apdu_submit(struct umtp *umtp, struct umtp_addr *dst,
		struct apdu_data *apdu_data);

#endif
