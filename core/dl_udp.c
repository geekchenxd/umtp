#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <netdb.h>
#include <sys/un.h>
#include <net/route.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include "umtp_dl.h"
#include "crypt.h"
#include "umtp.h"
#include "debug.h"

#define PASSWDDEFAULT  "Hello Umtp"

static int dludp_recv_data(struct umtp_dl *dl,
		struct umtp_addr *src, uint8_t *lpdu, int max_pdu,
		uint32_t timeout)
{  
	int recv_bytes = 0;
	int pdu_len = 0;
	fd_set fds;
	int max = 0;
	struct timeval st;
	struct sockaddr_in sin = {0x0};
	socklen_t sin_len = sizeof(sin);
	uint8_t pdu[MAX_PDU] = {0x0};

	if (!dl || dl->socket < 0 || !src || !lpdu)
		return -EINVAL;

	if (timeout >= 1000) {
		st.tv_sec = timeout / 1000;
		st.tv_usec =
			1000 * (timeout - st.tv_sec * 1000);
	} else {
		st.tv_sec = 0;
		st.tv_usec = 1000 * timeout;
	}

	FD_ZERO(&fds);
	FD_SET(dl->socket, &fds);
	max = dl->socket;

	if (select(max + 1, &fds, NULL, NULL, &st) > 0) {
		recv_bytes =
			recvfrom(dl->socket, (char *)&pdu[0], max_pdu, 0,
					(struct sockaddr *)&sin, &sin_len);
	} else {
		return -ETIMEDOUT;
	}
	//debug(INFO, "src is %s:%d\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

	if (recv_bytes <= 0)
		return 0;
	if (recv_bytes > max_pdu)
		return -ENOMEM;

	debug(INFO, "Datalink received data is:");
	dump_hex(stdout, pdu, recv_bytes);
	/* message from myself, drop it */
	if ((sin.sin_addr.s_addr == dl->addr.s_addr) &&
			(sin.sin_port == dl->port))
		return 0;

	if (pdu[0] != (uint8_t)UMTP_SIGNATURE)
		return 0;

	src->addr_len = 6;
	memcpy(&src->addr[0], &sin.sin_addr.s_addr, 4);
	memcpy(&src->addr[4], &sin.sin_port, 2);

	if (dl->decrypt) {
		pdu_len = dl->decrypt(&pdu[1], recv_bytes - 1, lpdu);
		if (pdu_len <= 0) {
			debug(ERROR, "dludp decrypt failed!\n");
			pdu_len = 0;
		}
	} else {
		memcpy(lpdu, &pdu[1], recv_bytes - 1);
		pdu_len = recv_bytes - 1;
	}

	return pdu_len;
}

static int dludp_send_data(struct umtp_dl *dl,
		struct umtp_addr *dst, uint8_t *pdu, int pdu_len)
{
	struct sockaddr_in dest;
	int bytes_sent = 0;
	int len = 0;
	uint8_t buf[MAX_PDU] = {0x0};
	struct in_addr addr;
	uint16_t port;

	if (dl->socket < 0)
		return -EBADF;
	if (dst->addr_len == 0) {
		/* broadcast addr */
		port = dl->port;
		memcpy(&addr, &dl->broadcast_addr, sizeof(struct in_addr));
	} else {
		dlumtp_decode_address(dst, &addr, &port);
	}

	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = addr.s_addr;
	dest.sin_port = port;
	memset(&(dest.sin_zero), '\0', 8);

	if (dl->encrypt) {
		len = dl->encrypt(pdu, pdu_len, &buf[1]);
		if (len <= 0) {
			debug(WARNING, "UDP encrypt failed!\n");
			return 0;
		}
	} else {
		memcpy(&buf[1], pdu, pdu_len);
		len = pdu_len;
	}
	buf[0] = UMTP_SIGNATURE;
	len += 1;

//	debug(INFO, "dest is %s:%d\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
	debug(INFO, "Data link sending data is:");
	dump_hex(stdout, buf, len);
	bytes_sent = sendto(dl->socket, (char *)buf, len, 0,
			(struct sockaddr *)&dest, sizeof(struct sockaddr));
	if (bytes_sent != len) {
		debug(ERROR, "udp send data failed!-%s\n", strerror(errno));
		return -errno;
	}

	return 0;
}

