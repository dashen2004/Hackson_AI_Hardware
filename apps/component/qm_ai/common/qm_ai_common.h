#ifndef __QM_AI_COMMON_H__
#define __QM_AI_COMMON_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "qm.h"

typedef enum
{
    QM_AI_COMM_TYPE_RTC = 0,
    QM_AI_COMM_TYPE_WEBSOCKET,
    QM_AI_COMM_TYPE_CUSTOM,
}qm_ai_comm_type_t;

/**
 * Audio codec type list.
 */
typedef enum {
  /**
   * 0: Disable
   */
  QM_AI_AUDIO_CODEC_DISABLED = 0,
  /**
   * 1: OPUS
   */
  QM_AI_AUDIO_CODEC_TYPE_OPUS = 1,
  /**
   * 2: G722
   */
  QM_AI_AUDIO_CODEC_TYPE_G722 = 2,
  /**
   * 3: G711A
   */
  QM_AI_AUDIO_CODEC_TYPE_G711A = 3,
  /**
   * 4: G711U
   */
  QM_AI_AUDIO_CODEC_TYPE_G711U = 4,
} qm_ai_audio_codec_type_t;

/**
 * Audio data type list.
 */
typedef enum {
  /**
   * 1: OPUS, sample=16k
   */
  QM_AI_AUDIO_DATA_TYPE_OPUS = 1,
  /**
   * 2: OPUSFB, sample=48k
   */
  QM_AI_AUDIO_DATA_TYPE_OPUSFB = 2,
  /**
   * 3: PCMA, sample=8k
   */
  QM_AI_AUDIO_DATA_TYPE_PCMA = 3,
  /**
   * 4: PCMU, sample=8k
   */
  QM_AI_AUDIO_DATA_TYPE_PCMU = 4,
  /**
   * 5: G722, sample=16k
   */
  QM_AI_AUDIO_DATA_TYPE_G722 = 5,
  /*
   * 6: AACLC sample=8k
   */
  QM_AI_AUDIO_DATA_TYPE_AACLC_8K = 6,
  /*
   * 7: AACLC sample=16k
   */
  QM_AI_AUDIO_DATA_TYPE_AACLC_16K = 7,
  /**
   * 8: AACLC, sample=48k
   */
  QM_AI_AUDIO_DATA_TYPE_AACLC = 8,
  /**
   * 9: HEAAC, sample=32k
   */
  QM_AI_AUDIO_DATA_TYPE_HEAAC = 9,
  /**
   * 10: HEAAC2, sample=48k
   */
  QM_AI_AUDIO_DATA_TYPE_HEAAC2 = 10,
  /*
   * 11: AACLC3 sample=8k 2CH
   */
  QM_AI_AUDIO_DATA_TYPE_AACLC_8K_2CH = 11,
  /*
   * 12: AACLC2 sample=16k 2CH
   */
  QM_AI_AUDIO_DATA_TYPE_AACLC_16K_2CH = 12,
  /**
   * 13: AACLC, sample=48k 2CH
   */
  QM_AI_AUDIO_DATA_TYPE_AACLC_2CH = 13,
  /**
   * 14: HEAAC, sample=48k 2CH
   */
  QM_AI_AUDIO_DATA_TYPE_HEAAC_2CH = 14,
  /**
   * 15: HEAAC2, sample=48k 2CH
   */
  QM_AI_AUDIO_DATA_TYPE_HEAAC2_2CH = 15,
  /**
   * 100: PCM (audio codec should be enabled)
   */
  QM_AI_AUDIO_DATA_TYPE_PCM = 100,
  /**
   * 253: GENERIC
   */
  QM_AI_AUDIO_DATA_TYPE_GENERIC = 253,
} qm_ai_audio_data_type_t;

/**
 * The definition of the video_data_type_e enum.
 */
typedef enum {
  /* 0: YUV420 */
  QM_AI_VIDEO_DATA_TYPE_YUV420 = 0,
  /* 2: H264 */
  QM_AI_VIDEO_DATA_TYPE_H264 = 2,
  /* 3: H265 */
  QM_AI_VIDEO_DATA_TYPE_H265 = 3,
  /* 6: generic */
  QM_AI_VIDEO_DATA_TYPE_GENERIC = 6,
  /* 20: generic JPEG */
  QM_AI_VIDEO_DATA_TYPE_GENERIC_JPEG = 20,
} qm_ai_video_data_type_t;

/**
 * The definition of the video_frame_type_e enum.
 */
typedef enum {
  /**
   * 0: unknow frame type
   * If you set it ,the SDK will judge the frame type
   */
  QM_AI_VIDEO_FRAME_AUTO_DETECT = 0,
  /**
   * 3: key frame
   */
  QM_AI_VIDEO_FRAME_KEY = 3,
  /*
   * 4: delta frame, e.g: P-Frame
   */
  QM_AI_VIDEO_FRAME_DELTA = 4,
} qm_ai_video_frame_type_t;

typedef uint16_t qm_ai_video_frame_rate_t;

/**
 * Video stream types.
 */
typedef enum {
  /**
   * 0: The high-quality video stream, which has a higher resolution and bitrate.
   */
  QM_AI_VIDEO_STREAM_HIGH = 0,
  /**
   * 1: The low-quality video stream, which has a lower resolution and bitrate.
   */
  QM_AI_VIDEO_STREAM_LOW = 1,
} qm_ai_video_stream_type_t;

/*
 * Video rotation information.
 */
typedef enum {
  /**
   * 0: Rotate the video by 0 degree clockwise.
   */
  QM_AI_VIDEO_ORIENTATION_0 = 0,
  /**
   * 90: Rotate the video by 90 degrees clockwise.
   */
  QM_AI_VIDEO_ORIENTATION_90 = 1,
  /**
   * 180: Rotate the video by 180 degrees clockwise.
   */
  QM_AI_VIDEO_ORIENTATION_180 = 2,
  /**
   * 270: Rotate the video by 270 degrees clockwise.
   */
  QM_AI_VIDEO_ORIENTATION_270 = 3
} qm_ai_video_orientation_t;

typedef enum {
    QM_AI_CODE_SUCCESS = 0,
    QM_AI_CODE_ERROR,
}qm_ai_code_t;

#if defined(__cplusplus)
}
#endif

#endif  /* __QM_IOT_COMMON_H__ */

