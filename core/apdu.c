#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "apdu.h"
#include "session.h"
#include "error.h"
#include "umtp.h"
#include "debug.h"
#include "service.h"
#include "npdu.h"
#include "mpu.h"

static uint8_t apdu_trans_blk[MAX_APDU];
/*
 * apdu timeout indicate the longest time that a session wait for reply.
 */
static uint32_t umtp_apdu_timeout = 3000;
/*
 * apdu_retry indicate once the session waiting reply timeout, how many
 * times the request of this session will be retried.
 */
static uint8_t umtp_apdu_retry = 1;


uint8_t apdu_retry(void)
{
	return umtp_apdu_retry;
}

void apdu_retry_set(uint8_t count)
{
	umtp_apdu_retry = count;
}

uint32_t apdu_timeout(void)
{
	return umtp_apdu_timeout;
}

void apdu_timeout_set(uint32_t timeout)
{
	umtp_apdu_timeout = timeout;
}

static bool apdu_has_session_id(uint8_t apdu_flag)
{
	return apdu_flag & UMTP_SESSION_ID_MASK;
}

static void apdu_flag_set_session_id(uint8_t apdu_flag)
{
	apdu_flag |= UMTP_SESSION_ID_MASK;
}

static void apdu_timeout_handler(struct umtp_addr *dst,
		uint8_t *apdu, int apdu_len)
{
	int service = apdu[2];
	service_timeout_handler(dst, service, &apdu[4], apdu_len - 4);
}

int _umtp_apdu_submit(struct umtp *umtp,
		struct umtp_addr *dst,
		uint8_t *apdu, int pdu_len)
{
	if (!umtp->conf->sync_mode)
		return mpu_put_send(umtp->mpu, (void *)umtp,
			dst, apdu, pdu_len);
	return npdu_send(umtp, dst, apdu, pdu_len);
}

/*
 * according to diffrent pdu type and service call the 
 * service handler function.
 */
int apdu_handler(struct umtp *umtp, struct umtp_addr *src, 
		uint8_t *apdu, uint16_t pdu_len)
{
	uint8_t session_id = 0;
	uint8_t apdu_flag;
	uint8_t apdu_type;
	int service;
	uint16_t decode_len = 0;
	uint16_t encode_len = 0;
	int ret = UMTP_ERROR_UNKNOW_PDU_TYPE;
	struct service_data sd;
	int service_data_offset;
	struct umtp_session *session = NULL;

	if (!apdu || !umtp)
		return -EINVAL;

	apdu_flag = apdu[decode_len++];
	apdu_type = apdu[decode_len++];
	if (apdu_type >= UMTP_PDU_TYPE_MAX)
		return -EPROTO;

	service = apdu[decode_len++];
	if (apdu_has_session_id(apdu_flag))
		session_id = apdu[decode_len++];

	switch (apdu_type) {	/* pdu type */
	case UMTP_PDU_REQUEST:
		service_data_offset = decode_len;
		sd.data = &apdu_trans_blk[service_data_offset];
		sd.service_id = service;
		sd.data_len = 0;
		ret = service_handler(src, &sd, &apdu[decode_len],
				pdu_len - decode_len);

		/* encode the rsp message */
		apdu_trans_blk[encode_len++] = apdu_flag;
		if (ret)
			apdu_type = UMTP_PDU_ERROR;
		else
			apdu_type = UMTP_PDU_RESPONSE;
		apdu_trans_blk[encode_len++] = apdu_type;
		apdu_trans_blk[encode_len++] = service;
		if (apdu_has_session_id(apdu_flag))
			apdu_trans_blk[encode_len++] = session_id;

		if (apdu_type == UMTP_PDU_ERROR)
			apdu_trans_blk[encode_len++] = (uint8_t)ret;
		else 
		{
			memcpy(&apdu_trans_blk[encode_len], sd.data, sd.data_len);
			encode_len += sd.data_len;
		}

		ret = _umtp_apdu_submit(umtp, src, &apdu_trans_blk[0], encode_len);
		if (ret) {
			debug(ERROR,
					"Error while submit ack for service request!, service:%d,return:%d\n",
					service, ret);
			return ret;
		}
		break;
	case UMTP_PDU_RESPONSE:
		if (apdu_has_session_id(apdu_flag))
			session = umtp_get_session(session_id);
		service_rsp_handler(src, service, session, &apdu[decode_len],
				pdu_len - decode_len);

		if (apdu_has_session_id(apdu_flag))
			umtp_session_clear(session_id);

		return 0;
	case UMTP_PDU_ERROR:
		if (apdu_has_session_id(apdu_flag))
			session = umtp_get_session(session_id);
		service_error_handler(src, service, (int)apdu[decode_len], session);
		if (apdu_has_session_id(apdu_flag))
			umtp_session_clear(session_id);

		return 0;
		break;
	case UMTP_PDU_REJECT:
		if (apdu_has_session_id(apdu_flag))
			umtp_session_clear(session_id);
		break;
	default:
		break;
	}

	return 0;
}

int umtp_apdu_submit(struct umtp *umtp, struct umtp_addr *dst,
		struct apdu_data *apdu_data)
{
	uint8_t tmp[MAX_APDU] = {0x0};
	uint8_t apdu_flag = 0;
	uint8_t session_id = 0;
	int encode_len = 0;

	if (!umtp)
		return -EINVAL;
	if (!apdu_data || sizeof(apdu_data->data) > MAX_APDU)
		return -EINVAL;

	/* flag set. */
	if (apdu_data->confirmed)
		apdu_flag_set_session_id(apdu_flag);

	tmp[encode_len++] = apdu_flag;
	tmp[encode_len++] = UMTP_PDU_REQUEST;
	tmp[encode_len++] = apdu_data->service;

	/* the session id area will be seted later */
	if (apdu_has_session_id(apdu_flag)) {
		while (session_id == 0) {
			session_id = umtp_free_session();
			if (session_id == 0)
				usleep(100);
		}
		tmp[encode_len++] = session_id;
	}

	memcpy(&tmp[encode_len], apdu_data->data,
			apdu_data->data_len);

	encode_len += apdu_data->data_len;

	if (apdu_data->confirmed)
		umtp_session_set(session_id, apdu_timeout_handler, umtp,
				dst, tmp, encode_len);

	return _umtp_apdu_submit(umtp, dst, tmp, encode_len);
}


