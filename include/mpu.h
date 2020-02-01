#ifndef __MPU_H__
#define __MPU_H__
#include <pthread.h>
#include "ringbuf.h"
#include "umtp_dl.h"

#define MAX_PDU_COUNT 16
#define MAX_MPU_DATA 2000

struct mpu_packet {
	uint16_t length;
	uint8_t buffer[MAX_MPU_DATA];
	struct umtp_addr addr;
	void *data;
};

struct mpu {
	pthread_cond_t mpu_msg_flag;
	pthread_mutex_t mpu_msg_mutex;
	ring_buffer recv_queue;
	ring_buffer send_queue;
	struct mpu_packet recv_buffer[MAX_PDU_COUNT];
	struct mpu_packet send_buffer[MAX_PDU_COUNT];
	int (*recv_handler)(struct mpu_packet *);
	int (*send_handler)(struct mpu_packet *);
	unsigned int timeout;
	pthread_t task_id;
	int running;
};

int mpu_create(struct mpu **_mpu,
	int (*recv_handler)(struct mpu_packet *),
	int (*send_handler)(struct mpu_packet *));
void mpu_destroy(struct mpu *mpu);
int mpu_start(struct mpu *mpu);
void mpu_stop(struct mpu *mpu);
int mpu_put_recv(struct mpu *mpu, struct umtp_addr *src, void *data,
		uint8_t *msg, int len);
int mpu_put_send(struct mpu *mpu, struct umtp_addr *dst, void *data,
		uint8_t *msg, int len);

#endif
