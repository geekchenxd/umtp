#ifndef __UMTP_APDU_H__
#define __UMTP_APDU_H__

struct apdu_data {
	int pdu_type;
	int service;
	bool confirmed;
	uint8_t data[MAX_APDU];
	int data_len;
};

#endif
