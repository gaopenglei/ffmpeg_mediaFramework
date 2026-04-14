/**
 * @file media_context.c
 * @brief 媒体上下文管理实现
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 */

#include "core/media_context.h"
#include "utils/media_utils.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* FFmpeg头文件 */
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
}

/* 内部上下文结构体 */
struct MediaContext {
    int initialized;
    int log_level;
    MediaLogCallback log_callback;
    void *log_user_data;
    pthread_mutex_t mutex;
    
    /* 统计信息 */
    int64_t total_frames_processed;
    int64_t total_bytes_processed;
};

/* 全局上下文 */
static MediaContext *g_context = NULL;
static int g_initialized = 0;

/* FFmpeg日志回调 */
static void ffmpeg_log_callback(void *ptr, int level, const char *fmt, va_list vl)
{
    if (!g_context || !g_context->log_callback) {
        return;
    }
    
    MediaLogLevel media_level;
    switch (level) {
        case AV_LOG_PANIC:   media_level = MEDIA_LOG_PANIC; break;
        case AV_LOG_FATAL:   media_level = MEDIA_LOG_FATAL; break;
        case AV_LOG_ERROR:   media_level = MEDIA_LOG_ERROR; break;
        case AV_LOG_WARNING: media_level = MEDIA_LOG_WARNING; break;
        case AV_LOG_INFO:    media_level = MEDIA_LOG_INFO; break;
        case AV_LOG_VERBOSE: media_level = MEDIA_LOG_VERBOSE; break;
        case AV_LOG_DEBUG:   media_level = MEDIA_LOG_DEBUG; break;
        default:             media_level = MEDIA_LOG_TRACE; break;
    }
    
    char message[1024];
    vsnprintf(message, sizeof(message), fmt, vl);
    
    g_context->log_callback(media_level, NULL, 0, NULL, message, g_context->log_user_data);
}

/* ============================================================================
 * 全局初始化与清理
 * ============================================================================ */

MediaErrorCode media_init(void)
{
    if (g_initialized) {
        return MEDIA_ERROR_ALREADY_INITIALIZED;
    }
    
    g_context = (MediaContext*)calloc(1, sizeof(MediaContext));
    if (!g_context) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    pthread_mutex_init(&g_context->mutex, NULL);
    g_context->initialized = 1;
    g_context->log_level = MEDIA_LOG_INFO;
    
    /* 初始化FFmpeg网络 */
    avformat_network_init();
    
    /* 设置FFmpeg日志级别 */
    av_log_set_level(AV_LOG_INFO);
    
    g_initialized = 1;
    
    MEDIA_LOG_INFO("Media framework initialized (version %s)\n", 
                   MEDIA_FRAMEWORK_VERSION_STRING);
    
    return MEDIA_SUCCESS;
}

void media_cleanup(void)
{
    if (!g_initialized || !g_context) {
        return;
    }
    
    pthread_mutex_lock(&g_context->mutex);
    
    /* 清理FFmpeg网络 */
    avformat_network_deinit();
    
    g_context->initialized = 0;
    
    pthread_mutex_unlock(&g_context->mutex);
    pthread_mutex_destroy(&g_context->mutex);
    
    free(g_context);
    g_context = NULL;
    g_initialized = 0;
    
    printf("Media framework cleaned up\n");
}

/* ============================================================================
 * 上下文创建与销毁
 * ============================================================================ */

MediaContext* media_context_create(void)
{
    if (!g_initialized) {
        MEDIA_LOG_ERROR("Framework not initialized\n");
        return NULL;
    }
    
    MediaContext *ctx = (MediaContext*)calloc(1, sizeof(MediaContext));
    if (!ctx) {
        return NULL;
    }
    
    pthread_mutex_init(&ctx->mutex, NULL);
    ctx->initialized = 1;
    ctx->log_level = MEDIA_LOG_INFO;
    
    return ctx;
}

void media_context_destroy(MediaContext **ctx)
{
    if (!ctx || !*ctx) return;
    
    MediaContext *c = *ctx;
    
    pthread_mutex_destroy(&c->mutex);
    free(c);
    *ctx = NULL;
}

/* ============================================================================
 * 日志配置
 * ============================================================================ */

void media_context_set_log_level(MediaContext *ctx, MediaLogLevel level)
{
    if (!ctx) {
        if (g_context) {
            g_context->log_level = level;
            av_log_set_level(level);
        }
        return;
    }
    
    ctx->log_level = level;
    av_log_set_level(level);
}

MediaLogLevel media_context_get_log_level(MediaContext *ctx)
{
    if (!ctx) {
        return g_context ? g_context->log_level : MEDIA_LOG_INFO;
    }
    return ctx->log_level;
}

void media_context_set_log_callback(MediaContext *ctx, 
                                     MediaLogCallback callback, 
                                     void *user_data)
{
    if (!ctx) {
        if (g_context) {
            g_context->log_callback = callback;
            g_context->log_user_data = user_data;
            
            if (callback) {
                av_log_set_callback(ffmpeg_log_callback);
            } else {
                av_log_set_callback(av_log_default_callback);
            }
        }
        return;
    }
    
    ctx->log_callback = callback;
    ctx->log_user_data = user_data;
}

/* ============================================================================
 * 编解码器查询
 * ============================================================================ */

bool media_context_codec_supported(MediaContext *ctx, MediaCodecID codec_id)
{
    const AVCodec *encoder = NULL;
    const AVCodec *decoder = NULL;
    
    /* 查找编码器 */
    const char *codec_name = media_codec_name(codec_id);
    if (codec_name) {
        encoder = avcodec_find_encoder_by_name(codec_name);
        if (!encoder) {
            encoder = avcodec_find_encoder((enum AVCodecID)codec_id);
        }
        
        decoder = avcodec_find_decoder_by_name(codec_name);
        if (!decoder) {
            decoder = avcodec_find_decoder((enum AVCodecID)codec_id);
        }
    }
    
    return (encoder != NULL || decoder != NULL);
}

