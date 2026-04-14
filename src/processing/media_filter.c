/**
 * @file media_filter.c
 * @brief 媒体滤镜实现
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 */

#include "processing/media_filter.h"
#include "utils/media_utils.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* FFmpeg头文件 */
extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>
}

/* 内部滤镜上下文结构体 */
struct MediaFilterContext {
    AVFilterGraph *graph;
    AVFilterContext *buffersrc_ctx;
    AVFilterContext *buffersink_ctx;
    MediaFilterType type;
    char filter_name[64];
    int initialized;
};

/* ============================================================================
 * 参数默认值
 * ============================================================================ */

void media_filter_color_adjust_params_default(ColorAdjustParams *params)
{
    if (!params) return;
    memset(params, 0, sizeof(ColorAdjustParams));
    params->brightness = 0.0;
    params->contrast = 1.0;
    params->saturation = 1.0;
    params->hue = 0.0;
    params->gamma = 1.0;
    params->invert = 0;
}

void media_filter_geometric_params_default(GeometricParams *params)
{
    if (!params) return;
    memset(params, 0, sizeof(GeometricParams));
    params->rotation = MEDIA_ROTATION_NONE;
    params->flip = MEDIA_FLIP_NONE;
    params->angle = 0.0;
    params->scale_x = 1.0;
    params->scale_y = 1.0;
}

void media_filter_blur_params_default(BlurParams *params)
{
    if (!params) return;
    memset(params, 0, sizeof(BlurParams));
    params->type = MEDIA_BLUR_GAUSSIAN;
    params->radius = 5.0;
    params->sigma = 1.0;
}

void media_filter_sharpen_params_default(SharpenParams *params)
{
    if (!params) return;
    memset(params, 0, sizeof(SharpenParams));
    params->amount = 1.0;
    params->radius = 1.0;
    params->threshold = 0;
}

void media_filter_equalizer_params_default(EqualizerParams *params)
{
    if (!params) return;
    memset(params, 0, sizeof(EqualizerParams));
    params->bands_count = 10;
    for (int i = 0; i < 10; i++) {
        params->gains[i] = 0.0;
    }
}

void media_filter_reverb_params_default(ReverbParams *params)
{
    if (!params) return;
    memset(params, 0, sizeof(ReverbParams));
    params->room_size = 0.5;
    params->damping = 0.5;
    params->wet_level = 0.3;
    params->dry_level = 0.7;
    params->width = 1.0;
}

void media_filter_denoise_params_default(DenoiseParams *params)
{
    if (!params) return;
    memset(params, 0, sizeof(DenoiseParams));
    params->strength = 0.5;
    params->profile = MEDIA_DENOISE_PROFILE_MEDIUM;
}

void media_filter_time_stretch_params_default(TimeStretchParams *params)
{
    if (!params) return;
    memset(params, 0, sizeof(TimeStretchParams));
    params->tempo = 1.0;
    params->pitch = 1.0;
    params->rate = 1.0;
}

/* ============================================================================
 * 滤镜上下文创建与销毁
 * ============================================================================ */

MediaFilterContext* media_filter_create(MediaFilterType type)
{
    MediaFilterContext *ctx = (MediaFilterContext*)calloc(1, sizeof(MediaFilterContext));
    if (!ctx) {
        return NULL;
    }
    
    ctx->graph = avfilter_graph_alloc();
    if (!ctx->graph) {
        free(ctx);
        return NULL;
    }
    
    ctx->type = type;
    ctx->initialized = 0;
    
    return ctx;
}

void media_filter_destroy(MediaFilterContext **ctx)
{
    if (!ctx || !*ctx) return;
    
    MediaFilterContext *f = *ctx;
    
    if (f->graph) {
        avfilter_graph_free(&f->graph);
    }
    
    free(f);
    *ctx = NULL;
}

/* ============================================================================
 * 滤镜初始化
 * ============================================================================ */

