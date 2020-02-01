#ifndef __NPDU__H__
#define __NPDU__H__
#include <stdint.h>
#include "umtp.h"
#include "umtp_dl.h"

struct umtp_npdu {
	uint8_t protocol_version;
};

int npdu_decode(uint8_t *npdu, struct umtp_npdu *npdu_data);
uint16_t encode_npdu(uint8_t *npdu);
int npdu_handler(struct umtp *umtp,
		struct umtp_addr *src, uint8_t *pdu, 
		uint16_t pdu_len);
int npdu_send(struct umtp *umtp, struct umtp_addr *dst,
		uint8_t *data, int len);

#endif
