#ifndef _QM_PLAYER_DEV_H_
#define _QM_PLAYER_DEV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"
#include "qm_player.h"

typedef struct qm_player_dev qm_player_dev_t;

typedef struct {
    qm_err_t (*init)(qm_player_dev_t *player);
    qm_err_t (*deinit)(qm_player_dev_t *player);
    qm_err_t (*stop)(qm_player_dev_t *player);
    qm_err_t (*pause)(qm_player_dev_t *player);
    qm_err_t (*resume)(qm_player_dev_t *player);
    qm_err_t (*unmute)(qm_player_dev_t *player);
    qm_err_t (*mute)(qm_player_dev_t *player);
    qm_err_t (*play_file)(qm_player_dev_t *player, char *file_path);
    qm_err_t (*play_bin)(qm_player_dev_t *player, uint8_t *audio_buffer, int buffer_len);
}qm_player_dev_ops_t;

struct qm_player_dev
{
    qm_player_t handle;
    qm_player_dev_ops_t *ops;
    qm_player_event_t event_handler;
    void *arg;
    void *priv;   			/**< priv data */
};

extern int qm_hal_audio_player_init(qm_player_t handle);

int qm_player_dev_regsiter(qm_player_t handle, qm_player_dev_ops_t *ops, void *priv);

#ifdef __cplusplus
}
#endif

#endif
