/**
 * @file media_capture.c
 * @brief 媒体设备捕获实现
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 */

#include "input/media_capture.h"
#include "utils/media_utils.h"
#include <stdlib.h>
#include <string.h>

/* FFmpeg头文件 */
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavdevice/avdevice.h>
}

/* 内部捕获器结构体 */
struct MediaCapture {
    AVFormatContext *fmt_ctx;
    AVCodecContext *codec_ctx;
    AVFrame *frame;
    AVPacket *packet;
    
    char device_id[256];
    MediaDeviceType device_type;
    MediaCaptureState state;
    
    int stream_index;
    
    /* 视频参数 */
    int width;
    int height;
    MediaPixelFormat pixel_format;
    MediaRational frame_rate;
    
    /* 音频参数 */
    int sample_rate;
    int channels;
    MediaSampleFormat sample_format;
    
    /* 回调 */
    MediaCaptureCallback callback;
    void *callback_user_data;
    
    /* 线程控制 */
    int running;
    void *thread_handle;
};

/* ============================================================================
 * 全局初始化
 * ============================================================================ */

static int g_devices_initialized = 0;

static void init_devices(void)
{
    if (!g_devices_initialized) {
        avdevice_register_all();
        g_devices_initialized = 1;
    }
}

/* ============================================================================
 * 配置默认值
 * ============================================================================ */

void media_capture_video_config_default(VideoCaptureConfig *config)
{
    if (!config) return;
    memset(config, 0, sizeof(VideoCaptureConfig));
    config->width = 1280;
    config->height = 720;
    config->pixel_format = MEDIA_PIXEL_FORMAT_YUV420P;
    config->frame_rate = media_rational_make(30, 1);
    config->buffer_count = 4;
}

void media_capture_audio_config_default(AudioCaptureConfig *config)
{
    if (!config) return;
    memset(config, 0, sizeof(AudioCaptureConfig));
    config->sample_rate = 44100;
    config->channels = 2;
    config->sample_format = MEDIA_SAMPLE_FORMAT_S16;
    config->buffer_size = 4096;
}

/* ============================================================================
 * 设备枚举
 * ============================================================================ */

