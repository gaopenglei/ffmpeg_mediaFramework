/**
 * @file media_types.h
 * @brief 媒体框架基础类型定义
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 本文件定义了媒体框架中使用的基础数据类型、枚举和结构体。
 * 所有模块共享这些类型定义，确保接口一致性。
 */

#ifndef MEDIA_TYPES_H
#define MEDIA_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 版本信息
 * ============================================================================ */

#define MEDIA_FRAMEWORK_VERSION_MAJOR 1
#define MEDIA_FRAMEWORK_VERSION_MINOR 0
#define MEDIA_FRAMEWORK_VERSION_PATCH 0
#define MEDIA_FRAMEWORK_VERSION_STRING "1.0.0"

/* ============================================================================
 * 错误码定义
 * ============================================================================ */

/**
 * @brief 错误码枚举
 * 
 * 所有API函数返回此枚举值，用于指示操作结果。
 * 负值表示错误，正值表示特殊状态，0表示成功。
 */
typedef enum {
    /* 成功状态 */
    MEDIA_SUCCESS = 0,
    MEDIA_NEED_MORE_DATA = 1,
    MEDIA_EOF = 2,
    MEDIA_BUFFER_FULL = 3,
    
    /* 通用错误 (-1 ~ -99) */
    MEDIA_ERROR_UNKNOWN = -1,
    MEDIA_ERROR_INVALID_PARAM = -2,
    MEDIA_ERROR_NULL_POINTER = -3,
    MEDIA_ERROR_OUT_OF_MEMORY = -4,
    MEDIA_ERROR_NOT_SUPPORTED = -5,
    MEDIA_ERROR_NOT_INITIALIZED = -6,
    MEDIA_ERROR_ALREADY_INITIALIZED = -7,
    MEDIA_ERROR_TIMEOUT = -8,
    MEDIA_ERROR_INTERRUPTED = -9,
    
    /* 文件/IO错误 (-100 ~ -199) */
    MEDIA_ERROR_FILE_NOT_FOUND = -100,
    MEDIA_ERROR_FILE_OPEN_FAILED = -101,
    MEDIA_ERROR_FILE_READ_FAILED = -102,
    MEDIA_ERROR_FILE_WRITE_FAILED = -103,
    MEDIA_ERROR_FILE_SEEK_FAILED = -104,
    MEDIA_ERROR_FILE_FORMAT = -105,
    MEDIA_ERROR_FILE_CORRUPTED = -106,
    
    /* 编解码错误 (-200 ~ -299) */
    MEDIA_ERROR_CODEC_NOT_FOUND = -200,
    MEDIA_ERROR_CODEC_INIT_FAILED = -201,
    MEDIA_ERROR_DECODE_FAILED = -202,
    MEDIA_ERROR_ENCODE_FAILED = -203,
    MEDIA_ERROR_CODEC_UNSUPPORTED = -204,
    
    /* 流处理错误 (-300 ~ -399) */
    MEDIA_ERROR_STREAM_NOT_FOUND = -300,
    MEDIA_ERROR_STREAM_INVALID = -301,
    MEDIA_ERROR_STREAM_END = -302,
    MEDIA_ERROR_STREAM_DISCONNECTED = -303,
    
    /* 滤镜错误 (-400 ~ -499) */
    MEDIA_ERROR_FILTER_NOT_FOUND = -400,
    MEDIA_ERROR_FILTER_INIT_FAILED = -401,
    MEDIA_ERROR_FILTER_PROCESS_FAILED = -402,
    MEDIA_ERROR_FILTER_CHAIN_INVALID = -403,
    
    /* 设备错误 (-500 ~ -599) */
    MEDIA_ERROR_DEVICE_NOT_FOUND = -500,
    MEDIA_ERROR_DEVICE_OPEN_FAILED = -501,
    MEDIA_ERROR_DEVICE_BUSY = -502,
    MEDIA_ERROR_DEVICE_PERMISSION = -503,
    
    /* 网络错误 (-600 ~ -699) */
    MEDIA_ERROR_NETWORK_CONNECT = -600,
    MEDIA_ERROR_NETWORK_TIMEOUT = -601,
    MEDIA_ERROR_NETWORK_DNS = -602,
    MEDIA_ERROR_NETWORK_PROTOCOL = -603,
    
} MediaErrorCode;

