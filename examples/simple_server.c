#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "umtp.h"
#include "service.h"
#include "umtp_dl.h"

static int simple_server_req_handler(struct umtp_addr *src,
	struct service_data *sd, uint8_t *pdu, int len)
{
	printf("server recv:%s\n", (char *)pdu);
	memcpy(sd->data, "Nihao China", 12);
	sd->data_len = 12;
	return 0;
}

static struct service_type simple_server = {
	.id = 1,
	.name = "simple server",
	.req_handler = simple_server_req_handler,
};

int main(void)
{
	struct umtp *umtp;
	struct umtp_dl *dl;
	struct umtp_conf conf = {
		.sync_mode = false,
	};

	dl = umtp_dludp_create("eth0", "0.0.0.0", -1);
	if (!dl) {
		printf("umtp_dludp_create failed!\n");
		return -1;
	}

	umtp = umtp_alloc(&conf, dl);
	if (!umtp) {
		printf("umtp_alloc failed!\n");
		return -1;
	}

	if (service_type_register(&simple_server)) {
		printf("register service failed!\n");
		return -1;
	}

	if (umtp_start(umtp)) {
		printf("umtp_start failed!\n");
		return -1;
	}

	while (1) {
		sleep(1);
	}

	return 0;
}