MediaErrorCode media_filter_init_video(MediaFilterContext *ctx,
                                        int width, int height,
                                        MediaPixelFormat format,
                                        MediaRational time_base,
                                        MediaRational frame_rate)
{
    if (!ctx) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    
    char args[512];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:frame_rate=%d/%d",
             width, height, (int)format,
             time_base.num, time_base.den,
             frame_rate.num, frame_rate.den);
    
    int ret = avfilter_graph_create_filter(&ctx->buffersrc_ctx, buffersrc,
                                            "in", args, NULL, ctx->graph);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Cannot create buffer source\n");
        return MEDIA_ERROR_FILTER_INIT_FAILED;
    }
    
    ret = avfilter_graph_create_filter(&ctx->buffersink_ctx, buffersink,
                                        "out", NULL, NULL, ctx->graph);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Cannot create buffer sink\n");
        return MEDIA_ERROR_FILTER_INIT_FAILED;
    }
    
    ctx->initialized = 1;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_filter_init_audio(MediaFilterContext *ctx,
                                        int sample_rate,
                                        MediaSampleFormat format,
                                        int channels,
                                        int64_t channel_layout)
{
    if (!ctx) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    const AVFilter *abuffersrc = avfilter_get_by_name("abuffer");
    const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    
    char args[512];
    snprintf(args, sizeof(args),
             "sample_rate=%d:sample_fmt=%s:channels=%d:channel_layout=0x%llx",
             sample_rate, av_get_sample_fmt_name((enum AVSampleFormat)format),
             channels, (long long)channel_layout);
    
    int ret = avfilter_graph_create_filter(&ctx->buffersrc_ctx, abuffersrc,
                                            "in", args, NULL, ctx->graph);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Cannot create audio buffer source\n");
        return MEDIA_ERROR_FILTER_INIT_FAILED;
    }
    
    ret = avfilter_graph_create_filter(&ctx->buffersink_ctx, abuffersink,
                                        "out", NULL, NULL, ctx->graph);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Cannot create audio buffer sink\n");
        return MEDIA_ERROR_FILTER_INIT_FAILED;
    }
    
    ctx->initialized = 1;
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 滤镜配置
 * ============================================================================ */

MediaErrorCode media_filter_set_graph(MediaFilterContext *ctx,
                                       const char *filter_desc)
{
    if (!ctx || !filter_desc) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (!ctx->initialized) {
        return MEDIA_ERROR_NOT_INITIALIZED;
    }
    
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    
    outputs->name = av_strdup("in");
    outputs->filter_ctx = ctx->buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;
    
    inputs->name = av_strdup("out");
    inputs->filter_ctx = ctx->buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;
    
    int ret = avfilter_graph_parse_ptr(ctx->graph, filter_desc,
                                        &inputs, &outputs, NULL);
    
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    
    if (ret < 0) {
        MEDIA_LOG_ERROR("Failed to parse filter graph: %s\n", filter_desc);
        return MEDIA_ERROR_FILTER_INIT_FAILED;
    }
    
    ret = avfilter_graph_config(ctx->graph, NULL);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Failed to configure filter graph\n");
        return MEDIA_ERROR_FILTER_INIT_FAILED;
    }
    
    strncpy(ctx->filter_name, filter_desc, sizeof(ctx->filter_name) - 1);
    MEDIA_LOG_INFO("Filter graph configured: %s\n", filter_desc);
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 滤镜处理
 * ============================================================================ */

