 #include <string.h>

 #include "esp_log.h"
 #include "esp_timer.h"
 #include "esp_check.h"
 #include "sdkconfig.h"
 
 #include "audio_element.h"
 #include "raw_stream.h"
 #include "filter_resample.h"
 #include "audio_mem.h"
 #include "audio_thread.h"
 #include "board.h"
 #include "es7210.h"
 
#include "qm.h"
#include "qm_audio_sr.h"
#include "qm_audio_vc.h"
#include "qm_audio_encoder.h"
#include "qm_audio_recorder.h"
#include "qm_audio_stream.h"
#include "qm_audio_processor.h"

#include "qm_audio_board.h"

#define CONFIG_AUDIO_SUPPORT_OPUS_DECODER 1
 
 #if defined (CONFIG_AUDIO_SUPPORT_OPUS_DECODER)
 #include "raw_opus_encoder.h"
 #include "raw_opus_decoder.h"
 #elif defined (CONFIG_AUDIO_SUPPORT_AAC_DECODER)
 #include "aac_encoder.h"
 #include "aac_decoder.h"
 #elif defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
 #include "g711_encoder.h"
 #include "g711_decoder.h"
 #endif

 
 static const char *TAG = "audio processor";
 
 #define audio_pipe_safe_free(x, fn) do { \
     if (x) {                             \
         fn(x);                           \
         x = NULL;                        \
     }                                    \
 } while (0)
 
 struct recorder_pipeline_t {
     audio_pipeline_handle_t audio_pipeline;
     audio_element_handle_t  i2s_stream_reader;
     audio_element_handle_t  audio_encoder;
     audio_element_handle_t  raw_reader;
     audio_rec_handle_t      recorder_engine;
     pipe_player_state_e     record_state;
     qm_audio_board_handle_t board_handle;
 };
 
 struct player_pipeline_t {
     audio_pipeline_handle_t audio_pipeline;
     audio_element_handle_t  raw_writer;
     audio_element_handle_t  audio_decoder;
     audio_element_handle_t  i2s_stream_writer;
     audio_element_handle_t  player_rsp;
     pipe_player_state_e     player_state;
 };
 
 typedef struct {
     audio_pipeline_handle_t pipeline;
     audio_element_handle_t  spiffs_stream;
     audio_element_handle_t  audio_decoder; // only support for wav; 16k, 16bit, double channel
     audio_element_handle_t  i2s_stream_writer;
     pipe_player_state_e     player_state;
     bool                    running;
     tone_play_callback_t    tone_cb;
 } audio_player_t;
 
 static audio_player_t             *s_audio_player      = NULL;
 static struct recorder_pipeline_t *s_recorder_pipeline = NULL;
 static struct player_pipeline_t   *s_player_pipeline   = NULL;
 
 recorder_pipeline_handle_t recorder_pipeline_open(void)
 {
     ESP_LOGI(TAG, "%s", __func__);

     recorder_pipeline_handle_t pipeline = audio_calloc(1, sizeof(struct recorder_pipeline_t));
     pipeline->record_state = PIPE_STATE_IDLE;
     s_recorder_pipeline = pipeline;
 
     ESP_LOGI(TAG, "Create audio pipeline for recording");
     audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
     pipeline->audio_pipeline = audio_pipeline_init(&pipeline_cfg);
     mem_assert(pipeline->audio_pipeline);

     pipeline->board_handle = qm_audio_board_init();
     mem_assert(pipeline->board_handle);
     audio_hal_ctrl_codec(pipeline->board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

     ESP_LOGI(TAG, "Create record audio stream");
     pipeline->i2s_stream_reader = create_record_i2s_stream();
     pipeline->raw_reader = create_record_raw_stream();
 
     ESP_LOGI(TAG, " Register all record elements to audio pipeline");
     audio_pipeline_register(pipeline->audio_pipeline, pipeline->i2s_stream_reader, "record_i2s");
     audio_pipeline_register(pipeline->audio_pipeline, pipeline->raw_reader, "record_raw");
 
     ESP_LOGI(TAG, " Link all record elements to audio pipeline");
     const char *link_tag[2] = {"record_i2s", "record_raw"};
     audio_pipeline_link(pipeline->audio_pipeline, &link_tag[0], 2);
     return pipeline;
 }
 
 esp_err_t recorder_pipeline_run(recorder_pipeline_handle_t pipeline)
 {
     ESP_RETURN_ON_FALSE(pipeline != NULL, ESP_FAIL, TAG, "recorder pipeline not initialized");
     audio_pipeline_run(pipeline->audio_pipeline);
     pipeline->record_state = PIPE_STATE_RUNNING;
     return ESP_OK;
 };
 
 esp_err_t recorder_pipeline_stop(recorder_pipeline_handle_t pipeline)
 {
     ESP_RETURN_ON_FALSE(pipeline != NULL, ESP_FAIL, TAG, "recorder pipeline not initialized");
     audio_pipeline_stop(pipeline->audio_pipeline);
     audio_pipeline_wait_for_stop(pipeline->audio_pipeline);
     pipeline->record_state = PIPE_STATE_IDLE;
     return ESP_OK;
 };
 
 esp_err_t recorder_pipeline_close(recorder_pipeline_handle_t pipeline)
 {
     ESP_RETURN_ON_FALSE(pipeline != NULL, ESP_FAIL, TAG, "recorder pipeline not initialized");
     audio_pipeline_terminate(pipeline->audio_pipeline);
     audio_pipeline_unregister(pipeline->audio_pipeline, pipeline->i2s_stream_reader);
     audio_pipeline_unregister(pipeline->audio_pipeline, pipeline->raw_reader);
 
     /* Release all resources */
     audio_pipe_safe_free(pipeline->audio_pipeline, audio_pipeline_deinit);
     audio_pipe_safe_free(pipeline->raw_reader, audio_element_deinit);
     audio_pipe_safe_free(pipeline->i2s_stream_reader, audio_element_deinit);
     audio_pipe_safe_free(pipeline, audio_free);
     return ESP_OK;
 };
 
 int recorder_pipeline_get_default_read_size(recorder_pipeline_handle_t pipeline)
 {
     #if defined (CONFIG_AUDIO_SUPPORT_OPUS_DECODER)
         return 960;
     #elif defined (CONFIG_AUDIO_SUPPORT_AAC_DECODER)
         return 512;
     #elif defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
         return 160;
     #endif
     return -1;
 };
 
 audio_element_handle_t recorder_pipeline_get_raw_reader(recorder_pipeline_handle_t pipeline)
 {
     return pipeline->raw_reader;
 }
 
 audio_pipeline_handle_t recorder_pipeline_get_pipeline(recorder_pipeline_handle_t pipeline)
 {
     return pipeline->audio_pipeline;
 };
 
 int recorder_pipeline_read(recorder_pipeline_handle_t pipeline,char *buffer, int buf_size)
 {
     return raw_stream_read(pipeline->raw_reader, buffer,buf_size);
 }
 
 static esp_err_t _player_i2s_write_cb(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
 {    
     return audio_element_output(s_player_pipeline->i2s_stream_writer, buffer, len);
 }
 
 static esp_err_t _player_write_nop_cb(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
 {
     return len;
 }

 int player_pipeline_write(char *buffer, int buf_size)
 {
    return audio_element_output(s_player_pipeline->i2s_stream_writer, buffer, buf_size);
 }
 
 player_pipeline_handle_t player_pipeline_open(void) 
 {
     ESP_LOGI(TAG, "%s", __func__);
     player_pipeline_handle_t player_pipeline = audio_calloc(1, sizeof(struct player_pipeline_t));
     AUDIO_MEM_CHECK(TAG, player_pipeline, goto _exit_open);
     player_pipeline->player_state = PIPE_STATE_IDLE;
     s_player_pipeline = player_pipeline;
 
     ESP_LOGI(TAG, "Create audio pipeline for playback");
     audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
     player_pipeline->audio_pipeline = audio_pipeline_init(&pipeline_cfg);
     AUDIO_MEM_CHECK(TAG, player_pipeline, goto _exit_open);
 
     ESP_LOGI(TAG, "Create playback audio stream");
     player_pipeline->raw_writer = create_player_raw_stream();
     AUDIO_MEM_CHECK(TAG, player_pipeline->raw_writer, goto _exit_open);
     player_pipeline->i2s_stream_writer = create_player_i2s_stream(false);
     AUDIO_MEM_CHECK(TAG, player_pipeline->i2s_stream_writer, goto _exit_open);
 
     ESP_LOGI(TAG, "Register all elements to playback pipeline");
     audio_pipeline_register(player_pipeline->audio_pipeline, player_pipeline->raw_writer, "player_raw");
 
     player_pipeline->player_rsp = create_ch1_to_ch2_rsp_stream();   
     AUDIO_MEM_CHECK(TAG, player_pipeline->player_rsp, goto _exit_open);
 
     ESP_LOGI(TAG, "Link playback element together raw-->audio_decoder-->rsp-->i2s_stream-->[codec_chip]");
 
     audio_element_set_write_cb(player_pipeline->player_rsp, _player_write_nop_cb, NULL);
     audio_pipeline_register(player_pipeline->audio_pipeline, player_pipeline->player_rsp, "player_rsp");
     const char *link_tag[3] = {"player_raw", "player_rsp"};
     audio_pipeline_link(player_pipeline->audio_pipeline, &link_tag[0], 2);
 
     audio_pipeline_run(player_pipeline->audio_pipeline);
     return player_pipeline;
 
 _exit_open:
     audio_pipe_safe_free(player_pipeline->player_rsp, audio_element_deinit);
     audio_pipe_safe_free(player_pipeline->raw_writer, audio_element_deinit);
     audio_pipe_safe_free(player_pipeline->audio_decoder, audio_element_deinit);
     audio_pipe_safe_free(player_pipeline->i2s_stream_writer, audio_element_deinit);
     audio_pipe_safe_free(player_pipeline->audio_pipeline, audio_pipeline_deinit);
     audio_pipe_safe_free(player_pipeline, audio_free);
     return NULL;
 }
 
 esp_err_t player_pipeline_run(player_pipeline_handle_t player_pipeline)
 {
     ESP_RETURN_ON_FALSE(player_pipeline != NULL, ESP_FAIL, TAG, "player pipeline not initialized");
     if (player_pipeline->player_state == PIPE_STATE_RUNNING) {
         ESP_LOGW(TAG, "player pipe is already running state");
         return ESP_OK;
     }
     ESP_LOGI(TAG, "player pipe start running");
     audio_element_set_write_cb(player_pipeline->player_rsp, _player_i2s_write_cb, NULL);
     player_pipeline->player_state = PIPE_STATE_RUNNING;
     return ESP_OK;
 };
 
 esp_err_t player_pipeline_stop(player_pipeline_handle_t player_pipeline)
 {
     ESP_RETURN_ON_FALSE(player_pipeline != NULL, ESP_FAIL, TAG, "player pipeline not initialized");
     if (player_pipeline->player_state == PIPE_STATE_IDLE) {
         ESP_LOGW(TAG, "player pipe is idle state");
         return ESP_FAIL;
     }
     ESP_LOGI(TAG, "player pipe stop running");
     audio_element_set_write_cb(player_pipeline->player_rsp, _player_write_nop_cb, NULL);
     player_pipeline->player_state = PIPE_STATE_IDLE;
     return ESP_OK;
 }
 
 esp_err_t player_pipeline_get_state(player_pipeline_handle_t player_pipeline, pipe_player_state_e *state)
 {
     ESP_RETURN_ON_FALSE(player_pipeline != NULL, ESP_FAIL, TAG, "player pipeline not initialized");
     *state = player_pipeline->player_state;
     return ESP_OK;
 }
 
 esp_err_t player_pipeline_close(player_pipeline_handle_t player_pipeline)
 {
     ESP_RETURN_ON_FALSE(player_pipeline != NULL, ESP_FAIL, TAG, "player pipeline not initialized");
     if (player_pipeline->player_state == PIPE_STATE_RUNNING) {
         ESP_LOGW(TAG, "Please stop player pipe first");
         return ESP_FAIL;
     }
     audio_pipeline_terminate(player_pipeline->audio_pipeline);
 
     audio_pipeline_unregister(player_pipeline->audio_pipeline, player_pipeline->raw_writer);
     audio_pipeline_unregister(player_pipeline->audio_pipeline, player_pipeline->i2s_stream_writer);
     audio_pipeline_unregister(player_pipeline->audio_pipeline, player_pipeline->audio_decoder);
     audio_pipeline_unregister(player_pipeline->audio_pipeline, player_pipeline->player_rsp);
 
     /* Release all resources */
     audio_pipe_safe_free(player_pipeline->player_rsp, audio_element_deinit);
     audio_pipe_safe_free(player_pipeline->raw_writer, audio_element_deinit);
     audio_pipe_safe_free(player_pipeline->i2s_stream_writer, audio_element_deinit);
     audio_pipe_safe_free(player_pipeline->audio_decoder, audio_element_deinit);
     audio_pipe_safe_free(player_pipeline->audio_pipeline, audio_pipeline_deinit);
     audio_pipe_safe_free(player_pipeline, audio_free);
     return ESP_OK;
 };
 
 // Just create a placeholder function
 int player_pipeline_get_default_read_size(player_pipeline_handle_t player_pipeline)
 {
     return 0;
 };
 
 audio_element_handle_t player_pipeline_get_raw_write(player_pipeline_handle_t player_pipeline)
 {
    return player_pipeline->raw_writer; 
 }
 
 static int input_cb_for_afe(int16_t *buffer, int buf_sz, void *user_ctx, TickType_t ticks)
 {
    return raw_stream_read(s_recorder_pipeline->raw_reader, (char *)buffer, buf_sz);
 }
 
 void * audio_record_engine_init(recorder_pipeline_handle_t pipeline, pipe_record_engine_params_t *params)
 {
     qm_audio_encoder_cfg_t recorder_encoder_cfg = { 0 };
     qm_audio_sr_cfg_t *recorder_sr_cfg = get_default_audio_sr_config();
     qm_audio_vc_cfg_t *recorder_vc_cfg = get_default_audio_vc_config();

 #if defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
     rsp_filter_cfg_t filter_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
     filter_cfg.src_ch = 1;
     filter_cfg.src_rate = 16000;
     filter_cfg.dest_ch = 1;
     filter_cfg.dest_rate = 8000;
     filter_cfg.complexity = 0;
     filter_cfg.stack_in_ext = true;
     filter_cfg.max_indata_bytes = 1024;
     recorder_encoder_cfg.resample = rsp_filter_init(&filter_cfg);
 #endif // CONFIG_AUDIO_SUPPORT_G711A_DECODER
 
     recorder_encoder_cfg.encoder = create_record_encoder_stream();
 
     audio_rec_cfg_t cfg = AUDIO_RECORDER_DEFAULT_CFG();
     cfg.read = (qm_audio_data_read_t)&input_cb_for_afe;
     cfg.event_cb = params->cb;
     cfg.wakeup_time = 120000;
     
     if(recorder_vc_cfg){
        cfg.vc_handle = qm_audio_vc_create(recorder_vc_cfg, &cfg.vc_iface);
     }
     
     if(recorder_sr_cfg){
        cfg.sr_handle = qm_audio_sr_create(recorder_sr_cfg, &cfg.sr_iface);
        if(params->command_str && params->command_num){
            qm_audio_sr_add_speech_cmd(cfg.sr_handle, params->command_str, params->command_num);
        }
     }

     if(recorder_encoder_cfg.encoder){
        cfg.encoder_handle = qm_audio_encoder_create(&recorder_encoder_cfg, &cfg.encoder_iface);
     }

     pipeline->recorder_engine = qm_audio_recorder_create(&cfg);
#if 0
     audio_recorder_trigger_start(pipeline->recorder_engine);
 #endif // CONFIG_CONTINUOUS_CONVERSATION_MODE
     return pipeline->recorder_engine;
 }
 