/* ============================================================================
 * 媒体类型枚举
 * ============================================================================ */

/**
 * @brief 媒体流类型
 */
typedef enum {
    MEDIA_STREAM_TYPE_UNKNOWN = 0,
    MEDIA_STREAM_TYPE_VIDEO = 1,
    MEDIA_STREAM_TYPE_AUDIO = 2,
    MEDIA_STREAM_TYPE_SUBTITLE = 3,
    MEDIA_STREAM_TYPE_DATA = 4,
    MEDIA_STREAM_TYPE_ATTACHMENT = 5,
} MediaStreamType;

/**
 * @brief 像素格式
 * 
 * 支持常见的像素格式，与FFmpeg的AVPixelFormat对应
 */
typedef enum {
    MEDIA_PIX_FMT_NONE = -1,
    MEDIA_PIX_FMT_YUV420P = 0,
    MEDIA_PIX_FMT_YUYV422 = 1,
    MEDIA_PIX_FMT_RGB24 = 2,
    MEDIA_PIX_FMT_BGR24 = 3,
    MEDIA_PIX_FMT_YUV422P = 4,
    MEDIA_PIX_FMT_YUV444P = 5,
    MEDIA_PIX_FMT_YUV410P = 6,
    MEDIA_PIX_FMT_YUV411P = 7,
    MEDIA_PIX_FMT_GRAY8 = 8,
    MEDIA_PIX_FMT_MONOWHITE = 9,
    MEDIA_PIX_FMT_MONOBLACK = 10,
    MEDIA_PIX_FMT_PAL8 = 11,
    MEDIA_PIX_FMT_YUVJ420P = 12,
    MEDIA_PIX_FMT_YUVJ422P = 13,
    MEDIA_PIX_FMT_YUVJ444P = 14,
    MEDIA_PIX_FMT_UYVY422 = 15,
    MEDIA_PIX_FMT_UYYVYY411 = 16,
    MEDIA_PIX_FMT_BGR8 = 17,
    MEDIA_PIX_FMT_BGR4 = 18,
    MEDIA_PIX_FMT_BGR4_BYTE = 19,
    MEDIA_PIX_FMT_RGB8 = 20,
    MEDIA_PIX_FMT_RGB4 = 21,
    MEDIA_PIX_FMT_RGB4_BYTE = 22,
    MEDIA_PIX_FMT_NV12 = 23,
    MEDIA_PIX_FMT_NV21 = 24,
    MEDIA_PIX_FMT_ARGB = 25,
    MEDIA_PIX_FMT_RGBA = 26,
    MEDIA_PIX_FMT_ABGR = 27,
    MEDIA_PIX_FMT_BGRA = 28,
    MEDIA_PIX_FMT_GRAY16BE = 29,
    MEDIA_PIX_FMT_GRAY16LE = 30,
    MEDIA_PIX_FMT_YUV440P = 31,
    MEDIA_PIX_FMT_YUVJ440P = 32,
    MEDIA_PIX_FMT_YUVA420P = 33,
    MEDIA_PIX_FMT_RGB48BE = 34,
    MEDIA_PIX_FMT_RGB48LE = 35,
    MEDIA_PIX_FMT_RGB565BE = 36,
    MEDIA_PIX_FMT_RGB565LE = 37,
    MEDIA_PIX_FMT_RGB555BE = 38,
    MEDIA_PIX_FMT_RGB555LE = 39,
    MEDIA_PIX_FMT_BGR565BE = 40,
    MEDIA_PIX_FMT_BGR565LE = 41,
    MEDIA_PIX_FMT_BGR555BE = 42,
    MEDIA_PIX_FMT_BGR555LE = 43,
    MEDIA_PIX_FMT_VAAPI = 44,
    MEDIA_PIX_FMT_YUV420P16LE = 45,
    MEDIA_PIX_FMT_YUV420P16BE = 46,
    MEDIA_PIX_FMT_YUV422P16LE = 47,
    MEDIA_PIX_FMT_YUV422P16BE = 48,
    MEDIA_PIX_FMT_YUV444P16LE = 49,
    MEDIA_PIX_FMT_YUV444P16BE = 50,
    MEDIA_PIX_FMT_DXVA2_VLD = 51,
    MEDIA_PIX_FMT_RGB444BE = 52,
    MEDIA_PIX_FMT_RGB444LE = 53,
    MEDIA_PIX_FMT_BGR444BE = 54,
    MEDIA_PIX_FMT_BGR444LE = 55,
    MEDIA_PIX_FMT_YA8 = 56,
    MEDIA_PIX_FMT_YA16BE = 57,
    MEDIA_PIX_FMT_YA16LE = 58,
    MEDIA_PIX_FMT_QSV = 59,
    MEDIA_PIX_FMT_MMAL = 60,
    MEDIA_PIX_FMT_D3D11VA_VLD = 61,
    MEDIA_PIX_FMT_CUDA = 62,
    MEDIA_PIX_FMT_0RGB = 63,
    MEDIA_PIX_FMT_RGB0 = 64,
    MEDIA_PIX_FMT_0BGR = 65,
    MEDIA_PIX_FMT_BGR0 = 66,
    MEDIA_PIX_FMT_YUV420P12BE = 67,
    MEDIA_PIX_FMT_YUV420P12LE = 68,
    MEDIA_PIX_FMT_YUV420P14BE = 69,
    MEDIA_PIX_FMT_YUV420P14LE = 70,
    MEDIA_PIX_FMT_YUV422P12BE = 71,
    MEDIA_PIX_FMT_YUV422P12LE = 72,
    MEDIA_PIX_FMT_YUV422P14BE = 73,
    MEDIA_PIX_FMT_YUV422P14LE = 74,
    MEDIA_PIX_FMT_YUV444P12BE = 75,
    MEDIA_PIX_FMT_YUV444P12LE = 76,
    MEDIA_PIX_FMT_YUV444P14BE = 77,
    MEDIA_PIX_FMT_YUV444P14LE = 78,
    MEDIA_PIX_FMT_GBRP = 79,
    MEDIA_PIX_FMT_GBRP9BE = 80,
    MEDIA_PIX_FMT_GBRP9LE = 81,
    MEDIA_PIX_FMT_GBRP10BE = 82,
    MEDIA_PIX_FMT_GBRP10LE = 83,
    MEDIA_PIX_FMT_GBRP12BE = 84,
    MEDIA_PIX_FMT_GBRP12LE = 85,
    MEDIA_PIX_FMT_GBRP14BE = 86,
    MEDIA_PIX_FMT_GBRP14LE = 87,
    MEDIA_PIX_FMT_GBRP16BE = 88,
    MEDIA_PIX_FMT_GBRP16LE = 89,
    MEDIA_PIX_FMT_YUVA422P = 90,
    MEDIA_PIX_FMT_YUVA444P = 91,
    MEDIA_PIX_FMT_YUVA420P9BE = 92,
    MEDIA_PIX_FMT_YUVA420P9LE = 93,
    MEDIA_PIX_FMT_YUVA422P9BE = 94,
    MEDIA_PIX_FMT_YUVA422P9LE = 95,
    MEDIA_PIX_FMT_YUVA444P9BE = 96,
    MEDIA_PIX_FMT_YUVA444P9LE = 97,
    MEDIA_PIX_FMT_YUVA420P10BE = 98,
    MEDIA_PIX_FMT_YUVA420P10LE = 99,
    MEDIA_PIX_FMT_YUVA422P10BE = 100,
    MEDIA_PIX_FMT_YUVA422P10LE = 101,
    MEDIA_PIX_FMT_YUVA444P10BE = 102,
    MEDIA_PIX_FMT_YUVA444P10LE = 103,
    MEDIA_PIX_FMT_YUVA420P16BE = 104,
    MEDIA_PIX_FMT_YUVA420P16LE = 105,
    MEDIA_PIX_FMT_YUVA422P16BE = 106,
    MEDIA_PIX_FMT_YUVA422P16LE = 107,
    MEDIA_PIX_FMT_YUVA444P16BE = 108,
    MEDIA_PIX_FMT_YUVA444P16LE = 109,
    MEDIA_PIX_FMT_VIDEOTOOLBOX = 110,
    MEDIA_PIX_FMT_NB = 111,
    
    /* 别名定义 */
    MEDIA_PIX_FMT_GRAY16 = MEDIA_PIX_FMT_GRAY16LE,  /* 本地字节序 */
    MEDIA_PIX_FMT_RGB32 = MEDIA_PIX_FMT_BGRA,       /* 平台相关 */
    MEDIA_PIX_FMT_BGR32 = MEDIA_PIX_FMT_ARGB,       /* 平台相关 */
} MediaPixelFormat;

