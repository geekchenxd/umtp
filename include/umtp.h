#ifndef __UMTP__H__
#define __UMTP__H__
#include <stdbool.h>
#include <pthread.h>
#include "umtp_dl.h"
#include "mpu.h"

#define MAX_APDU 1024
#define MAX_NPDU MAX_APDU + 1
#define MAX_PDU MAX_APDU+MAX_NPDU

#define FTTP_PROTOCOL_VERSION 0x01

struct umtp_conf {
	bool sync_mode;
};

struct umtp {
	struct umtp_conf *conf;
	struct umtp_dl *dl;
	struct mpu *mpu;
	pthread_t task_id;
	bool running;
};

struct umtp *umtp_alloc(struct umtp_conf *conf,
		struct umtp_dl *dl);
int umtp_start(struct umtp *umtp);
void umtp_stop(struct umtp *umtp);
void umtp_free(struct umtp *umtp);
int umtp_send_message();

#endif