MediaErrorCode media_filter_process(MediaFilterContext *ctx,
                                     MediaFrame *input,
                                     MediaFrame **output)
{
    if (!ctx || !input || !output) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (!ctx->initialized) {
        return MEDIA_ERROR_NOT_INITIALIZED;
    }
    
    /* 推送帧到滤镜图 */
    int ret = av_buffersrc_add_frame_flags(ctx->buffersrc_ctx,
                                            (AVFrame*)input,
                                            AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Error feeding to filter graph\n");
        return MEDIA_ERROR_FILTER_PROCESS_FAILED;
    }
    
    /* 从滤镜图拉取处理后的帧 */
    *output = media_frame_create();
    if (!*output) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    ret = av_buffersink_get_frame(ctx->buffersink_ctx, (AVFrame*)*output);
    if (ret < 0) {
        media_frame_free(output);
        if (ret == AVERROR(EAGAIN)) {
            return MEDIA_NEED_MORE_DATA;
        }
        return MEDIA_ERROR_FILTER_PROCESS_FAILED;
    }
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_filter_flush(MediaFilterContext *ctx)
{
    if (!ctx) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (!ctx->initialized) {
        return MEDIA_ERROR_NOT_INITIALIZED;
    }
    
    int ret = av_buffersrc_add_frame_flags(ctx->buffersrc_ctx, NULL, 0);
    return (ret < 0) ? MEDIA_ERROR_FILTER_PROCESS_FAILED : MEDIA_SUCCESS;
}

/* ============================================================================
 * 视频滤镜工厂函数
 * ============================================================================ */

MediaFilterContext* media_filter_color_adjust(const ColorAdjustParams *params)
{
    MediaFilterContext *ctx = media_filter_create(MEDIA_FILTER_TYPE_VIDEO);
    if (!ctx) return NULL;
    
    char filter_desc[512];
    snprintf(filter_desc, sizeof(filter_desc),
             "eq=brightness=%.2f:contrast=%.2f:saturation=%.2f:gamma=%.2f",
             params->brightness, params->contrast, 
             params->saturation, params->gamma);
    
    if (params->hue != 0.0) {
        char hue_str[64];
        snprintf(hue_str, sizeof(hue_str), ",hue=h=%.2f", params->hue);
        strncat(filter_desc, hue_str, sizeof(filter_desc) - strlen(filter_desc) - 1);
    }
    
    strncpy(ctx->filter_name, filter_desc, sizeof(ctx->filter_name) - 1);
    return ctx;
}

MediaFilterContext* media_filter_geometric(const GeometricParams *params)
{
    MediaFilterContext *ctx = media_filter_create(MEDIA_FILTER_TYPE_VIDEO);
    if (!ctx) return NULL;
    
    char filter_desc[512] = {0};
    
    /* 旋转 */
    if (params->rotation != MEDIA_ROTATION_NONE) {
        switch (params->rotation) {
            case MEDIA_ROTATION_90:
                strcpy(filter_desc, "transpose=1");
                break;
            case MEDIA_ROTATION_180:
                strcpy(filter_desc, "transpose=1,transpose=1");
                break;
            case MEDIA_ROTATION_270:
                strcpy(filter_desc, "transpose=2");
                break;
            default:
                break;
        }
    }
    
    /* 任意角度旋转 */
    if (params->angle != 0.0) {
        char rotate_str[64];
        snprintf(rotate_str, sizeof(rotate_str), "rotate=%.2f", params->angle * M_PI / 180.0);
        if (filter_desc[0]) strcat(filter_desc, ",");
        strcat(filter_desc, rotate_str);
    }
    
    /* 翻转 */
    if (params->flip != MEDIA_FLIP_NONE) {
        if (filter_desc[0]) strcat(filter_desc, ",");
        switch (params->flip) {
            case MEDIA_FLIP_HORIZONTAL:
                strcat(filter_desc, "hflip");
                break;
            case MEDIA_FLIP_VERTICAL:
                strcat(filter_desc, "vflip");
                break;
            case MEDIA_FLIP_BOTH:
                strcat(filter_desc, "hflip,vflip");
                break;
            default:
                break;
        }
    }
    
    /* 缩放 */
    if (params->scale_x != 1.0 || params->scale_y != 1.0) {
        if (filter_desc[0]) strcat(filter_desc, ",");
        char scale_str[64];
        snprintf(scale_str, sizeof(scale_str), "scale=iw*%.2f:ih*%.2f", 
                 params->scale_x, params->scale_y);
        strcat(filter_desc, scale_str);
    }
    
    strncpy(ctx->filter_name, filter_desc, sizeof(ctx->filter_name) - 1);
    return ctx;
}

MediaFilterContext* media_filter_blur(const BlurParams *params)
{
    MediaFilterContext *ctx = media_filter_create(MEDIA_FILTER_TYPE_VIDEO);
    if (!ctx) return NULL;
    
    char filter_desc[256];
    
    switch (params->type) {
        case MEDIA_BLUR_GAUSSIAN:
            snprintf(filter_desc, sizeof(filter_desc), 
                     "gblur=sigma=%.2f:radius=%d", 
                     params->sigma, (int)params->radius);
            break;
        case MEDIA_BLUR_BOX:
            snprintf(filter_desc, sizeof(filter_desc), 
                     "boxblur=luma_radius=%d:luma_power=2", (int)params->radius);
            break;
        case MEDIA_BLUR_MEDIAN:
            snprintf(filter_desc, sizeof(filter_desc), 
                     "median=radius=%d", (int)params->radius);
            break;
        default:
            snprintf(filter_desc, sizeof(filter_desc), 
                     "gblur=sigma=%.2f", params->sigma);
            break;
    }
    
    strncpy(ctx->filter_name, filter_desc, sizeof(ctx->filter_name) - 1);
    return ctx;
}

MediaFilterContext* media_filter_sharpen(const SharpenParams *params)
{
    MediaFilterContext *ctx = media_filter_create(MEDIA_FILTER_TYPE_VIDEO);
    if (!ctx) return NULL;
    
    char filter_desc[256];
    snprintf(filter_desc, sizeof(filter_desc),
             "unsharp=luma_amount=%.2f:luma_radius=%.1f",
             params->amount, params->radius);
    
    strncpy(ctx->filter_name, filter_desc, sizeof(ctx->filter_name) - 1);
    return ctx;
}

MediaFilterContext* media_filter_crop(int32_t x, int32_t y, 
                                       int32_t width, int32_t height)
{
    MediaFilterContext *ctx = media_filter_create(MEDIA_FILTER_TYPE_VIDEO);
    if (!ctx) return NULL;
    
    char filter_desc[256];
    snprintf(filter_desc, sizeof(filter_desc),
             "crop=%d:%d:%d:%d", width, height, x, y);
    
    strncpy(ctx->filter_name, filter_desc, sizeof(ctx->filter_name) - 1);
    return ctx;
}

MediaFilterContext* media_filter_scale(int32_t width, int32_t height,
                                        MediaScaleAlgorithm algorithm)
{
    MediaFilterContext *ctx = media_filter_create(MEDIA_FILTER_TYPE_VIDEO);
    if (!ctx) return NULL;
    
    const char *algo_str = "bilinear";
    switch (algorithm) {
        case MEDIA_SCALE_BICUBIC: algo_str = "bicubic"; break;
        case MEDIA_SCALE_NEAREST: algo_str = "neighbor"; break;
        case MEDIA_SCALE_LANCZOS: algo_str = "lanczos"; break;
        default: break;
    }
    
    char filter_desc[256];
    snprintf(filter_desc, sizeof(filter_desc),
             "scale=%d:%d:flags=%s", width, height, algo_str);
    
    strncpy(ctx->filter_name, filter_desc, sizeof(ctx->filter_name) - 1);
    return ctx;
}

/* ============================================================================
 * 音频滤镜工厂函数
 * ============================================================================ */

MediaFilterContext* media_filter_equalizer(const EqualizerParams *params)
{
    MediaFilterContext *ctx = media_filter_create(MEDIA_FILTER_TYPE_AUDIO);
    if (!ctx) return NULL;
    
    char filter_desc[1024] = {0};
    char *p = filter_desc;
    
    /* 创建多频段均衡器 */
    p += sprintf(p, "equalizer=");
    
    for (int i = 0; i < params->bands_count && i < 10; i++) {
        if (i > 0) p += sprintf(p, ":");
        p += sprintf(p, "f%d=%.0f:g%d=%.1f", i, params->frequencies[i], i, params->gains[i]);
    }
    
    strncpy(ctx->filter_name, filter_desc, sizeof(ctx->filter_name) - 1);
    return ctx;
}

MediaFilterContext* media_filter_reverb(const ReverbParams *params)
{
    MediaFilterContext *ctx = media_filter_create(MEDIA_FILTER_TYPE_AUDIO);
    if (!ctx) return NULL;
    
    char filter_desc[256];
    snprintf(filter_desc, sizeof(filter_desc),
             "aecho=0.8:0.9:%d:%d", 
             (int)(params->room_size * 1000), 
             (int)(params->damping * 100));
    
    strncpy(ctx->filter_name, filter_desc, sizeof(ctx->filter_name) - 1);
    return ctx;
}

MediaFilterContext* media_filter_denoise(const DenoiseParams *params)
{
    MediaFilterContext *ctx = media_filter_create(MEDIA_FILTER_TYPE_AUDIO);
    if (!ctx) return NULL;
    
    char filter_desc[256];
    snprintf(filter_desc, sizeof(filter_desc),
             "afftdn=nf=%.2f", params->strength);
    
    strncpy(ctx->filter_name, filter_desc, sizeof(ctx->filter_name) - 1);
    return ctx;
}

MediaFilterContext* media_filter_time_stretch(const TimeStretchParams *params)
{
    MediaFilterContext *ctx = media_filter_create(MEDIA_FILTER_TYPE_AUDIO);
    if (!ctx) return NULL;
    
    char filter_desc[256];
    snprintf(filter_desc, sizeof(filter_desc),
             "atempo=%.2f,asetrate=r=%d*%.2f",
             params->tempo, 44100, params->pitch);
    
    strncpy(ctx->filter_name, filter_desc, sizeof(ctx->filter_name) - 1);
    return ctx;
}

MediaFilterContext* media_filter_volume(double volume)
{
    MediaFilterContext *ctx = media_filter_create(MEDIA_FILTER_TYPE_AUDIO);
    if (!ctx) return NULL;
    
    char filter_desc[64];
    snprintf(filter_desc, sizeof(filter_desc), "volume=%.2f", volume);
    
    strncpy(ctx->filter_name, filter_desc, sizeof(ctx->filter_name) - 1);
    return ctx;
}

MediaFilterContext* media_filter_channel_mix(int32_t input_layout, 
                                              int32_t output_layout)
{
    MediaFilterContext *ctx = media_filter_create(MEDIA_FILTER_TYPE_AUDIO);
    if (!ctx) return NULL;
    
    char filter_desc[128];
    snprintf(filter_desc, sizeof(filter_desc),
             "aformat=channel_layouts=0x%x", output_layout);
    
    strncpy(ctx->filter_name, filter_desc, sizeof(ctx->filter_name) - 1);
    return ctx;
}

/* ============================================================================
 * 滤镜信息
 * ============================================================================ */

const char* media_filter_get_name(const MediaFilterContext *ctx)
{
    if (!ctx) return NULL;
    return ctx->filter_name;
}

MediaFilterType media_filter_get_type(const MediaFilterContext *ctx)
{
    if (!ctx) return MEDIA_FILTER_TYPE_UNKNOWN;
    return ctx->type;
}

bool media_filter_is_initialized(const MediaFilterContext *ctx)
{
    if (!ctx) return false;
    return ctx->initialized != 0;
}
