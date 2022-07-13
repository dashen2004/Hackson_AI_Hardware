
#ifndef _TOTP_H_
#define _TOTP_H_

#include "qm_types.h"

/* Secret key size, in bytes. */
#define TOTP_SECRET_KEY_SIZE 6

/* Time padded size, in bytes. */
#define TOTP_TIME_PADDED    8

/* Interval for generating keys, in seconds. */
#define TOTP_KEY_INTERVAL 225

/* Amount of digits for the final key. */
#define TOTP_DIGITS 1000000

/* Default window size. */
#define TOTP_DEFAULT_WINDOW_SIZE 3

/* totp key sets */
#define TOTP_KEY_SETS     8

typedef uint32_t totp_t[TOTP_KEY_SETS];

/**
 * generate a password from given secret key and utc timestamp
 *
 * @param [in] : secret_key Your secret-key
 *
 * @param [in] : timestamp target time in Unix Time Stamp
 *
 * @param [out] : key password
 * 
 * @return 0:success,otherwise is error
 *
 */
extern int totp_get_key(const uint8_t *secret_key, uint32_t timestamp, uint32_t *key);

/**
 * generate 8 sets password from given secret key and utc timestamp
 *
 * @param [in] refresh_interval : OTP validity period in second. Range from 600s to 3600s.
 * 
 * @param [in] secret_key : Your secret-key
 *
 * @param [in] timestamp : target time in Unix Time Stamp
 * 
 * @param [out] p_totp : pointer the memory that OTP to be stored.
 * 
 * @param [out] p_last_totp : pointer the memory that last valid OTP to be stored
 *
 * @return 0:success,otherwise is error
 *
 */
extern int totp_get_keys(uint32_t refresh_interval, const uint8_t *secret_key, uint32_t timestamp, totp_t *p_totp, totp_t *p_last_totp);


#endif /* QM_TOTP_H */
