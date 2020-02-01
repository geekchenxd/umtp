/*
 * We do not support more network layer function currently.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "apdu.h"
#include "npdu.h"
#include "debug.h"
#include "umtp.h"

/*
 * decode the net work layer data of message. return zero on
 * error, or return the decoded length of net work layer data.
 */
int npdu_decode(uint8_t *npdu, struct umtp_npdu *npdu_data)
{
	if (!npdu || !npdu_data)
		return 0;

	npdu_data->protocol_version = npdu[0];

	return 1;
}

/*
 * handle the message of net work layer for umtp procotol
 * stack.
 */
void npdu_handler(struct umtp *umtp,
		struct umtp_addr *src, uint8_t *pdu, 
		uint16_t pdu_len)
{
	int decode_len = 0;
	struct umtp_npdu npdu_data = {0x0};

	if (!umtp || !pdu)
		return;

	if (pdu[0] != FTTP_PROTOCOL_VERSION)
		return;

	decode_len = npdu_decode(pdu, &npdu_data);
	debug(DEBUG, "the umtp protocol version is %d\n",
			npdu_data.protocol_version);
	if ((decode_len > 0) && (decode_len < pdu_len)) {
		pdu_len -= decode_len;
		apdu_handler(umtp, src, &pdu[decode_len], pdu_len);
	} else {
		debug(INFO, "umtp decode npdu error!\n");
	}
}

uint16_t encode_npdu(uint8_t *npdu)
{
	if (!npdu)
		return 0;

	npdu[0] = FTTP_PROTOCOL_VERSION;
	return 1;
}

int npdu_send(struct umtp *umtp, struct umtp_addr *dst,
		uint8_t *data, int len)
{
	int encode_len = 0;
	uint8_t tmp[MAX_NPDU] = {0};

	if (!data || len <= 0)
		return -EINVAL;
	
	encode_len = encode_npdu(tmp);
	if (encode_len < 0)
		return encode_len;

	memcpy(&tmp[encode_len], data, len);
	encode_len += len;

	if (umtp->dl)
		return umtp->dl->send_data(umtp->dl, dst,
				&tmp[0], encode_len);
	else
		debug(ERROR, "No date link struct inplemented\n");
	
	return -EINVAL;
}


