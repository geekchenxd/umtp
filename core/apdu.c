#include <errno.h>
#include "apdu.h"
#include "session.h"
#include "error.h"

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

bool apdu_flag_has_session_id(uint8_t apdu_flag)
{
	return apdu_type & UMTP_SESSION_ID_MASK;
}

void apdu_flag_set_session_id(uint8_t apdu_flag)
{
	apdu_type |= UMTP_SESSION_ID_MASK;
}

static int _umtp_apdu_submit(struct umtp_addr *dst,
		uint8_t apdu, uint16_t pdu_len)
{
}

/*
 * according to diffrent pdu type and service call the 
 * service handler function.
 */
int apdu_handler(struct umtp_addr *src, 
		uint8_t *apdu, uint16_t pdu_len)
{
	uint8_t session_id;
	uint8_t apdu_flag;
	uint8_t apdu_type;
	int service;
	uint16_t decode_len = 0;
	uint16_t encode_len = 0;
	int ret = UMTP_ERROR_UNKNOW_PDU_TYPE;
	struct service_data sd;
	int service_data_offset;
	struct umtp_session *session;

	if (!apdu)
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
			encode_len += sd.data_len;
		ret = _umtp_apdu_submit(src, apdu_trans_blk, encode_len);
		if (ret) {
			debug(ERROR,
					"Error while submit ack for service request!, service:%d,return:%d\n",
					service, ret);
			return ret;
		}
		break;
	case UMTP_PDU_RESPONSE:
		if (apdu_has_session_id(apdu_flag))
			session = umtp_session_get(session_id);
		service_rsp_handler(src, service, session, &apdu[decode_len],
				pdu_len - decode_len);

		if (apdu_has_session_id(apdu_flag))
			umtp_session_clear(session_id);

		return 0;
	case UMTP_PDU_ERROR:
		if (apdu_has_session_id(apdu_flag))
			session = umtp_session_get(session_id);
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
}

static int encode_apdu_common(uint8_t *apdu,
		uint8_t apdu_flag, int pdu_type, int service)
{
	int encode_len = 0;
	if (!apdu)
		return 0;

	apdu[encode_len++] = apdu_flag;
	apdu[encode_len++] = (uint8_t)pdu_type;
	apdu[encode_len++] = (uint8_t)service;
	/* the session id area will be seted later */
	if (apdu_has_session_id(apdu_flag))
		encode_len++;

	return encode_len;
}

int umtp_apdu_submit(struct umtp_addr *dst,
		struct apdu_data *apdu_data)
{
	int len, i;
	uint8_t tmp[MAX_APDU - 4] = {0x0};
	uint8_t apdu_flag = 0;
	if (!apdu_data || sizeof(apdu_data->data) > MAX_APDU)
		return -EINVAL;

	if (apdu_data->confirmed)
		apdu_flag_set_session_id(apdu_flag);

	memcpy(tmp, apdu_data->data, apdu_data->data_len);

	len = encode_apdu_common(apdu_data->data, apdu_flag,
			apdu_data->pdu_type, apdu_data->service);
	if (len <= 0) {
		debug(ERROR, "Encode apdu common failed!return %d\n", ret);
		return ret;
	}
	memcpy(&apdu_data->data[len], tmp, apdu_data->data_len);

	return _umtp_apdu_submit(dst, apdu_data->data, apdu_data->data_len + ret);
}


