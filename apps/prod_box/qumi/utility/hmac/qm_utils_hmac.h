
#ifndef _QM_UTILS_HMAC_H_
#define _QM_UTILS_HMAC_H_

int qm_utils_hmac_md5(const char *msg, int msg_len, char *digest, const char *key, int key_len);

int qm_utils_hmac_sha1(const char *msg, int msg_len, char *digest, const char *key, int key_len);

#endif