bool media_context_encoder_available(MediaContext *ctx, MediaCodecID codec_id)
{
    const AVCodec *codec = avcodec_find_encoder((enum AVCodecID)codec_id);
    if (!codec) {
        const char *name = media_codec_name(codec_id);
        if (name) {
            codec = avcodec_find_encoder_by_name(name);
        }
    }
    return codec != NULL;
}

bool media_context_decoder_available(MediaContext *ctx, MediaCodecID codec_id)
{
    const AVCodec *codec = avcodec_find_decoder((enum AVCodecID)codec_id);
    return codec != NULL;
}

int32_t media_context_get_supported_codecs(MediaContext *ctx, 
                                            MediaCodecInfo *codecs, 
                                            int32_t max_count)
{
    if (!codecs || max_count <= 0) {
        return 0;
    }
    
    int count = 0;
    const AVCodec *codec = NULL;
    void *iter = NULL;
    
    while ((codec = av_codec_iterate(&iter)) != NULL && count < max_count) {
        if (av_codec_is_encoder(codec)) {
            codecs[count].codec_id = (MediaCodecID)codec->id;
            strncpy(codecs[count].name, codec->name, sizeof(codecs[count].name) - 1);
            strncpy(codecs[count].long_name, codec->long_name, 
                    sizeof(codecs[count].long_name) - 1);
            codecs[count].type = (codec->type == AVMEDIA_TYPE_VIDEO) ? 
                                  MEDIA_STREAM_TYPE_VIDEO : MEDIA_STREAM_TYPE_AUDIO;
            codecs[count].is_encoder = true;
            count++;
        }
    }
    
    return count;
}

/* ============================================================================
 * 滤镜查询
 * ============================================================================ */

bool media_context_filter_available(MediaContext *ctx, const char *filter_name)
{
    const AVFilter *filter = avfilter_get_by_name(filter_name);
    return filter != NULL;
}

int32_t media_context_get_supported_filters(MediaContext *ctx, 
                                             char **filters, 
                                             int32_t max_count)
{
    if (!filters || max_count <= 0) {
        return 0;
    }
    
    int count = 0;
    const AVFilter *filter = NULL;
    void *iter = NULL;
    
    while ((filter = av_filter_iterate(&iter)) != NULL && count < max_count) {
        if (filters[count]) {
            strncpy(filters[count], filter->name, 255);
        }
        count++;
    }
    
    return count;
}

/* ============================================================================
 * 格式查询
 * ============================================================================ */

bool media_context_container_supported(MediaContext *ctx, 
                                        MediaContainerFormat container)
{
    const AVOutputFormat *fmt = NULL;
    const char *name = media_container_name(container);
    
    if (name) {
        fmt = av_guess_format(name, NULL, NULL);
    }
    
    return fmt != NULL;
}

bool media_context_protocol_supported(MediaContext *ctx, MediaProtocol protocol)
{
    const char *name = media_protocol_name(protocol);
    if (!name) return false;
    
    return media_context_protocol_supported_by_name(ctx, name);
}

bool media_context_protocol_supported_by_name(MediaContext *ctx, 
                                               const char *protocol_name)
{
    if (!protocol_name) return false;
    
    /* 检查URL协议是否支持 */
    void *opaque = NULL;
    const char *name;
    
    while ((name = avio_enum_protocols(&opaque, 0)) != NULL) {
        if (strcmp(name, protocol_name) == 0) {
            return true;
        }
    }
    
    return false;
}

/* ============================================================================
 * 统计信息
 * ============================================================================ */

int64_t media_context_get_total_frames_processed(MediaContext *ctx)
{
    if (!ctx) {
        return g_context ? g_context->total_frames_processed : 0;
    }
    return ctx->total_frames_processed;
}

int64_t media_context_get_total_bytes_processed(MediaContext *ctx)
{
    if (!ctx) {
        return g_context ? g_context->total_bytes_processed : 0;
    }
    return ctx->total_bytes_processed;
}

void media_context_reset_statistics(MediaContext *ctx)
{
    if (!ctx) {
        if (g_context) {
            pthread_mutex_lock(&g_context->mutex);
            g_context->total_frames_processed = 0;
            g_context->total_bytes_processed = 0;
            pthread_mutex_unlock(&g_context->mutex);
        }
        return;
    }
    
    pthread_mutex_lock(&ctx->mutex);
    ctx->total_frames_processed = 0;
    ctx->total_bytes_processed = 0;
    pthread_mutex_unlock(&ctx->mutex);
}

/* ============================================================================
 * 版本信息
 * ============================================================================ */

const char* media_context_get_version(void)
{
    return MEDIA_FRAMEWORK_VERSION_STRING;
}

int media_context_get_version_major(void)
{
    return MEDIA_FRAMEWORK_VERSION_MAJOR;
}

int media_context_get_version_minor(void)
{
    return MEDIA_FRAMEWORK_VERSION_MINOR;
}

int media_context_get_version_patch(void)
{
    return MEDIA_FRAMEWORK_VERSION_PATCH;
}

const char* media_context_get_ffmpeg_version(void)
{
    return av_version_info();
}

const char* media_context_get_ffmpeg_configuration(void)
{
    return avcodec_configuration();
}

const char* media_context_get_ffmpeg_license(void)
{
    return avcodec_license();
}
