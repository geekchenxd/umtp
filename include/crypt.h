#ifndef __CRYPT_H_
#define __CRYPT_H_

#define KEY_SIZE 32  /*AES 256 GCM mode.*/
#define KEY_SALT "umtp_v0.1"
#define IV_SIZE	16
#define ITERATIONS 1000					

void gen_key(char *passwd, int passwd_len,
		unsigned char *key);
int encrypt(unsigned char *in, int in_len,
		const unsigned char *key, const unsigned char *iv,
		unsigned char *out, unsigned char *tag);
int decrypt(unsigned char *in, int in_len, 
		unsigned char *tag, const unsigned char *key, 
		const unsigned char *iv, unsigned char *out);

#endif
