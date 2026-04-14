/**
 * @file media_packet.c
 * @brief 媒体包管理实现
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 */

#include "core/media_packet.h"
#include "utils/media_utils.h"
#include <stdlib.h>
#include <string.h>

/* FFmpeg头文件 */
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/mem.h>
}

/* 内部包结构体 */
struct MediaPacket {
    AVPacket *av_packet;
    int32_t ref_count;
};

/* ============================================================================
 * 包创建与销毁
 * ============================================================================ */

MediaPacket* media_packet_create(void)
{
    MediaPacket *packet = (MediaPacket*)calloc(1, sizeof(MediaPacket));
    if (!packet) {
        return NULL;
    }
    
    packet->av_packet = av_packet_alloc();
    if (!packet->av_packet) {
        free(packet);
        return NULL;
    }
    
    packet->ref_count = 1;
    return packet;
}

MediaPacket* media_packet_create_with_size(int32_t size)
{
    MediaPacket *packet = media_packet_create();
    if (!packet) {
        return NULL;
    }
    
    int ret = av_new_packet(packet->av_packet, size);
    if (ret < 0) {
        av_packet_free(&packet->av_packet);
        free(packet);
        return NULL;
    }
    
    return packet;
}

MediaPacket* media_packet_create_with_data(const uint8_t *data, int32_t size)
{
    if (!data || size <= 0) {
        return NULL;
    }
    
    MediaPacket *packet = media_packet_create();
    if (!packet) {
        return NULL;
    }
    
    int ret = av_new_packet(packet->av_packet, size);
    if (ret < 0) {
        av_packet_free(&packet->av_packet);
        free(packet);
        return NULL;
    }
    
    memcpy(packet->av_packet->data, data, size);
    return packet;
}

MediaPacket* media_packet_ref(MediaPacket *packet)
{
    if (!packet) {
        return NULL;
    }
    
    MediaPacket *ref = (MediaPacket*)calloc(1, sizeof(MediaPacket));
    if (!ref) {
        return NULL;
    }
    
    ref->av_packet = av_packet_alloc();
    if (!ref->av_packet) {
        free(ref);
        return NULL;
    }
    
    int ret = av_packet_ref(ref->av_packet, packet->av_packet);
    if (ret < 0) {
        av_packet_free(&ref->av_packet);
        free(ref);
        return NULL;
    }
    
    ref->ref_count = 1;
    return ref;
}

void media_packet_unref(MediaPacket *packet)
{
    if (!packet) return;
    
    packet->ref_count--;
    if (packet->ref_count <= 0) {
        if (packet->av_packet) {
            av_packet_unref(packet->av_packet);
        }
    }
}

void media_packet_free(MediaPacket **packet)
{
    if (!packet || !*packet) return;
    
    MediaPacket *p = *packet;
    
    if (p->av_packet) {
        av_packet_free(&p->av_packet);
    }
    
    free(p);
    *packet = NULL;
}

MediaErrorCode media_packet_copy_props(MediaPacket *dst, const MediaPacket *src)
{
    if (!dst || !src) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    av_packet_copy_props(dst->av_packet, src->av_packet);
    return MEDIA_SUCCESS;
}

MediaPacket* media_packet_clone(const MediaPacket *packet)
{
    if (!packet) {
        return NULL;
    }
    
    MediaPacket *clone = media_packet_create();
    if (!clone) {
        return NULL;
    }
    
    int ret = av_packet_ref(clone->av_packet, packet->av_packet);
    if (ret < 0) {
        media_packet_free(&clone);
        return NULL;
    }
    
    return clone;
}

/* ============================================================================
 * 包属性设置
 * ============================================================================ */

MediaErrorCode media_packet_set_pts(MediaPacket *packet, int64_t pts)
{
    if (!packet) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    packet->av_packet->pts = pts;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_packet_set_dts(MediaPacket *packet, int64_t dts)
{
    if (!packet) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    packet->av_packet->dts = dts;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_packet_set_duration(MediaPacket *packet, int64_t duration)
{
    if (!packet) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    packet->av_packet->duration = duration;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_packet_set_stream_index(MediaPacket *packet, int32_t index)
{
    if (!packet) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    packet->av_packet->stream_index = index;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_packet_set_key_frame(MediaPacket *packet, bool is_key)
{
    if (!packet) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (is_key) {
        packet->av_packet->flags |= AV_PKT_FLAG_KEY;
    } else {
        packet->av_packet->flags &= ~AV_PKT_FLAG_KEY;
    }
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_packet_set_pos(MediaPacket *packet, int64_t pos)
{
    if (!packet) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    packet->av_packet->pos = pos;
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 包属性获取
 * ============================================================================ */

int64_t media_packet_get_pts(const MediaPacket *packet)
{
    if (!packet) return AV_NOPTS_VALUE;
    return packet->av_packet->pts;
}

int64_t media_packet_get_dts(const MediaPacket *packet)
{
    if (!packet) return AV_NOPTS_VALUE;
    return packet->av_packet->dts;
}

int64_t media_packet_get_duration(const MediaPacket *packet)
{
    if (!packet) return 0;
    return packet->av_packet->duration;
}

int32_t media_packet_get_stream_index(const MediaPacket *packet)
{
    if (!packet) return -1;
    return packet->av_packet->stream_index;
}

bool media_packet_is_key_frame(const MediaPacket *packet)
{
    if (!packet) return false;
    return (packet->av_packet->flags & AV_PKT_FLAG_KEY) != 0;
}

int64_t media_packet_get_pos(const MediaPacket *packet)
{
    if (!packet) return -1;
    return packet->av_packet->pos;
}

double media_packet_get_timestamp_seconds(const MediaPacket *packet, 
                                           MediaRational time_base)
{
    if (!packet || time_base.den == 0) return 0.0;
    
    int64_t ts = packet->av_packet->pts;
    if (ts == AV_NOPTS_VALUE) {
        ts = packet->av_packet->dts;
    }
    if (ts == AV_NOPTS_VALUE) {
        return 0.0;
    }
    
    return (double)ts * time_base.num / time_base.den;
}

/* ============================================================================
 * 包数据访问
 * ============================================================================ */

uint8_t* media_packet_get_data(MediaPacket *packet)
{
    if (!packet) return NULL;
    return packet->av_packet->data;
}

const uint8_t* media_packet_get_data_const(const MediaPacket *packet)
{
    if (!packet) return NULL;
    return packet->av_packet->data;
}

int32_t media_packet_get_size(const MediaPacket *packet)
{
    if (!packet) return 0;
    return packet->av_packet->size;
}

MediaErrorCode media_packet_resize(MediaPacket *packet, int32_t new_size)
{
    if (!packet) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    int ret = av_grow_packet(packet->av_packet, new_size - packet->av_packet->size);
    if (ret < 0) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 包调试
 * ============================================================================ */

void media_packet_dump(const MediaPacket *packet, int32_t log_level)
{
    if (!packet) {
        MEDIA_LOG_INFO("Packet: NULL\n");
        return;
    }
    
    MEDIA_LOG_INFO("Packet: pts=%lld, dts=%lld, size=%d, stream=%d, flags=%d\n",
                   (long long)packet->av_packet->pts,
                   (long long)packet->av_packet->dts,
                   packet->av_packet->size,
                   packet->av_packet->stream_index,
                   packet->av_packet->flags);
}