/**
 * @brief 音频采样格式
 */
typedef enum {
    MEDIA_SAMPLE_FMT_NONE = -1,
    MEDIA_SAMPLE_FMT_U8 = 0,          ///< unsigned 8 bits
    MEDIA_SAMPLE_FMT_S16 = 1,         ///< signed 16 bits
    MEDIA_SAMPLE_FMT_S32 = 2,         ///< signed 32 bits
    MEDIA_SAMPLE_FMT_FLT = 3,         ///< float
    MEDIA_SAMPLE_FMT_DBL = 4,         ///< double
    MEDIA_SAMPLE_FMT_U8P = 5,         ///< unsigned 8 bits, planar
    MEDIA_SAMPLE_FMT_S16P = 6,        ///< signed 16 bits, planar
    MEDIA_SAMPLE_FMT_S32P = 7,        ///< signed 32 bits, planar
    MEDIA_SAMPLE_FMT_FLTP = 8,        ///< float, planar
    MEDIA_SAMPLE_FMT_DBLP = 9,        ///< double, planar
    MEDIA_SAMPLE_FMT_S64 = 10,        ///< signed 64 bits
    MEDIA_SAMPLE_FMT_S64P = 11,       ///< signed 64 bits, planar
    MEDIA_SAMPLE_FMT_NB = 12,
} MediaSampleFormat;

