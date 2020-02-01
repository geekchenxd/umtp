#ifndef __UMTP__DL__H__
#define __UMTP__DL__H__

#include <stdint.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#define UMTP_DEFAULT_PORT 0x2E08 /*8424*/
#define MAX_MAC_LEN 7
#define UMTP_MAX_ADDR (0xFFFF)
#define UMTP_BROADCAST_ADDR (0xFFFF)


#define UMTP_SIGNATURE 0x68

struct umtp_addr {
	/* addr len is 0 indicate a broadcast address */
	uint8_t addr_len;
	/* addr is 4 bytes IP address and 2 bytes port */
	uint8_t addr[MAX_MAC_LEN];
};

enum {
	UMTP_DL_UDP = 0,
	UMTP_DL_TCP = 1,
};

struct umtp_dl {
	int type;
	char *intf;
	int socket;
	int port;
	char *ip;
	struct in_addr addr;
	struct in_addr broadcast_addr;

	int (*init)(struct umtp_dl *);
	void (*exit)(struct umtp_dl *);
	int (*send_data)(struct umtp_dl *,
			struct umtp_addr *, uint8_t *, int);
	int (*recv_data)(struct umtp_dl *,
			struct umtp_addr *, uint8_t *, int, uint32_t);
	int (*encrypt)(uint8_t *, int, uint8_t *);
	int (*decrypt)(uint8_t *, int, uint8_t *);
};

int dlumtp_bcast_addr_get(char *ifname, struct in_addr *laddr,
		struct in_addr *addr);
int dlumtp_addr_get(char *ifname, struct in_addr *addr);
int dlumtp_decode_address(struct umtp_addr *addr,
		struct in_addr *inaddr, uint16_t *port);
void dlumtp_get_umtp_addr(struct umtp_dl *dl, struct umtp_addr *addr);
void dlumtp_get_umtp_bcast_addr(struct umtp_dl *dl, struct umtp_addr *addr);
uint32_t dlumtp_get_addr(struct umtp_dl *dl);
void dlumtp_set_addr(struct umtp_dl *dl, uint32_t addr);
uint32_t dlumtp_get_bcast_addr(struct umtp_dl *dl);
void dlumtp_set_bcast_addr(struct umtp_dl *dl, uint32_t addr);

struct umtp_dl *
umtp_dludp_create(char *intf, char *ip, int port);
void umtp_dludp_destroy(struct umtp_dl *dl);

#endif

