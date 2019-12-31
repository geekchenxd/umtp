#ifndef __UMTP__H__
#define __UMTP__H__

struct umtp {
	struct umtp_conf *conf;
	struct umtp_ll *ll;
};

struct umtp *umtp_create(struct umtp_conf *conf);
int umtp_start(struct umtp *umtp);
int umtp_stop(struct umtp *umtp);
void umtp_destroy(struct umtp *umtp);
int umtp_register_hook();
int umtp_unregister_hook();
int umtp_register_service();
void umtp_unreigster_service();
int umtp_send_message();


#endif