/**
 * @brief 编码器ID
 * 
 * 支持LGPL许可的编解码器
 */
typedef enum {
    MEDIA_CODEC_ID_NONE = 0,
    
    /* 视频编码器 */
    MEDIA_CODEC_ID_H264 = 1,
    MEDIA_CODEC_ID_H265 = 2,         /* HEVC */
    MEDIA_CODEC_ID_HEVC = 2,         /* 别名 */
    MEDIA_CODEC_ID_VP8 = 3,
    MEDIA_CODEC_ID_VP9 = 4,
    MEDIA_CODEC_ID_MPEG4 = 5,
    MEDIA_CODEC_ID_MPEG2 = 6,        /* MPEG2 Video */
    MEDIA_CODEC_ID_MPEG2VIDEO = 6,   /* 别名 */
    MEDIA_CODEC_ID_MPEG1VIDEO = 7,
    MEDIA_CODEC_ID_THEORA = 8,
    MEDIA_CODEC_ID_AV1 = 9,
    MEDIA_CODEC_ID_FFV1 = 10,
    MEDIA_CODEC_ID_UTVIDEO = 11,
    MEDIA_CODEC_ID_MJPEG = 12,       /* Motion JPEG */
    
    /* 音频编码器 */
    MEDIA_CODEC_ID_AAC = 100,
    MEDIA_CODEC_ID_MP3 = 101,
    MEDIA_CODEC_ID_VORBIS = 102,
    MEDIA_CODEC_ID_OPUS = 103,
    MEDIA_CODEC_ID_FLAC = 104,
    MEDIA_CODEC_ID_WAVPACK = 105,
    MEDIA_CODEC_ID_PCM_S16LE = 106,
    MEDIA_CODEC_ID_PCM_S16BE = 107,
    MEDIA_CODEC_ID_PCM_U8 = 108,
    MEDIA_CODEC_ID_PCM_MULAW = 109,
    MEDIA_CODEC_ID_PCM_ALAW = 110,
    MEDIA_CODEC_ID_SPEEX = 111,
    MEDIA_CODEC_ID_WAV = 112,        /* WAV/PCM */
    MEDIA_CODEC_ID_AC3 = 113,        /* Dolby Digital */
    MEDIA_CODEC_ID_EAC3 = 114,       /* Dolby Digital Plus */
    MEDIA_CODEC_ID_AMR_NB = 115,     /* AMR Narrow Band */
    MEDIA_CODEC_ID_AMR_WB = 116,     /* AMR Wide Band */
    
    /* 字幕编码器 */
    MEDIA_CODEC_ID_SRT = 200,
    MEDIA_CODEC_ID_SUBRIP = 201,
    MEDIA_CODEC_ID_ASS = 202,
    MEDIA_CODEC_ID_SSA = 203,
    MEDIA_CODEC_ID_WEBVTT = 204,
    
} MediaCodecID;