MediaErrorCode media_capture_enum_devices(MediaDeviceType type,
                                           MediaDeviceInfo *devices,
                                           int32_t max_count,
                                           int32_t *actual_count)
{
    init_devices();
    
    if (!devices || !actual_count) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    *actual_count = 0;
    
#ifdef _WIN32
    /* Windows平台使用dshow */
    AVFormatContext *fmt_ctx = avformat_alloc_context();
    AVInputFormat *input_fmt = av_find_input_format("dshow");
    
    if (input_fmt && fmt_ctx) {
        AVDictionary *options = NULL;
        av_dict_set(&options, "list_devices", "true", 0);
        
        /* 尝试打开设备列表 */
        int ret = avformat_open_input(&fmt_ctx, "video=dummy", input_fmt, &options);
        
        av_dict_free(&options);
        if (fmt_ctx) {
            avformat_close_input(&fmt_ctx);
        }
    }
    
#elif defined(__linux__)
    /* Linux平台使用v4l2/alsa */
    if (type == MEDIA_DEVICE_TYPE_VIDEO) {
        /* 枚举/dev/video*设备 */
        for (int i = 0; i < max_count && i < 10; i++) {
            char device_path[32];
            snprintf(device_path, sizeof(device_path), "/dev/video%d", i);
            
            AVFormatContext *fmt_ctx = NULL;
            AVInputFormat *input_fmt = av_find_input_format("v4l2");
            
            if (input_fmt) {
                int ret = avformat_open_input(&fmt_ctx, device_path, input_fmt, NULL);
                if (ret >= 0) {
                    snprintf(devices[*actual_count].device_id, 
                             sizeof(devices[*actual_count].device_id),
                             "%s", device_path);
                    snprintf(devices[*actual_count].name,
                             sizeof(devices[*actual_count].name),
                             "Video Device %d", i);
                    devices[*actual_count].type = MEDIA_DEVICE_TYPE_VIDEO;
                    devices[*actual_count].is_default = (i == 0);
                    (*actual_count)++;
                    avformat_close_input(&fmt_ctx);
                }
            }
        }
    } else if (type == MEDIA_DEVICE_TYPE_AUDIO) {
        /* 枚举ALSA设备 */
        for (int i = 0; i < max_count && i < 5; i++) {
            char device_name[64];
            snprintf(device_name, sizeof(device_name), "hw:%d", i);
            
            snprintf(devices[*actual_count].device_id,
                     sizeof(devices[*actual_count].device_id),
                     "%s", device_name);
            snprintf(devices[*actual_count].name,
                     sizeof(devices[*actual_count].name),
                     "Audio Device %d", i);
            devices[*actual_count].type = MEDIA_DEVICE_TYPE_AUDIO;
            devices[*actual_count].is_default = (i == 0);
            (*actual_count)++;
        }
    }
#endif
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 捕获器创建与销毁
 * ============================================================================ */

MediaCapture* media_capture_create(MediaDeviceType type)
{
    init_devices();
    
    MediaCapture *capture = (MediaCapture*)calloc(1, sizeof(MediaCapture));
    if (!capture) {
        return NULL;
    }
    
    capture->device_type = type;
    capture->state = MEDIA_CAPTURE_STATE_STOPPED;
    
    capture->frame = av_frame_alloc();
    capture->packet = av_packet_alloc();
    
    if (!capture->frame || !capture->packet) {
        if (capture->frame) av_frame_free(&capture->frame);
        if (capture->packet) av_packet_free(&capture->packet);
        free(capture);
        return NULL;
    }
    
    return capture;
}

void media_capture_destroy(MediaCapture **capture)
{
    if (!capture || !*capture) return;
    
    MediaCapture *c = *capture;
    
    media_capture_stop(c);
    
    if (c->codec_ctx) {
        avcodec_free_context(&c->codec_ctx);
    }
    
    if (c->fmt_ctx) {
        avformat_close_input(&c->fmt_ctx);
    }
    
    if (c->frame) {
        av_frame_free(&c->frame);
    }
    
    if (c->packet) {
        av_packet_free(&c->packet);
    }
    
    free(c);
    *capture = NULL;
}

/* ============================================================================
 * 设备打开与关闭
 * ============================================================================ */

MediaErrorCode media_capture_open_video(MediaCapture *capture,
                                         const char *device_id,
                                         const VideoCaptureConfig *config)
{
    if (!capture || !device_id) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    VideoCaptureConfig default_config;
    if (!config) {
        media_capture_video_config_default(&default_config);
        config = &default_config;
    }
    
    capture->width = config->width;
    capture->height = config->height;
    capture->pixel_format = config->pixel_format;
    capture->frame_rate = config->frame_rate;
    
    strncpy(capture->device_id, device_id, sizeof(capture->device_id) - 1);
    
    AVDictionary *options = NULL;
    char size_str[32];
    char fps_str[32];
    
    snprintf(size_str, sizeof(size_str), "%dx%d", config->width, config->height);
    snprintf(fps_str, sizeof(fps_str), "%d", config->frame_rate.num / config->frame_rate.den);
    
#ifdef _WIN32
    AVInputFormat *input_fmt = av_find_input_format("dshow");
    av_dict_set(&options, "video_size", size_str, 0);
    av_dict_set(&options, "framerate", fps_str, 0);
#else
    AVInputFormat *input_fmt = av_find_input_format("v4l2");
    av_dict_set(&options, "video_size", size_str, 0);
    av_dict_set(&options, "framerate", fps_str, 0);
#endif
    
    int ret = avformat_open_input(&capture->fmt_ctx, device_id, input_fmt, &options);
    av_dict_free(&options);
    
    if (ret < 0) {
        MEDIA_LOG_ERROR("Failed to open video device: %s\n", device_id);
        return MEDIA_ERROR_DEVICE_OPEN_FAILED;
    }
    
    /* 查找视频流 */
    ret = avformat_find_stream_info(capture->fmt_ctx, NULL);
    if (ret < 0) {
        avformat_close_input(&capture->fmt_ctx);
        return MEDIA_ERROR_STREAM_NOT_FOUND;
    }
    
    /* 查找视频流索引 */
    capture->stream_index = -1;
    for (unsigned int i = 0; i < capture->fmt_ctx->nb_streams; i++) {
        if (capture->fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            capture->stream_index = i;
            break;
        }
    }
    
    if (capture->stream_index < 0) {
        avformat_close_input(&capture->fmt_ctx);
        return MEDIA_ERROR_STREAM_NOT_FOUND;
    }
    
    /* 初始化解码器 */
    AVCodecParameters *codecpar = capture->fmt_ctx->streams[capture->stream_index]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        avformat_close_input(&capture->fmt_ctx);
        return MEDIA_ERROR_CODEC_NOT_FOUND;
    }
    
    capture->codec_ctx = avcodec_alloc_context3(codec);
    if (!capture->codec_ctx) {
        avformat_close_input(&capture->fmt_ctx);
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    avcodec_parameters_to_context(capture->codec_ctx, codecpar);
    
    ret = avcodec_open2(capture->codec_ctx, codec, NULL);
    if (ret < 0) {
        avcodec_free_context(&capture->codec_ctx);
        avformat_close_input(&capture->fmt_ctx);
        return MEDIA_ERROR_CODEC_OPEN_FAILED;
    }
    
    capture->width = capture->codec_ctx->width;
    capture->height = capture->codec_ctx->height;
    
    MEDIA_LOG_INFO("Video device opened: %s (%dx%d)\n", 
                   device_id, capture->width, capture->height);
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_capture_open_audio(MediaCapture *capture,
                                         const char *device_id,
                                         const AudioCaptureConfig *config)
{
    if (!capture || !device_id) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    AudioCaptureConfig default_config;
    if (!config) {
        media_capture_audio_config_default(&default_config);
        config = &default_config;
    }
    
    capture->sample_rate = config->sample_rate;
    capture->channels = config->channels;
    capture->sample_format = config->sample_format;
    
    strncpy(capture->device_id, device_id, sizeof(capture->device_id) - 1);
    
    AVDictionary *options = NULL;
    char rate_str[32];
    char ch_str[16];
    
    snprintf(rate_str, sizeof(rate_str), "%d", config->sample_rate);
    snprintf(ch_str, sizeof(ch_str), "%d", config->channels);
    
#ifdef _WIN32
    AVInputFormat *input_fmt = av_find_input_format("dshow");
    av_dict_set(&options, "sample_rate", rate_str, 0);
    av_dict_set(&options, "channels", ch_str, 0);
#else
    AVInputFormat *input_fmt = av_find_input_format("alsa");
    av_dict_set(&options, "sample_rate", rate_str, 0);
    av_dict_set(&options, "channels", ch_str, 0);
#endif
    
    int ret = avformat_open_input(&capture->fmt_ctx, device_id, input_fmt, &options);
    av_dict_free(&options);
    
    if (ret < 0) {
        MEDIA_LOG_ERROR("Failed to open audio device: %s\n", device_id);
        return MEDIA_ERROR_DEVICE_OPEN_FAILED;
    }
    
    ret = avformat_find_stream_info(capture->fmt_ctx, NULL);
    if (ret < 0) {
        avformat_close_input(&capture->fmt_ctx);
        return MEDIA_ERROR_STREAM_NOT_FOUND;
    }
    
    /* 查找音频流索引 */
    capture->stream_index = -1;
    for (unsigned int i = 0; i < capture->fmt_ctx->nb_streams; i++) {
        if (capture->fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            capture->stream_index = i;
            break;
        }
    }
    
    if (capture->stream_index < 0) {
        avformat_close_input(&capture->fmt_ctx);
        return MEDIA_ERROR_STREAM_NOT_FOUND;
    }
    
    /* 初始化解码器 */
    AVCodecParameters *codecpar = capture->fmt_ctx->streams[capture->stream_index]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        avformat_close_input(&capture->fmt_ctx);
        return MEDIA_ERROR_CODEC_NOT_FOUND;
    }
    
    capture->codec_ctx = avcodec_alloc_context3(codec);
    if (!capture->codec_ctx) {
        avformat_close_input(&capture->fmt_ctx);
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    avcodec_parameters_to_context(capture->codec_ctx, codecpar);
    
    ret = avcodec_open2(capture->codec_ctx, codec, NULL);
    if (ret < 0) {
        avcodec_free_context(&capture->codec_ctx);
        avformat_close_input(&capture->fmt_ctx);
        return MEDIA_ERROR_CODEC_OPEN_FAILED;
    }
    
    capture->sample_rate = capture->codec_ctx->sample_rate;
    capture->channels = capture->codec_ctx->channels;
    
    MEDIA_LOG_INFO("Audio device opened: %s (%d Hz, %d ch)\n",
                   device_id, capture->sample_rate, capture->channels);
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_capture_close(MediaCapture *capture)
{
    if (!capture) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    media_capture_stop(capture);
    
    if (capture->codec_ctx) {
        avcodec_free_context(&capture->codec_ctx);
    }
    
    if (capture->fmt_ctx) {
        avformat_close_input(&capture->fmt_ctx);
    }
    
    capture->state = MEDIA_CAPTURE_STATE_STOPPED;
    MEDIA_LOG_INFO("Capture device closed\n");
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 捕获控制
 * ============================================================================ */

MediaErrorCode media_capture_start(MediaCapture *capture)
{
    if (!capture) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (!capture->fmt_ctx) {
        return MEDIA_ERROR_NOT_INITIALIZED;
    }
    
    capture->running = 1;
    capture->state = MEDIA_CAPTURE_STATE_CAPTURING;
    
    MEDIA_LOG_INFO("Capture started\n");
    return MEDIA_SUCCESS;
}

MediaErrorCode media_capture_stop(MediaCapture *capture)
{
    if (!capture) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    capture->running = 0;
    capture->state = MEDIA_CAPTURE_STATE_STOPPED;
    
    MEDIA_LOG_INFO("Capture stopped\n");
    return MEDIA_SUCCESS;
}

MediaErrorCode media_capture_pause(MediaCapture *capture)
{
    if (!capture) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    capture->state = MEDIA_CAPTURE_STATE_PAUSED;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_capture_resume(MediaCapture *capture)
{
    if (!capture) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    capture->state = MEDIA_CAPTURE_STATE_CAPTURING;
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 帧读取
 * ============================================================================ */

MediaErrorCode media_capture_read_frame(MediaCapture *capture, MediaFrame **frame)
{
    if (!capture || !frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (!capture->running || capture->state != MEDIA_CAPTURE_STATE_CAPTURING) {
        return MEDIA_ERROR_INVALID_STATE;
    }
    
    int ret;
    
    while (capture->running) {
        ret = av_read_frame(capture->fmt_ctx, capture->packet);
        if (ret < 0) {
            if (ret == AVERROR(EAGAIN)) {
                continue;
            }
            return MEDIA_ERROR_READ_FAILED;
        }
        
        if (capture->packet->stream_index != capture->stream_index) {
            av_packet_unref(capture->packet);
            continue;
        }
        
        ret = avcodec_send_packet(capture->codec_ctx, capture->packet);
        if (ret < 0) {
            av_packet_unref(capture->packet);
            continue;
        }
        
        ret = avcodec_receive_frame(capture->codec_ctx, capture->frame);
        av_packet_unref(capture->packet);
        
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            return MEDIA_ERROR_DECODE_FAILED;
        }
        
        /* 创建输出帧 */
        *frame = media_frame_create();
        if (!*frame) {
            return MEDIA_ERROR_OUT_OF_MEMORY;
        }
        
        /* 复制帧数据 */
        av_frame_ref((AVFrame*)*frame, capture->frame);
        av_frame_unref(capture->frame);
        
        /* 调用回调 */
        if (capture->callback) {
            capture->callback(capture, *frame, capture->callback_user_data);
        }
        
        return MEDIA_SUCCESS;
    }
    
    return MEDIA_ERROR_INTERRUPTED;
}

/* ============================================================================
 * 回调设置
 * ============================================================================ */

void media_capture_set_callback(MediaCapture *capture,
                                 MediaCaptureCallback callback,
                                 void *user_data)
{
    if (!capture) return;
    
    capture->callback = callback;
    capture->callback_user_data = user_data;
}

/* ============================================================================
 * 状态查询
 * ============================================================================ */

MediaCaptureState media_capture_get_state(MediaCapture *capture)
{
    if (!capture) return MEDIA_CAPTURE_STATE_STOPPED;
    return capture->state;
}

MediaDeviceType media_capture_get_device_type(MediaCapture *capture)
{
    if (!capture) return MEDIA_DEVICE_TYPE_UNKNOWN;
    return capture->device_type;
}

const char* media_capture_get_device_id(MediaCapture *capture)
{
    if (!capture) return NULL;
    return capture->device_id;
}

int32_t media_capture_get_width(MediaCapture *capture)
{
    if (!capture) return 0;
    return capture->width;
}

int32_t media_capture_get_height(MediaCapture *capture)
{
    if (!capture) return 0;
    return capture->height;
}

int32_t media_capture_get_sample_rate(MediaCapture *capture)
{
    if (!capture) return 0;
    return capture->sample_rate;
}

int32_t media_capture_get_channels(MediaCapture *capture)
{
    if (!capture) return 0;
    return capture->channels;
}

/* ============================================================================
 * 屏幕捕获
 * ============================================================================ */

MediaErrorCode media_capture_enum_displays(MediaDeviceInfo *displays,
                                            int32_t max_count,
                                            int32_t *actual_count)
{
    if (!displays || !actual_count) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    *actual_count = 1;  /* 默认主显示器 */
    
    snprintf(displays[0].device_id, sizeof(displays[0].device_id), "screen");
    snprintf(displays[0].name, sizeof(displays[0].name), "Primary Display");
    displays[0].type = MEDIA_DEVICE_TYPE_SCREEN;
    displays[0].is_default = 1;
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_capture_enum_windows(MediaDeviceInfo *windows,
                                           int32_t max_count,
                                           int32_t *actual_count)
{
    if (!windows || !actual_count) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    *actual_count = 0;
    /* 需要平台特定的窗口枚举实现 */
    return MEDIA_SUCCESS;
}

MediaErrorCode media_capture_set_capture_region(MediaCapture *capture,
                                                 MediaRect rect)
{
    if (!capture) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    /* 设置捕获区域 */
    MEDIA_LOG_INFO("Setting capture region: %d,%d %dx%d\n",
                   rect.x, rect.y, rect.width, rect.height);
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_capture_set_show_cursor(MediaCapture *capture,
                                              bool show_cursor)
{
    if (!capture) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    MEDIA_LOG_INFO("Show cursor: %s\n", show_cursor ? "true" : "false");
    return MEDIA_SUCCESS;
}
