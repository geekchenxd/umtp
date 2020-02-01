#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "umtp.h"
#include "service.h"
#include "umtp_dl.h"
#include "apdu.h"

static int wait = 1;

static void simple_client_rsp_handler(struct umtp_addr *src,
	const struct umtp_session *session, uint8_t *pdu, int len)
{
	printf("client recv:%s\n", (char *)pdu);
	wait = 0;
}

static struct service_type simple_client = {
	.id = 1,
	.name = "simple client",
	.rsp_handler = simple_client_rsp_handler,
};

static struct umtp_conf conf = {
	.sync_mode = true,
};

int main(int argc, char *argv[])
{
	struct umtp *umtp;
	struct umtp_dl *dl;
	struct umtp_addr dst;
	int len = 0;
	struct apdu_data send_data = {
		.service = 1,
		.confirmed = 1,
	};
	memcpy(&send_data.data, "hello world", 12);
	send_data.data_len = 12;

	dl = umtp_dludp_create("eth0", "0.0.0.0", 10001);
	if (!dl) {
		printf("umtp_dludp_create failed!\n");
		return -1;
	}

	umtp = umtp_alloc(&conf, dl);
	if (!umtp) {
		printf("umtp_alloc failed!\n");
		return -1;
	}

	if (service_type_register(&simple_client)) {
		printf("register service failed!\n");
		return -1;
	}

	if (umtp_start(umtp)) {
		printf("umtp_start failed!\n");
		return -1;
	}

	len = umtp_send_message(umtp, &dst, &send_data);
	if (len <= 0) {
		printf("send message failed!\n");
		return -1;
	}

	while (wait) {
		sleep(1);
	}

	return 0;
}