/**
 * @brief 容器格式
 */
typedef enum {
    MEDIA_CONTAINER_NONE = 0,
    MEDIA_CONTAINER_MP4 = 1,
    MEDIA_CONTAINER_MKV = 2,
    MEDIA_CONTAINER_AVI = 3,
    MEDIA_CONTAINER_MOV = 4,
    MEDIA_CONTAINER_WEBM = 5,
    MEDIA_CONTAINER_FLV = 6,
    MEDIA_CONTAINER_MPEGTS = 7,
    MEDIA_CONTAINER_MPEGPS = 8,
    MEDIA_CONTAINER_MP3 = 9,
    MEDIA_CONTAINER_WAV = 10,
    MEDIA_CONTAINER_FLAC = 11,
    MEDIA_CONTAINER_OGG = 12,
    MEDIA_CONTAINER_AAC = 13,
    MEDIA_CONTAINER_M4A = 14,
    MEDIA_CONTAINER_3GP = 15,
    MEDIA_CONTAINER_TS = 16,
    MEDIA_CONTAINER_HLS = 17,
    MEDIA_CONTAINER_DASH = 18,
    MEDIA_CONTAINER_MXF = 19,
} MediaContainerFormat;

/**
 * @brief 流媒体协议
 */
typedef enum {
    MEDIA_PROTOCOL_NONE = 0,
    MEDIA_PROTOCOL_FILE = 1,
    MEDIA_PROTOCOL_RTMP = 2,
    MEDIA_PROTOCOL_RTSP = 3,
    MEDIA_PROTOCOL_HTTP = 4,
    MEDIA_PROTOCOL_HTTPS = 5,
    MEDIA_PROTOCOL_HLS = 6,
    MEDIA_PROTOCOL_DASH = 7,
    MEDIA_PROTOCOL_UDP = 8,
    MEDIA_PROTOCOL_RTP = 9,
    MEDIA_PROTOCOL_SRT = 10,
    MEDIA_PROTOCOL_WEBRTC = 11,
    MEDIA_PROTOCOL_TCP = 12,
} MediaProtocol;

/**
 * @brief 缩放算法
 */
typedef enum {
    MEDIA_SCALE_FAST_BILINEAR = 1,
    MEDIA_SCALE_BILINEAR = 2,
    MEDIA_SCALE_BICUBIC = 3,
    MEDIA_SCALE_X = 4,
    MEDIA_SCALE_POINT = 5,
    MEDIA_SCALE_AREA = 6,
    MEDIA_SCALE_BICUBLIN = 7,
    MEDIA_SCALE_GAUSS = 8,
    MEDIA_SCALE_SINC = 9,
    MEDIA_SCALE_LANCZOS = 10,
    MEDIA_SCALE_SPLINE = 11,
} MediaScaleAlgorithm;

/**
 * @brief 旋转角度
 */
typedef enum {
    MEDIA_ROTATE_NONE = 0,
    MEDIA_ROTATE_90_CW = 90,      ///< 顺时针90度
    MEDIA_ROTATE_180 = 180,
    MEDIA_ROTATE_90_CCW = 270,    ///< 逆时针90度（顺时针270度）
} MediaRotation;

/**
 * @brief 翻转方向
 */
typedef enum {
    MEDIA_FLIP_NONE = 0,
    MEDIA_FLIP_HORIZONTAL = 1,    ///< 水平翻转
    MEDIA_FLIP_VERTICAL = 2,      ///< 垂直翻转
} MediaFlipDirection;

/* ============================================================================
 * 结构体定义
 * ============================================================================ */

/**
 * @brief 有理数结构体
 * 
 * 用于表示帧率、采样率、时间基等需要精确表示的数值
 */
typedef struct {
    int32_t num;   ///< 分子
    int32_t den;   ///< 分母
} MediaRational;

/**
 * @brief 矩形区域
 */
typedef struct {
    int32_t x;      ///< 左上角X坐标
    int32_t y;      ///< 左上角Y坐标
    int32_t width;  ///< 宽度
    int32_t height; ///< 高度
} MediaRect;

