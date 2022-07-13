#include "totp.h"
#include "qm_errno.h"
#include "qm_utils_hmac.h"
#include "qm_utils_string.h"


#define HEX_LOG(name, p_data, len) \
    do{\
        uint32_t i; \
        printf("[%s:%d]",__FUNCTION__,__LINE__);\
        printf("%s ",name);  \
        for(i=0;i<len;i++)\
        {\
          printf("%02X ", *(p_data+i));\
        }\
        printf("\n");\
      }while(0)

int totp_get_key(const uint8_t *secret_key, uint32_t timestamp, uint32_t *key)
{
	uint8_t sk[TOTP_SECRET_KEY_SIZE] = {0};		 /* Original secret key.      */
	uint8_t hmac[SHA1_DIGEST_LENGTH] = {0};		 /* HMAC.                     */
	char str_hmac[SHA1_DIGEST_LENGTH*2] = {0};
	uint8_t time_padded[TOTP_TIME_PADDED] = {0}; /* Time padded into 8 bytes. */
	uint32_t ctime = 0;							 /* Reference time.           */
	int offset = 0;								 /* Result offset.            */
	int result = 0;								 /* Result.                   */

	if (timestamp == 0 || timestamp == 0 || key == NULL){
		return -QM_EINVAL;
	}
	ctime = timestamp;

	//ctime /= TOTP_KEY_INTERVAL;

	/* Pad time into a byte array. */
	for (int i = TOTP_TIME_PADDED - 1; i >= 0; i--)
	{
		time_padded[i] = ctime;
		ctime >>= 8;
	}

	HEX_LOG("hmac", time_padded, 8);

	/* Calculates hmac. */
	qm_utils_hmac_sha1((char*)time_padded, TOTP_TIME_PADDED, str_hmac, (char*)secret_key, TOTP_SECRET_KEY_SIZE);

	hex_str_to_nums(str_hmac, SHA1_DIGEST_LENGTH*2, hmac);

	/* Get offset and result. */
	offset = hmac[SHA1_DIGEST_LENGTH - 1] & 0xF;
	result = (((hmac[offset + 0] & 0x7F) << 24) |
			  ((hmac[offset + 1] & 0xFF) << 16) |
			  ((hmac[offset + 2] & 0xFF) << 8) |
			  ((hmac[offset + 3] & 0xFF)));

	result &= 0x7FFFFFFF;
	result %= TOTP_DIGITS;

	*key = result;

	return QM_EOK;
}

int totp_get_keys(uint32_t refresh_interval, const uint8_t *secret_key, uint32_t timestamp, totp_t *p_totp, totp_t *p_last_totp)
{
	int i = 0;
	uint32_t interval = 0;
	uint32_t ctime = 0; 
	uint32_t last_ctime = 0; 
	uint32_t *key = (uint32_t*)p_totp;
	uint32_t *last_key = (uint32_t*)p_last_totp;

	if(secret_key == NULL || p_totp == NULL || p_last_totp == NULL){
		return -QM_EINVAL;
	}
	
	ctime = timestamp;

	ctime -= ctime % refresh_interval;
	last_ctime = ctime - refresh_interval;

	interval = refresh_interval/TOTP_KEY_SETS;

	for(i = 0; i<TOTP_KEY_SETS; i++){
		totp_get_key(secret_key, ctime, &key[i]);
		ctime += interval;
	}

	for(i = 0; i<TOTP_KEY_SETS; i++){
		totp_get_key(secret_key, last_ctime, &last_key[i]);
		last_ctime += interval;
	}

	return QM_EOK;
}
