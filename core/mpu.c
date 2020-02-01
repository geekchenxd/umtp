/**
 * message process unit.
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "mpu.h"
#include "debug.h"
#include "umtp_dl.h"

int mpu_create(struct mpu **_mpu,
	int (*recv_handler)(struct mpu_packet *),
	int (*send_handler)(struct mpu_packet *))
{
	struct mpu *p_mpu;

	*_mpu = (struct mpu *)malloc(sizeof **_mpu);
	if (!*_mpu) {
		debug(ERROR, "Error while allocate memory for mpu!\n");
		return -ENOMEM;
	}
	
	p_mpu = *_mpu;
	pthread_mutex_init(&p_mpu->mpu_msg_mutex, NULL);
	pthread_cond_init(&p_mpu->mpu_msg_flag, NULL);

	ringbuf_init(&p_mpu->recv_queue, (uint8_t *)&p_mpu->recv_buffer,
			sizeof(struct mpu_packet), MAX_PDU_COUNT);
	ringbuf_init(&p_mpu->send_queue, (uint8_t *)&p_mpu->send_buffer,
			sizeof(struct mpu_packet), MAX_PDU_COUNT);

	p_mpu->recv_handler = recv_handler;
	p_mpu->send_handler = send_handler;

	return 0;
}

void mpu_destroy(struct mpu *mpu)
{
	if (mpu) {
		pthread_mutex_destroy(&mpu->mpu_msg_mutex);
		pthread_cond_destroy(&mpu->mpu_msg_flag);
		free(mpu);
	}
}

static void mpu_msg_handle(ring_buffer *queue,
		int (*handler)(struct mpu_packet *))
{
	struct mpu_packet *pkt = NULL;
	int ret;

	if (ringbuf_empty(queue))
		return;

	pkt = (struct mpu_packet *)ringbuf_peek(queue);
	if (pkt && handler) {
		ret = handler(pkt);
		if (ret) {
			debug(ERROR, "msg handler error %d!\n", ret);
			return;
		}
		(void)ringbuf_pop(queue, NULL);
	}
		
}

static void get_abstime(struct timespec *abstime,
		unsigned long milliseconds)
{
	struct timeval now, offset, result;
	
	gettimeofday(&now, NULL);
	offset.tv_sec = 0;
	offset.tv_usec = milliseconds * 1000;
	timeradd(&now, &offset, &result);
	abstime->tv_sec = result.tv_sec + result.tv_usec / 1000000;
	abstime->tv_nsec = (result.tv_usec % 1000000) * 1000;
}

static void *mpu_task(void *arg)
{
	struct timespec abstime;
	struct mpu *mpu = (struct mpu *)arg;
	int ret;

	if (!mpu) {
		debug(ERROR, "Invalid argument:%s\n", __func__);
		return NULL;
	}

	while (mpu->running) {
		get_abstime(&abstime, mpu->timeout);
		pthread_mutex_lock(&mpu->mpu_msg_mutex);
		ret = pthread_cond_timedwait(&mpu->mpu_msg_flag,
				&mpu->mpu_msg_mutex, &abstime);
		if (!ret) {
			while (!ringbuf_empty(&mpu->send_queue) || 
					!ringbuf_empty(&mpu->recv_queue)) {
				mpu_msg_handle(&mpu->recv_queue, mpu->recv_handler);
				mpu_msg_handle(&mpu->send_queue, mpu->send_handler);
			}
		} else if (ret != ETIMEDOUT) {
			debug(ERROR, "pthread_cond_timedwait:%s in function:%s\n",
				strerror(ret), __func__);
			mpu->running = 0;
			return NULL;
		}
		
		pthread_mutex_unlock(&mpu->mpu_msg_mutex);
	}
	usleep(10);

	return NULL;
}

int mpu_start(struct mpu *mpu)
{
	int ret;
	
	if (!mpu)
		return -EINVAL;

	mpu->running = 1;
	ret = pthread_create(&mpu->task_id, NULL, mpu_task, mpu);
	if (ret) {
		debug(ERROR, "Error create mpu task!\n");
		mpu->running = 0;
		return ret;
	}

	return 0;
}

void mpu_stop(struct mpu *mpu)
{
	if (!mpu)
		return;

	mpu->running = false;
	pthread_join(mpu->task_id, NULL);
}

int mpu_put_recv(struct mpu *mpu, struct umtp_addr *src, void *data,
		uint8_t *msg, int len)
{
	int bytes = 0;
	struct mpu_packet *pkt = NULL;
	int i = 0;

	if (!mpu || !msg)
		return -EINVAL;
	if (!mpu->running) {
		debug(ERROR, "mpu not running while calling!\n");
		return 0;
	}
	
	pthread_mutex_lock(&mpu->mpu_msg_mutex);
	pkt = (struct mpu_packet *)ringbuf_data_peek(&mpu->recv_queue);
	if (pkt) {
		for (i = 0; i < len; i++)
			pkt->buffer[i] = msg[i];
		pkt->length = len;
		pkt->data = data;
		memcpy(&pkt->addr, src, sizeof(struct umtp_addr));
		if (ringbuf_data_put(&mpu->recv_queue, (uint8_t *)pkt))
			bytes = len;
	}
	pthread_cond_signal(&mpu->mpu_msg_flag);
	pthread_mutex_unlock(&mpu->mpu_msg_mutex);
	
	return bytes;
}

int mpu_put_send(struct mpu *mpu, struct umtp_addr *dst,
		void *data, uint8_t *msg, int len)
{
	int bytes = 0;
	struct mpu_packet *pkt = NULL;
	int i = 0;

	if (!mpu || !msg)
		return -EINVAL;

	if (!mpu->running) {
		debug(ERROR, "mpu not running while calling!\n");
		return 0;
	}
	
	pthread_mutex_lock(&mpu->mpu_msg_mutex);
	pkt = (struct mpu_packet *)ringbuf_data_peek(&mpu->send_queue);
	if (pkt) {
		for (i = 0; i < len; i++)
			pkt->buffer[i] = msg[i];
		pkt->length = len;
		pkt->data = data;
		memcpy(&pkt->addr, dst, sizeof(struct umtp_addr));
		if (ringbuf_data_put(&mpu->send_queue, (uint8_t *)pkt))
			bytes = len;
	}
	pthread_cond_signal(&mpu->mpu_msg_flag);
	pthread_mutex_unlock(&mpu->mpu_msg_mutex);
	
	return bytes;
}

