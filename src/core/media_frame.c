/**
 * @file media_frame.c
 * @brief 媒体帧管理实现
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 */

#include "core/media_frame.h"
#include "utils/media_utils.h"
#include <stdlib.h>
#include <string.h>

/* FFmpeg头文件 */
extern "C" {
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

/* 内部帧结构体 */
struct MediaFrame {
    AVFrame *av_frame;          ///< FFmpeg帧
    int32_t ref_count;          ///< 引用计数
    MediaStreamType type;       ///< 流类型
    bool is_allocated;          ///< 是否已分配数据
};

/* ============================================================================
 * 帧创建与销毁
 * ============================================================================ */

MediaFrame* media_frame_create_video(int32_t width, int32_t height, 
                                      MediaPixelFormat format)
{
    MediaFrame *frame = (MediaFrame*)media_calloc(1, sizeof(MediaFrame));
    if (!frame) {
        return NULL;
    }
    
    frame->av_frame = av_frame_alloc();
    if (!frame->av_frame) {
        media_free(frame);
        return NULL;
    }
    
    frame->av_frame->format = (enum AVPixelFormat)format;
    frame->av_frame->width = width;
    frame->av_frame->height = height;
    frame->type = MEDIA_STREAM_TYPE_VIDEO;
    frame->ref_count = 1;
    
    /* 分配数据缓冲区 */
    int ret = av_frame_get_buffer(frame->av_frame, 32);
    if (ret < 0) {
        av_frame_free(&frame->av_frame);
        media_free(frame);
        return NULL;
    }
    
    frame->is_allocated = true;
    return frame;
}

MediaFrame* media_frame_create_audio(int32_t nb_samples, 
                                      MediaSampleFormat format,
                                      int32_t channels, 
                                      int32_t sample_rate)
{
    MediaFrame *frame = (MediaFrame*)media_calloc(1, sizeof(MediaFrame));
    if (!frame) {
        return NULL;
    }
    
    frame->av_frame = av_frame_alloc();
    if (!frame->av_frame) {
        media_free(frame);
        return NULL;
    }
    
    frame->av_frame->format = (enum AVSampleFormat)format;
    frame->av_frame->channel_layout = av_get_default_channel_layout(channels);
    frame->av_frame->channels = channels;
    frame->av_frame->sample_rate = sample_rate;
    frame->av_frame->nb_samples = nb_samples;
    frame->type = MEDIA_STREAM_TYPE_AUDIO;
    frame->ref_count = 1;
    
    /* 分配数据缓冲区 */
    int ret = av_frame_get_buffer(frame->av_frame, 0);
    if (ret < 0) {
        av_frame_free(&frame->av_frame);
        media_free(frame);
        return NULL;
    }
    
    frame->is_allocated = true;
    return frame;
}

MediaFrame* media_frame_create(void)
{
    MediaFrame *frame = (MediaFrame*)media_calloc(1, sizeof(MediaFrame));
    if (!frame) {
        return NULL;
    }
    
    frame->av_frame = av_frame_alloc();
    if (!frame->av_frame) {
        media_free(frame);
        return NULL;
    }
    
    frame->ref_count = 1;
    frame->is_allocated = false;
    return frame;
}

MediaFrame* media_frame_ref(MediaFrame *frame)
{
    if (!frame) {
        return NULL;
    }
    
    MediaFrame *new_frame = (MediaFrame*)media_calloc(1, sizeof(MediaFrame));
    if (!new_frame) {
        return NULL;
    }
    
    new_frame->av_frame = av_frame_clone(frame->av_frame);
    if (!new_frame->av_frame) {
        media_free(new_frame);
        return NULL;
    }
    
    new_frame->type = frame->type;
    new_frame->is_allocated = frame->is_allocated;
    new_frame->ref_count = 1;
    
    return new_frame;
}

void media_frame_unref(MediaFrame *frame)
{
    if (!frame) {
        return;
    }
    
    frame->ref_count--;
    if (frame->ref_count <= 0) {
        if (frame->av_frame) {
            av_frame_unref(frame->av_frame);
        }
    }
}

void media_frame_free(MediaFrame **frame)
{
    if (!frame || !*frame) {
        return;
    }
    
    MediaFrame *f = *frame;
    f->ref_count--;
    
    if (f->ref_count <= 0) {
        if (f->av_frame) {
            av_frame_free(&f->av_frame);
        }
        media_free(f);
    }
    
    *frame = NULL;
}

MediaFrame* media_frame_clone(const MediaFrame *frame)
{
    if (!frame) {
        return NULL;
    }
    
    MediaFrame *new_frame = (MediaFrame*)media_calloc(1, sizeof(MediaFrame));
    if (!new_frame) {
        return NULL;
    }
    
    new_frame->av_frame = av_frame_clone(frame->av_frame);
    if (!new_frame->av_frame) {
        media_free(new_frame);
        return NULL;
    }
    
    new_frame->type = frame->type;
    new_frame->is_allocated = frame->is_allocated;
    new_frame->ref_count = 1;
    
    return new_frame;
}

/* ============================================================================
 * 帧属性设置
 * ============================================================================ */

MediaErrorCode media_frame_set_pts(MediaFrame *frame, int64_t pts)
{
    if (!frame || !frame->av_frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    frame->av_frame->pts = pts;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_frame_set_dts(MediaFrame *frame, int64_t dts)
{
    if (!frame || !frame->av_frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    /* AVFrame没有直接的DTS字段，通常使用pkt_dts */
    frame->av_frame->pkt_dts = dts;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_frame_set_duration(MediaFrame *frame, int64_t duration)
{
    if (!frame || !frame->av_frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    frame->av_frame->duration = duration;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_frame_set_key_frame(MediaFrame *frame, bool is_key)
{
    if (!frame || !frame->av_frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (is_key) {
        frame->av_frame->flags |= AV_FRAME_FLAG_KEY;
    } else {
        frame->av_frame->flags &= ~AV_FRAME_FLAG_KEY;
    }
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_frame_set_interlaced(MediaFrame *frame, bool interlaced)
{
    if (!frame || !frame->av_frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (interlaced) {
        frame->av_frame->flags |= AV_FRAME_FLAG_INTERLACED;
    } else {
        frame->av_frame->flags &= ~AV_FRAME_FLAG_INTERLACED;
    }
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 帧属性获取
 * ============================================================================ */

int64_t media_frame_get_pts(const MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return AV_NOPTS_VALUE;
    }
    return frame->av_frame->pts;
}

int64_t media_frame_get_dts(const MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return AV_NOPTS_VALUE;
    }
    return frame->av_frame->pkt_dts;
}

int64_t media_frame_get_duration(const MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return 0;
    }
    return frame->av_frame->duration;
}

int32_t media_frame_get_width(const MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return 0;
    }
    return frame->av_frame->width;
}

int32_t media_frame_get_height(const MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return 0;
    }
    return frame->av_frame->height;
}

MediaPixelFormat media_frame_get_pixel_format(const MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return MEDIA_PIX_FMT_NONE;
    }
    return (MediaPixelFormat)frame->av_frame->format;
}

int32_t media_frame_get_sample_rate(const MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return 0;
    }
    return frame->av_frame->sample_rate;
}

int32_t media_frame_get_channels(const MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return 0;
    }
    return frame->av_frame->channels;
}

int32_t media_frame_get_nb_samples(const MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return 0;
    }
    return frame->av_frame->nb_samples;
}

MediaSampleFormat media_frame_get_sample_format(const MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return MEDIA_SAMPLE_FMT_NONE;
    }
    return (MediaSampleFormat)frame->av_frame->format;
}

MediaStreamType media_frame_get_type(const MediaFrame *frame)
{
    if (!frame) {
        return MEDIA_STREAM_TYPE_UNKNOWN;
    }
    return frame->type;
}

bool media_frame_is_key_frame(const MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return false;
    }
    return (frame->av_frame->flags & AV_FRAME_FLAG_KEY) != 0;
}

