#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <net/if_arp.h>
#include "umtp_dl.h"


void dlumtp_set_bcast_addr(struct umtp_dl *dl, uint32_t addr)
{
	if (dl)
		dl->broadcast_addr.s_addr = addr;
}

uint32_t dlumtp_get_bcast_addr(struct umtp_dl *dl)
{
	if (dl)
		return dl->broadcast_addr.s_addr;
	return 0;
}

void dlumtp_set_addr(struct umtp_dl *dl, uint32_t addr)
{
	if (dl)
		dl->addr.s_addr = addr;
}

uint32_t dlumtp_get_addr(struct umtp_dl *dl)
{
	if (dl)
		return dl->addr.s_addr;
	return 0;
}

void dlumtp_get_umtp_bcast_addr(struct umtp_dl *dl, struct umtp_addr *addr)
{
	if (!addr || !dl)
		return;

	adr->addr_len = 6;
	memcpy(&addr->addr[0], &dl->broadcast_addr.s_addr, 4);
	memcpy(&addr->addr[4], &dl->port, 2);
}

void dlumtp_get_umtp_addr(struct umtp_dl *dl, struct umtp_addr *addr)
{
	if (!addr || !dl)
		return;

	adr->addr_len = 6;
	memcpy(&addr->addr[0], &dl->addr.s_addr, 4);
	memcpy(&addr->addr[4], &dl->port, 2);
}

int dlumtp_decode_address(struct umtp_addr *addr,
		struct in_addr *inaddr, uint16_t *port)
{
	if (!addr)
		return 0;

	memcpy(&inaddr->s_addr, &addr->addr[0], 4);
	memcpy(port, &addr->addr[4], 2);

	return 6;
}

static int get_local_ifr_ioctl(char *ifname, 
		struct ifreq *ifr, int opt)
{
	int fd;
	int ret;
	strncpy(ifr->ifr_name, ifname, sizeof(ifr->ifr_name));
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (fd < 0) {
		ret = fd;
	} else {
		ret = ioctl(fd, opt, ifr);
		close(fd);
	}

	return ret;
}

static int get_local_address_ioctl(char *ifname, 
		struct in_addr *addr, int opt)
{
	struct ifreq ifr = { {{0}} };
	struct sockaddr_in *tcpip_addr;
	int ret;

	ret = get_local_ifr_ioctl(ifname, &ifr, opt);
	if (ret >= 0) {
		tcpip_addr = (struct sockaddr_in *)&ifr.ifr_addr;
		memcpy(addr, &tcpip_addr->sin_addr, sizeof(struct in_addr));
	}

	return ret;
}

int dlumtp_addr_get(char *ifname, struct in_addr *addr)
{
	struct in_addr local_addr;
	int ret = 0;

	if (!ifname || !addr)
		return -EINVAL;

	ret = get_local_address_ioctl(ifname, &local_addr, SIOCGIFADDR);
	if (ret)
		local_addr.s_addr = 0;

	return ret;
}

int dlumtp_bcast_addr_get(char *ifname, struct in_addr *laddr,
		struct in_addr *addr)
{
	struct in_addr netmask;
	int ret = 0;

	if (!ifname || !laddr || !addr)
		return -EINVAL;

	ret = get_local_address_ioctl(ifname, &netmask, SIOCGIFNETMASK);
	if (ret) {
		addr.s_addr = ~0;
	} else {
		memcpy(add, laddr, sizeof(struct in_addr));
		addr.s_addr |= (~netmask.s_addr);
	}

	return ret;
}