/**
 * @brief 视频参数
 */
typedef struct {
    int32_t width;                    ///< 视频宽度（像素）
    int32_t height;                   ///< 视频高度（像素）
    MediaPixelFormat pixel_format;    ///< 像素格式
    MediaRational frame_rate;         ///< 帧率
    MediaRational time_base;          ///< 时间基
    MediaRational sample_aspect_ratio;///< 采样宽高比
    int32_t bit_rate;                 ///< 比特率（bps）
    int32_t gop_size;                 ///< GOP大小
    int32_t max_b_frames;             ///< 最大B帧数
    MediaCodecID codec_id;            ///< 编码器ID
    int32_t profile;                  ///< 编码配置
    int32_t level;                    ///< 编码级别
    int32_t refs;                     ///< 参考帧数
    int32_t thread_count;             ///< 编码线程数
    uint32_t flags;                   ///< 额外标志
} VideoParams;

/**
 * @brief 音频参数
 */
typedef struct {
    int32_t sample_rate;              ///< 采样率（Hz）
    int32_t channels;                 ///< 声道数
    int32_t channel_layout;           ///< 声道布局
    MediaSampleFormat sample_format;  ///< 采样格式
    MediaRational time_base;          ///< 时间基
    int32_t bit_rate;                 ///< 比特率（bps）
    int32_t frame_size;               ///< 每帧采样数
    int32_t block_align;              ///< 块对齐
    MediaCodecID codec_id;            ///< 编码器ID
    int32_t profile;                  ///< 编码配置
    uint32_t flags;                   ///< 额外标志
} AudioParams;

/**
 * @brief 媒体帧结构体
 */
typedef struct {
    uint8_t *data[8];         ///< 数据指针数组（平面格式）
    int32_t linesize[8];      ///< 每行字节数
    int64_t pts;              ///< 显示时间戳
    int64_t dts;              ///< 解码时间戳
    int64_t duration;         ///< 帧持续时间
    int32_t width;            ///< 视频帧宽度
    int32_t height;           ///< 视频帧高度
    int32_t nb_samples;       ///< 音频采样数
    int32_t format;           ///< 格式（像素或采样格式）
    int32_t key_frame;        ///< 是否为关键帧
    int32_t pict_type;        ///< 帧类型（I/P/B等）
    MediaStreamType type;     ///< 媒体类型
    int32_t stream_index;     ///< 流索引
    uint32_t flags;           ///< 额外标志
    void *opaque;             ///< 私有数据指针
} MediaFrame;

/**
 * @brief 媒体包结构体
 */
typedef struct {
    uint8_t *data;            ///< 数据指针
    int32_t size;             ///< 数据大小
    int64_t pts;              ///< 显示时间戳
    int64_t dts;              ///< 解码时间戳
    int64_t duration;         ///< 包持续时间
    int64_t pos;              ///< 文件位置
    int32_t stream_index;     ///< 流索引
    int32_t flags;            ///< 标志（关键帧等）
    int32_t side_data_count;  ///< 侧边数据数量
    void *side_data;          ///< 侧边数据
    void *opaque;             ///< 私有数据指针
} MediaPacket;

/**
 * @brief 流信息
 */
typedef struct {
    int32_t index;                  ///< 流索引
    MediaStreamType type;           ///< 流类型
    MediaCodecID codec_id;          ///< 编码器ID
    const char *codec_name;         ///< 编码器名称
    const char *codec_long_name;    ///< 编码器全称
    MediaRational time_base;        ///< 时间基
    int64_t duration;               ///< 时长（以时间基为单位）
    int64_t nb_frames;              ///< 总帧数
    int64_t start_time;             ///< 起始时间
    int32_t disposition;            ///< 流属性
    
    union {
        VideoParams video;          ///< 视频参数
        AudioParams audio;          ///< 音频参数
    } params;
    
    void *metadata;                 ///< 元数据字典
    void *codecpar;                 ///< 编码参数（FFmpeg兼容）
} MediaStreamInfo;

/**
 * @brief 媒体文件信息
 */