static int dludp_init(struct umtp_dl *dludp)
{
	int ret;
	int sockopt = 0;
	struct sockaddr_in sin;

	if (!dludp)
		return -EINVAL;

	if (dludp->port <= 0)
		dludp->port = UMTP_DEFAULT_PORT;

	if (dludp->ip)
		inet_aton(dludp->ip, &dludp->addr);
	else
		dlumtp_addr_get(dludp->intf ? dludp->intf : "eth0", &dludp->addr);

	dlumtp_bcast_addr_get(dludp->intf ? dludp->intf : "eth0",
			&dludp->addr, &dludp->broadcast_addr);

	dludp->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (dludp->socket <= 0) {
		debug(ERROR, "Error create socket for dludp:");
		perror("");
		return errno;
	}

	sockopt = 1;
	ret = setsockopt(dludp->socket, SOL_SOCKET,
			SO_REUSEADDR, &sockopt, sizeof(sockopt));
	if (ret) {
		debug(ERROR, "setsockopt error:%s\n", strerror(errno));
		ret = errno;
		goto close_socket;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(dludp->port);
	memset(&(sin.sin_zero), '\0', sizeof(sin.sin_zero));
	ret = bind(dludp->socket, (const struct sockaddr *)&sin, 
			sizeof(struct sockaddr));
	if (ret) {
		debug(ERROR, "Error bind:%s\n", strerror(errno));
		ret = errno;
		goto close_socket;
	}
	debug(INFO, "udp bind at %s:%d\n", dludp->ip, dludp->port);

	return 0;

close_socket:
	close(dludp->socket);
	dludp->socket = -1;

	return ret;
}

int dludp_encrypt(uint8_t *input, int input_len, uint8_t *output)
{
	/* A 256 bit key */
	static unsigned char key[KEY_SIZE] = {0x0};

	/* A 128 bit IV */
	static unsigned char iv[IV_SIZE] = {0x0};

	/* Buffer for the tag */
	unsigned char tag[16];

	int len = 0;

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();     

	if (key[0] == 0x0)
		gen_key(PASSWDDEFAULT, strlen(PASSWDDEFAULT), key);
	RAND_bytes(iv, IV_SIZE);
	memcpy(output, iv, IV_SIZE);

	/* Encrypt the plaintext */
	len = encrypt(input, input_len, key, iv, 
			(unsigned char *)&output[IV_SIZE + sizeof(tag)], tag);

	if (len < 0) {
		ERR_free_strings();
		printf("Encrypted text failed to verify\n");
		return len;
	}
#if 0
	printf("Ciphertext is:\n");
	BIO_dump_fp(stdout, (const char *)&output[32], len);
	printf("Tag is:\n");
	BIO_dump_fp(stdout, (const char *)tag, 16);
#endif

	memcpy(&output[IV_SIZE], tag, sizeof(tag));
	/* Remove error strings */
	ERR_free_strings();

	if (len > 0)
		return len + IV_SIZE + sizeof(tag);
	else 
		return len;
}

int dludp_decrypt(uint8_t *input, int input_len, uint8_t *output)
{
	/* A 256 bit key */
	static unsigned char key[KEY_SIZE] = {0x0};

	/* A 128 bit IV */
	static unsigned char iv[IV_SIZE] = {0x0};

	/* Buffer for the tag */
	unsigned char tag[16];

	int len = 0;

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();     

	if (key[0] == 0x0)
		gen_key(PASSWDDEFAULT, strlen(PASSWDDEFAULT), key);


	/*decode the iv*/
	memcpy(&iv[0], input, IV_SIZE);
	input_len -= IV_SIZE;

	/*decode the tag */
	memcpy(&tag[0], &input[IV_SIZE], sizeof(tag));
	input_len -= sizeof(tag);

	/* Decrypt the ciphertext */
	len = decrypt(&input[IV_SIZE + sizeof(tag)], input_len, tag, key, iv, output);
	if(len < 0)
	{
		/* Verify error */
		printf("Decrypted text failed to verify\n");
	}

	/* Remove error strings */
	ERR_free_strings();

	return len;
}

void dludp_exit(struct umtp_dl *dl)
{
	if (dl)
		if (dl->socket) {
			close(dl->socket);
			dl->socket = -1;
		}
}

void umtp_dludp_destroy(struct umtp_dl *dl)
{
	if (dl) {
		dludp_exit(dl);
		free(dl);
	}
}

struct umtp_dl *
umtp_dludp_create(char *intf, char *ip, int port)
{
	struct umtp_dl *dl;

	dl = (struct umtp_dl *)malloc(sizeof *dl);
	if (!dl) {
		debug(ERROR, "Error while allco mem for dl:%s\n",
				strerror(errno));
		return NULL;
	}
	memset(dl, 0x0, sizeof *dl);

	dl->type = UMTP_DL_UDP;
	dl->ip = ip;
	dl->socket = -1;
	dl->port = port;
	dl->intf = intf;

	dl->init = dludp_init;
	dl->exit = dludp_exit;
	dl->send_data = dludp_send_data;
	dl->recv_data = dludp_recv_data;
//	dl->encrypt = dludp_encrypt;
//	dl->decrypt = dludp_decrypt;

	return dl;
}


