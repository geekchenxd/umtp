#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include "umtp.h"
#include "debug.h"
#include "mpu.h"
#include "apdu.h"
#include "npdu.h"
#include "session.h"


static struct umtp_conf umtp_default_config = {
	.sync_mode = false,
};

extern int _umtp_apdu_submit(struct umtp *umtp,
		struct umtp_addr *dst,
		uint8_t *apdu, int pdu_len);

static int _umtp_send_msg(struct mpu_packet *pkt)
{
	return npdu_send((struct umtp *)pkt->data, &pkt->addr,
			pkt->buffer, pkt->length);
}

static int _umtp_handle_msg(struct mpu_packet *pkt)
{
	return npdu_handler((struct umtp *)pkt->data, &pkt->addr,
			pkt->buffer, pkt->length);
}

static void *umtp_task(void *arg)
{
	struct umtp *umtp = (struct umtp *)arg;
	struct umtp_conf *conf = umtp->conf;
	struct umtp_dl *dl = umtp->dl;
	int len = 0;
	struct umtp_addr src;
	uint8_t buf[MAX_PDU];
	int ret;
	time_t last_seconds = 0;
	time_t current_seconds = 0;
	uint32_t elapsed_seconds = 0;
	uint32_t elapsed_milliseconds = 0;

	last_seconds = time(NULL);

	while (umtp->running) {
		current_seconds = time(NULL);
		len = dl->recv_data(dl, &src, buf, MAX_PDU, 1000);
		if (len < 0 && len != -ETIMEDOUT) {
			debug(ERROR, "Umtp recv error, ret %d\n", len);
			break;
		}

		if (len > 0) {
			if (!conf->sync_mode)
				ret = mpu_put_recv(umtp->mpu, &src, (void *)umtp, buf, len);
			else
				ret = npdu_handler(umtp, &src, buf, len);
			if (ret) {
				debug(ERROR, "handle message error,ret %d\n", ret);
			}
		}
		
		elapsed_seconds = (uint32_t)(current_seconds - last_seconds);
		if (elapsed_seconds) {
			elapsed_milliseconds = elapsed_seconds * 1000;
			last_seconds = current_seconds;
			umtp_session_process(elapsed_milliseconds);
		}
	}

	umtp->running = false;
	return NULL;
}

int umtp_start(struct umtp *umtp)
{
	int ret = 0;

	if (!umtp)
		return -EINVAL;

	if (!umtp->dl) {
		debug(ERROR, "umtp data link layer not impletated!\n");
		return -EINVAL;
	}

	if (!umtp->conf) {
		debug(ERROR, "umtp not configured!\n");
		return -EINVAL;
	}

	if (!umtp->conf->sync_mode) {
		ret = mpu_create(&umtp->mpu, _umtp_handle_msg,
				_umtp_send_msg);
		if (ret) {
			debug(ERROR, "Error create mpu for umtp,ret %d\n", ret);
			return ret;
		}
		ret = mpu_start(umtp->mpu);
		if (ret) {
			debug(ERROR, "Error start mpu,ret %d\n", ret);
			goto free_mpu;
		}
	}

	if (umtp->dl->init) {
		ret = umtp->dl->init(umtp->dl);
		if (ret) {
			debug(ERROR, "Error init umtp data link!\n");
			return ret;
		}
	}

	umtp->running = true;
	ret = pthread_create(&umtp->task_id, NULL, umtp_task, umtp);
	if (ret) {
		debug(ERROR, "Failed to create umtp task, ret %d\n", ret);
		umtp->running = false;
		return ret;
	}

	return 0;
free_mpu:
	mpu_destroy(umtp->mpu);
	return ret;
}

void umtp_stop(struct umtp *umtp)
{
	if (umtp) {
		if (umtp->dl->exit)
			umtp->dl->exit(umtp->dl);
		if (umtp->mpu) {
			if (umtp->mpu->running)
				mpu_stop(umtp->mpu);
			mpu_destroy(umtp->mpu);
		}
		umtp->running = true;
		pthread_join(umtp->task_id, NULL);
	}
}

struct umtp *umtp_alloc(struct umtp_conf *conf, struct umtp_dl *dl)
{
	struct umtp *up;

	if (!dl)
		return NULL;
		
	up = (struct umtp *)malloc(sizeof *up);
	if (!up) {
		debug(ERROR, "Error while allocate umtp\n");
		return NULL;
	}
	memset(up, 0x0, sizeof *up);
	
	up->conf = conf;
	if (!up->conf)
		up->conf = &umtp_default_config;
	up->dl = dl;

	return up;
}

void umtp_free(struct umtp *umtp)
{
	if (umtp) {
		if (umtp->running)
			umtp_stop(umtp);
		free(umtp);
	}
}

int umtp_send_message(struct umtp *umtp, struct umtp_addr *dst,
		struct apdu_data *apdu_data)
{
	return umtp_apdu_submit(umtp, dst, apdu_data);
}