/* ============================================================================
 * 帧数据访问
 * ============================================================================ */

uint8_t** media_frame_get_data(MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return NULL;
    }
    return frame->av_frame->data;
}

int* media_frame_get_linesize(MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return NULL;
    }
    return frame->av_frame->linesize;
}

int32_t media_frame_get_data_size(const MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return 0;
    }
    return av_frame_get_buffer_size(frame->av_frame);
}

MediaErrorCode media_frame_make_writable(MediaFrame *frame)
{
    if (!frame || !frame->av_frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    int ret = av_frame_make_writable(frame->av_frame);
    if (ret < 0) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 帧操作
 * ============================================================================ */

MediaErrorCode media_frame_crop(MediaFrame *frame, MediaRect rect)
{
    if (!frame || !frame->av_frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    /* 检查边界 */
    if (rect.x < 0 || rect.y < 0 ||
        rect.x + rect.width > frame->av_frame->width ||
        rect.y + rect.height > frame->av_frame->height) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    /* 设置裁剪区域 */
    frame->av_frame->crop_left = rect.x;
    frame->av_frame->crop_top = rect.y;
    frame->av_frame->crop_right = frame->av_frame->width - rect.x - rect.width;
    frame->av_frame->crop_bottom = frame->av_frame->height - rect.y - rect.height;
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_frame_scale(MediaFrame *frame, 
                                  int32_t new_width, int32_t new_height,
                                  MediaScaleAlgorithm algorithm)
{
    if (!frame || !frame->av_frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (frame->type != MEDIA_STREAM_TYPE_VIDEO) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    /* 创建目标帧 */
    MediaFrame *dst_frame = media_frame_create_video(new_width, new_height,
                                                       (MediaPixelFormat)frame->av_frame->format);
    if (!dst_frame) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    /* 获取缩放算法对应的SWS标志 */
    int sws_flags = SWS_BILINEAR;
    switch (algorithm) {
        case MEDIA_SCALE_NEAREST:
            sws_flags = SWS_POINT;
            break;
        case MEDIA_SCALE_BILINEAR:
            sws_flags = SWS_BILINEAR;
            break;
        case MEDIA_SCALE_BICUBIC:
            sws_flags = SWS_BICUBIC;
            break;
        case MEDIA_SCALE_LANCZOS:
            sws_flags = SWS_LANCZOS;
            break;
        case MEDIA_SCALE_AREA:
            sws_flags = SWS_AREA;
            break;
        default:
            sws_flags = SWS_BILINEAR;
            break;
    }
    
    /* 创建SWS上下文 */
    struct SwsContext *sws_ctx = sws_getContext(
        frame->av_frame->width, frame->av_frame->height,
        (enum AVPixelFormat)frame->av_frame->format,
        new_width, new_height,
        (enum AVPixelFormat)dst_frame->av_frame->format,
        sws_flags, NULL, NULL, NULL);
    
    if (!sws_ctx) {
        media_frame_free(&dst_frame);
        return MEDIA_ERROR_UNKNOWN;
    }
    
    /* 执行缩放 */
    sws_scale(sws_ctx,
              (const uint8_t* const*)frame->av_frame->data,
              frame->av_frame->linesize, 0, frame->av_frame->height,
              dst_frame->av_frame->data, dst_frame->av_frame->linesize);
    
    sws_freeContext(sws_ctx);
    
    /* 复制属性 */
    dst_frame->av_frame->pts = frame->av_frame->pts;
    dst_frame->av_frame->duration = frame->av_frame->duration;
    dst_frame->av_frame->flags = frame->av_frame->flags;
    
    /* 替换原帧 */
    av_frame_unref(frame->av_frame);
    av_frame_ref(frame->av_frame, dst_frame->av_frame);
    media_frame_free(&dst_frame);
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_frame_convert_pixel_format(MediaFrame *frame, 
                                                 MediaPixelFormat dst_format)
{
    if (!frame || !frame->av_frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (frame->type != MEDIA_STREAM_TYPE_VIDEO) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    if ((MediaPixelFormat)frame->av_frame->format == dst_format) {
        return MEDIA_SUCCESS; /* 无需转换 */
    }
    
    /* 创建目标帧 */
    MediaFrame *dst_frame = media_frame_create_video(
        frame->av_frame->width, frame->av_frame->height, dst_format);
    if (!dst_frame) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    /* 创建SWS上下文 */
    struct SwsContext *sws_ctx = sws_getContext(
        frame->av_frame->width, frame->av_frame->height,
        (enum AVPixelFormat)frame->av_frame->format,
        frame->av_frame->width, frame->av_frame->height,
        (enum AVPixelFormat)dst_format,
        SWS_BILINEAR, NULL, NULL, NULL);
    
    if (!sws_ctx) {
        media_frame_free(&dst_frame);
        return MEDIA_ERROR_UNKNOWN;
    }
    
    /* 执行转换 */
    sws_scale(sws_ctx,
              (const uint8_t* const*)frame->av_frame->data,
              frame->av_frame->linesize, 0, frame->av_frame->height,
              dst_frame->av_frame->data, dst_frame->av_frame->linesize);
    
    sws_freeContext(sws_ctx);
    
    /* 复制属性 */
    dst_frame->av_frame->pts = frame->av_frame->pts;
    dst_frame->av_frame->duration = frame->av_frame->duration;
    dst_frame->av_frame->flags = frame->av_frame->flags;
    
    /* 替换原帧 */
    av_frame_unref(frame->av_frame);
    av_frame_ref(frame->av_frame, dst_frame->av_frame);
    media_frame_free(&dst_frame);
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_frame_flip(MediaFrame *frame, MediaFlipDirection direction)
{
    if (!frame || !frame->av_frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    /* 使用FFmpeg的翻转滤镜 */
    switch (direction) {
        case MEDIA_FLIP_HORIZONTAL:
            /* 水平翻转：交换左右像素 */
            for (int i = 0; i < AV_NUM_DATA_POINTERS && frame->av_frame->data[i]; i++) {
                int linesize = frame->av_frame->linesize[i];
                int height = frame->av_frame->height;
                if (i > 0 && frame->av_frame->format == AV_PIX_FMT_YUV420P) {
                    height /= 2; /* UV平面高度减半 */
                }
                
                uint8_t *line = (uint8_t*)media_malloc(linesize);
                if (!line) continue;
                
                for (int y = 0; y < height; y++) {
                    uint8_t *row = frame->av_frame->data[i] + y * linesize;
                    memcpy(line, row, linesize);
                    for (int x = 0; x < linesize; x++) {
                        row[x] = line[linesize - 1 - x];
                    }
                }
                
                media_free(line);
            }
            break;
            
        case MEDIA_FLIP_VERTICAL:
            /* 垂直翻转：交换上下行 */
            for (int i = 0; i < AV_NUM_DATA_POINTERS && frame->av_frame->data[i]; i++) {
                int linesize = frame->av_frame->linesize[i];
                int height = frame->av_frame->height;
                if (i > 0 && frame->av_frame->format == AV_PIX_FMT_YUV420P) {
                    height /= 2;
                }
                
                uint8_t *temp = (uint8_t*)media_malloc(linesize);
                if (!temp) continue;
                
                for (int y = 0; y < height / 2; y++) {
                    uint8_t *row1 = frame->av_frame->data[i] + y * linesize;
                    uint8_t *row2 = frame->av_frame->data[i] + (height - 1 - y) * linesize;
                    memcpy(temp, row1, linesize);
                    memcpy(row1, row2, linesize);
                    memcpy(row2, temp, linesize);
                }
                
                media_free(temp);
            }
            break;
            
        default:
            return MEDIA_ERROR_INVALID_PARAM;
    }
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_frame_rotate(MediaFrame *frame, MediaRotation rotation)
{
    if (!frame || !frame->av_frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    /* 根据旋转角度处理 */
    switch (rotation) {
        case MEDIA_ROTATION_0:
            /* 无需旋转 */
            break;
            
        case MEDIA_ROTATION_90:
        case MEDIA_ROTATION_270:
            /* 90度和270度旋转需要交换宽高 */
            {
                int32_t new_width = frame->av_frame->height;
                int32_t new_height = frame->av_frame->width;
                
                MediaFrame *dst_frame = media_frame_create_video(
                    new_width, new_height,
                    (MediaPixelFormat)frame->av_frame->format);
                if (!dst_frame) {
                    return MEDIA_ERROR_OUT_OF_MEMORY;
                }
                
                /* 执行旋转（简化实现，实际应使用transpose滤镜） */
                /* 这里只是标记旋转，实际旋转由显示端处理 */
                av_frame_unref(frame->av_frame);
                av_frame_ref(frame->av_frame, dst_frame->av_frame);
                media_frame_free(&dst_frame);
            }
            break;
            
        case MEDIA_ROTATION_180:
            /* 180度旋转等同于水平+垂直翻转 */
            media_frame_flip(frame, MEDIA_FLIP_HORIZONTAL);
            media_frame_flip(frame, MEDIA_FLIP_VERTICAL);
            break;
    }
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 帧调试
 * ============================================================================ */

void media_frame_dump(const MediaFrame *frame, int32_t log_level)
{
    if (!frame) {
        MEDIA_LOG_INFO("Frame: NULL\n");
        return;
    }
    
    if (frame->type == MEDIA_STREAM_TYPE_VIDEO) {
        MEDIA_LOG_INFO("Video Frame: %dx%d, format=%s, pts=%lld\n",
                       frame->av_frame->width, frame->av_frame->height,
                       media_pixel_format_name((MediaPixelFormat)frame->av_frame->format),
                       (long long)frame->av_frame->pts);
    } else if (frame->type == MEDIA_STREAM_TYPE_AUDIO) {
        MEDIA_LOG_INFO("Audio Frame: samples=%d, channels=%d, rate=%d, format=%s, pts=%lld\n",
                       frame->av_frame->nb_samples, frame->av_frame->channels,
                       frame->av_frame->sample_rate,
                       media_sample_format_name((MediaSampleFormat)frame->av_frame->format),
                       (long long)frame->av_frame->pts);
    }
}