typedef struct {
    const char *filename;           ///< 文件名
    const char *format_name;        ///< 格式名称
    const char *format_long_name;   ///< 格式全称
    MediaContainerFormat container; ///< 容器格式
    int64_t duration;               ///< 总时长（微秒）
    int64_t size;                   ///< 文件大小（字节）
    int32_t bit_rate;               ///< 总比特率
    int32_t nb_streams;             ///< 流数量
    int32_t nb_video_streams;       ///< 视频流数量
    int32_t nb_audio_streams;       ///< 音频流数量
    int32_t nb_subtitle_streams;    ///< 字幕流数量
    MediaStreamInfo **streams;      ///< 流信息数组
    void *metadata;                 ///< 元数据字典
    int32_t seekable;               ///< 是否可寻址
    void *opaque;                   ///< 私有数据指针
} MediaFileInfo;

/* ============================================================================
 * 辅助函数声明
 * ============================================================================ */

/**
 * @brief 获取错误码对应的错误信息
 * @param error_code 错误码
 * @return 错误信息字符串
 */
const char* media_strerror(MediaErrorCode error_code);

/**
 * @brief 创建有理数
 * @param num 分子
 * @param den 分母
 * @return 有理数结构体
 */
MediaRational media_rational_make(int32_t num, int32_t den);

/**
 * @brief 有理数转浮点数
 * @param r 有理数
 * @return 浮点数值
 */
double media_rational_to_double(MediaRational r);

/**
 * @brief 简化有理数
 * @param r 有理数
 * @return 简化后的有理数
 */
MediaRational media_rational_simplify(MediaRational r);

/**
 * @brief 有理数加法
 * @param a 第一个有理数
 * @param b 第二个有理数
 * @return 结果
 */
MediaRational media_rational_add(MediaRational a, MediaRational b);

/**
 * @brief 有理数减法
 * @param a 第一个有理数
 * @param b 第二个有理数
 * @return 结果
 */
MediaRational media_rational_sub(MediaRational a, MediaRational b);

/**
 * @brief 有理数乘法
 * @param a 第一个有理数
 * @param b 第二个有理数
 * @return 结果
 */
MediaRational media_rational_mul(MediaRational a, MediaRational b);

/**
 * @brief 有理数除法
 * @param a 第一个有理数
 * @param b 第二个有理数
 * @return 结果
 */
MediaRational media_rational_div(MediaRational a, MediaRational b);

/**
 * @brief 比较两个有理数
 * @param a 第一个有理数
 * @param b 第二个有理数
 * @return 负数表示a<b，0表示相等，正数表示a>b
 */
int media_rational_compare(MediaRational a, MediaRational b);

/**
 * @brief 创建矩形
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 * @return 矩形结构体
 */
MediaRect media_rect_make(int32_t x, int32_t y, int32_t width, int32_t height);

/**
 * @brief 获取像素格式名称
 * @param format 像素格式
 * @return 格式名称字符串
 */
const char* media_pixel_format_name(MediaPixelFormat format);

/**
 * @brief 获取采样格式名称
 * @param format 采样格式
 * @return 格式名称字符串
 */
const char* media_sample_format_name(MediaSampleFormat format);

/**
 * @brief 获取编码器名称
 * @param codec_id 编码器ID
 * @return 编码器名称字符串
 */
const char* media_codec_name(MediaCodecID codec_id);

/**
 * @brief 获取容器格式名称
 * @param container 容器格式
 * @return 容器格式名称字符串
 */
const char* media_container_name(MediaContainerFormat container);

/**
 * @brief 获取协议名称
 * @param protocol 协议类型
 * @return 协议名称字符串
 */
const char* media_protocol_name(MediaProtocol protocol);

/**
 * @brief 判断编码器是否为视频编码器
 * @param codec_id 编码器ID
 * @return true表示是视频编码器
 */
bool media_codec_is_video(MediaCodecID codec_id);

/**
 * @brief 判断编码器是否为音频编码器
 * @param codec_id 编码器ID
 * @return true表示是音频编码器
 */
bool media_codec_is_audio(MediaCodecID codec_id);

/**
 * @brief 根据文件扩展名猜测容器格式
 * @param filename 文件名
 * @return 容器格式
 */
MediaContainerFormat media_guess_container_format(const char *filename);

/**
 * @brief 根据名称获取编码器ID
 * @param name 编码器名称
 * @return 编码器ID
 */
MediaCodecID media_codec_id_from_name(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_TYPES_H */
