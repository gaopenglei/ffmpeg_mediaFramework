/**
 * @file media_reader.c
 * @brief 媒体读取器实现
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 */

#include "input/media_reader.h"
#include "utils/media_utils.h"
#include <stdlib.h>
#include <string.h>

/* FFmpeg头文件 */
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

/* 内部读取器结构体 */
struct MediaReader {
    AVFormatContext *fmt_ctx;       ///< 格式上下文
    AVCodecContext **codec_ctxs;    ///< 编解码器上下文数组
    AVFrame *frame;                 ///< 解码帧
    AVPacket *packet;               ///< 数据包
    
    char *filename;                 ///< 文件名
    MediaReaderState state;         ///< 状态
    MediaReaderConfig config;       ///< 配置
    
    int *stream_map;                ///< 流映射
    int nb_streams;                 ///< 流数量
    
    int64_t duration;               ///< 时长（微秒）
    int64_t start_time;             ///< 起始时间
    
    /* 视频信息 */
    int video_stream_idx;           ///< 视频流索引
    int video_width;                ///< 视频宽度
    int video_height;               ///< 视频高度
    MediaPixelFormat video_format;  ///< 视频像素格式
    MediaRational video_time_base;  ///< 视频时间基
    MediaRational video_frame_rate; ///< 视频帧率
    
    /* 音频信息 */
    int audio_stream_idx;           ///< 音频流索引
    int audio_sample_rate;          ///< 音频采样率
    int audio_channels;             ///< 音频声道数
    MediaSampleFormat audio_format; ///< 音频采样格式
    MediaRational audio_time_base;  ///< 音频时间基
    
    /* 回调 */
    MediaReadCallback read_callback;
    void *callback_user_data;
    
    /* 统计 */
    int64_t bytes_read;
    int64_t frames_decoded;
};

/* ============================================================================
 * 配置默认值
 * ============================================================================ */

void media_reader_config_default(MediaReaderConfig *config)
{
    if (!config) return;
    
    memset(config, 0, sizeof(MediaReaderConfig));
    config->buffer_size = 4 * 1024 * 1024;  /* 4MB */
    config->probe_size = 5 * 1024 * 1024;   /* 5MB */
    config->analyze_duration = 5 * 1000000; /* 5秒 */
    config->max_stream_count = 32;
    config->thread_count = 4;
    config->low_res = 0;
    config->fast_seek = 0;
    config->discard_corrupt = 1;
    config->gen_pts = 0;
    config->ignore_dts = 0;
}

/* ============================================================================
 * 创建与销毁
 * ============================================================================ */

MediaReader* media_reader_create(const char *filename)
{
    if (!filename) {
        return NULL;
    }
    
    MediaReader *reader = (MediaReader*)media_calloc(1, sizeof(MediaReader));
    if (!reader) {
        return NULL;
    }
    
    /* 设置默认配置 */
    media_reader_config_default(&reader->config);
    
    /* 复制文件名 */
    reader->filename = media_strdup(filename);
    if (!reader->filename) {
        media_free(reader);
        return NULL;
    }
    
    reader->state = MEDIA_READER_STATE_CLOSED;
    reader->video_stream_idx = -1;
    reader->audio_stream_idx = -1;
    
    return reader;
}

MediaReader* media_reader_create_with_config(const char *filename, 
                                              const MediaReaderConfig *config)
{
    MediaReader *reader = media_reader_create(filename);
    if (!reader) {
        return NULL;
    }
    
    if (config) {
        memcpy(&reader->config, config, sizeof(MediaReaderConfig));
    }
    
    return reader;
}

void media_reader_free(MediaReader *reader)
{
    if (!reader) {
        return;
    }
    
    media_reader_close(reader);
    
    if (reader->filename) {
        media_free(reader->filename);
    }
    
    media_free(reader);
}

/* ============================================================================
 * 打开与关闭
 * ============================================================================ */

static int open_codec_context(MediaReader *reader, int stream_index)
{
    AVFormatContext *fmt_ctx = reader->fmt_ctx;
    AVStream *st = fmt_ctx->streams[stream_index];
    const AVCodec *dec = NULL;
    AVCodecContext *dec_ctx = NULL;
    int ret;
    
    /* 查找解码器 */
    dec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!dec) {
        MEDIA_LOG_ERROR("Failed to find decoder for stream %d\n", stream_index);
        return MEDIA_ERROR_DECODER_NOT_FOUND;
    }
    
    /* 创建解码器上下文 */
    dec_ctx = avcodec_alloc_context3(dec);
    if (!dec_ctx) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    /* 复制流参数 */
    ret = avcodec_parameters_to_context(dec_ctx, st->codecpar);
    if (ret < 0) {
        avcodec_free_context(&dec_ctx);
        return MEDIA_ERROR_UNKNOWN;
    }
    
    /* 设置线程数 */
    if (reader->config.thread_count > 0) {
        dec_ctx->thread_count = reader->config.thread_count;
    }
    
    /* 打开解码器 */
    ret = avcodec_open2(dec_ctx, dec, NULL);
    if (ret < 0) {
        avcodec_free_context(&dec_ctx);
        return MEDIA_ERROR_DECODER_OPEN_FAILED;
    }
    
    /* 保存解码器上下文 */
    reader->codec_ctxs[stream_index] = dec_ctx;
    
    /* 更新流信息 */
    if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO && reader->video_stream_idx < 0) {
        reader->video_stream_idx = stream_index;
        reader->video_width = dec_ctx->width;
        reader->video_height = dec_ctx->height;
        reader->video_format = (MediaPixelFormat)dec_ctx->pix_fmt;
        reader->video_time_base.num = st->time_base.num;
        reader->video_time_base.den = st->time_base.den;
        reader->video_frame_rate = media_rational_make(st->r_frame_rate.num, st->r_frame_rate.den);
    } else if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO && reader->audio_stream_idx < 0) {
        reader->audio_stream_idx = stream_index;
        reader->audio_sample_rate = dec_ctx->sample_rate;
        reader->audio_channels = dec_ctx->channels;
        reader->audio_format = (MediaSampleFormat)dec_ctx->sample_fmt;
        reader->audio_time_base.num = st->time_base.num;
        reader->audio_time_base.den = st->time_base.den;
    }
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_reader_open(MediaReader *reader)
{
    if (!reader) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (reader->state != MEDIA_READER_STATE_CLOSED) {
        return MEDIA_ERROR_ALREADY_INITIALIZED;
    }
    
    reader->state = MEDIA_READER_STATE_OPENING;
    
    /* 打开输入文件 */
    int ret = avformat_open_input(&reader->fmt_ctx, reader->filename, NULL, NULL);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Failed to open input file: %s\n", reader->filename);
        reader->state = MEDIA_READER_STATE_ERROR;
        return MEDIA_ERROR_FILE_OPEN_FAILED;
    }
    
    /* 设置探测参数 */
    reader->fmt_ctx->probesize = reader->config.probe_size;
    reader->fmt_ctx->max_analyze_duration = reader->config.analyze_duration;
    
    /* 获取流信息 */
    ret = avformat_find_stream_info(reader->fmt_ctx, NULL);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Failed to find stream info\n");
        avformat_close_input(&reader->fmt_ctx);
        reader->state = MEDIA_READER_STATE_ERROR;
        return MEDIA_ERROR_STREAM_INFO;
    }
    
    /* 分配编解码器上下文数组 */
    reader->nb_streams = reader->fmt_ctx->nb_streams;
    reader->codec_ctxs = (AVCodecContext**)media_calloc(reader->nb_streams, sizeof(AVCodecContext*));
    if (!reader->codec_ctxs) {
        avformat_close_input(&reader->fmt_ctx);
        reader->state = MEDIA_READER_STATE_ERROR;
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    /* 分配流映射 */
    reader->stream_map = (int*)media_calloc(reader->nb_streams, sizeof(int));
    if (!reader->stream_map) {
        media_free(reader->codec_ctxs);
        avformat_close_input(&reader->fmt_ctx);
        reader->state = MEDIA_READER_STATE_ERROR;
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    /* 打开所有流的解码器 */
    for (unsigned int i = 0; i < reader->fmt_ctx->nb_streams; i++) {
        open_codec_context(reader, i);
        reader->stream_map[i] = i;
    }
    
    /* 分配帧和包 */
    reader->frame = av_frame_alloc();
    reader->packet = av_packet_alloc();
    
    if (!reader->frame || !reader->packet) {
        media_reader_close(reader);
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    /* 获取时长和起始时间 */
    reader->duration = reader->fmt_ctx->duration;
    reader->start_time = reader->fmt_ctx->start_time;
    
    reader->state = MEDIA_READER_STATE_READY;
    
    MEDIA_LOG_INFO("Opened file: %s\n", reader->filename);
    MEDIA_LOG_INFO("  Duration: %.2f seconds\n", (double)reader->duration / AV_TIME_BASE);
    MEDIA_LOG_INFO("  Video: %dx%d\n", reader->video_width, reader->video_height);
    MEDIA_LOG_INFO("  Audio: %d channels, %d Hz\n", reader->audio_channels, reader->audio_sample_rate);
    
    return MEDIA_SUCCESS;
}

void media_reader_close(MediaReader *reader)
{
    if (!reader) {
        return;
    }
    
    /* 释放帧和包 */
    if (reader->packet) {
        av_packet_free(&reader->packet);
    }
    if (reader->frame) {
        av_frame_free(&reader->frame);
    }
    
    /* 关闭所有解码器 */
    if (reader->codec_ctxs) {
        for (int i = 0; i < reader->nb_streams; i++) {
            if (reader->codec_ctxs[i]) {
                avcodec_free_context(&reader->codec_ctxs[i]);
            }
        }
        media_free(reader->codec_ctxs);
        reader->codec_ctxs = NULL;
    }
    
    /* 释放流映射 */
    if (reader->stream_map) {
        media_free(reader->stream_map);
        reader->stream_map = NULL;
    }
    
    /* 关闭输入文件 */
    if (reader->fmt_ctx) {
        avformat_close_input(&reader->fmt_ctx);
    }
    
    reader->state = MEDIA_READER_STATE_CLOSED;
    reader->nb_streams = 0;
    reader->video_stream_idx = -1;
    reader->audio_stream_idx = -1;
}

/* ============================================================================
 * 读取数据
 * ============================================================================ */

MediaErrorCode media_reader_read_packet(MediaReader *reader, MediaPacket **packet)
{
    if (!reader || !packet) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (reader->state != MEDIA_READER_STATE_READY && 
        reader->state != MEDIA_READER_STATE_READING) {
        return MEDIA_ERROR_NOT_INITIALIZED;
    }
    
    reader->state = MEDIA_READER_STATE_READING;
    
    /* 读取包 */
    int ret = av_read_frame(reader->fmt_ctx, reader->packet);
    if (ret < 0) {
        if (ret == AVERROR_EOF) {
            reader->state = MEDIA_READER_STATE_EOF;
            return MEDIA_EOF;
        }
        reader->state = MEDIA_READER_STATE_ERROR;
        return MEDIA_ERROR_READ_FAILED;
    }
    
    /* 创建MediaPacket */
    *packet = (MediaPacket*)reader->packet; /* 简化实现，实际应封装 */
    
    reader->bytes_read += reader->packet->size;
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_reader_read_frame(MediaReader *reader, MediaFrame **frame)
{
    if (!reader || !frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    MediaErrorCode ret;
    MediaPacket *packet = NULL;
    
    while (1) {
        /* 读取包 */
        ret = media_reader_read_packet(reader, &packet);
        if (ret != MEDIA_SUCCESS) {
            return ret;
        }
        
        /* 解码 */
        ret = media_reader_decode_packet(reader, packet, frame);
        
        /* 释放包 */
        av_packet_unref(reader->packet);
        
        if (ret == MEDIA_SUCCESS) {
            return MEDIA_SUCCESS;
        }
        
        /* 需要更多数据 */
        if (ret == MEDIA_NEED_MORE_DATA) {
            continue;
        }
        
        return ret;
    }
}

MediaErrorCode media_reader_decode_packet(MediaReader *reader, 
                                           MediaPacket *packet, 
                                           MediaFrame **frame)
{
    if (!reader || !packet || !frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    AVPacket *av_pkt = (AVPacket*)packet;
    int stream_index = av_pkt->stream_index;
    
    if (stream_index < 0 || stream_index >= reader->nb_streams) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    AVCodecContext *codec_ctx = reader->codec_ctxs[stream_index];
    if (!codec_ctx) {
        return MEDIA_ERROR_DECODER_NOT_FOUND;
    }
    
    /* 发送包到解码器 */
    int ret = avcodec_send_packet(codec_ctx, av_pkt);
    if (ret < 0) {
        if (ret == AVERROR(EAGAIN)) {
            return MEDIA_NEED_MORE_DATA;
        }
        return MEDIA_ERROR_DECODE_FAILED;
    }
    
    /* 接收解码帧 */
    ret = avcodec_receive_frame(codec_ctx, reader->frame);
    if (ret < 0) {
        if (ret == AVERROR(EAGAIN)) {
            return MEDIA_NEED_MORE_DATA;
        }
        if (ret == AVERROR_EOF) {
            return MEDIA_EOF;
        }
        return MEDIA_ERROR_DECODE_FAILED;
    }
    
    /* 创建MediaFrame */
    *frame = (MediaFrame*)media_calloc(1, sizeof(MediaFrame));
    if (!*frame) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    (*frame)->av_frame = av_frame_clone(reader->frame);
    (*frame)->type = (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) ? 
                      MEDIA_STREAM_TYPE_VIDEO : MEDIA_STREAM_TYPE_AUDIO;
    (*frame)->ref_count = 1;
    (*frame)->is_allocated = true;
    
    reader->frames_decoded++;
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 流信息获取
 * ============================================================================ */

int32_t media_reader_get_stream_count(MediaReader *reader)
{
    if (!reader) return 0;
    return reader->nb_streams;
}

MediaStreamType media_reader_get_stream_type(MediaReader *reader, int32_t stream_index)
{
    if (!reader || !reader->fmt_ctx || stream_index < 0 || 
        stream_index >= reader->nb_streams) {
        return MEDIA_STREAM_TYPE_UNKNOWN;
    }
    
    AVStream *st = reader->fmt_ctx->streams[stream_index];
    switch (st->codecpar->codec_type) {
        case AVMEDIA_TYPE_VIDEO: return MEDIA_STREAM_TYPE_VIDEO;
        case AVMEDIA_TYPE_AUDIO: return MEDIA_STREAM_TYPE_AUDIO;
        case AVMEDIA_TYPE_SUBTITLE: return MEDIA_STREAM_TYPE_SUBTITLE;
        case AVMEDIA_TYPE_DATA: return MEDIA_STREAM_TYPE_DATA;
        default: return MEDIA_STREAM_TYPE_UNKNOWN;
    }
}

MediaStreamInfo* media_reader_get_stream_info(MediaReader *reader, int32_t stream_index)
{
    if (!reader || !reader->fmt_ctx || stream_index < 0 || 
        stream_index >= reader->nb_streams) {
        return NULL;
    }
    
    MediaStreamInfo *info = (MediaStreamInfo*)media_calloc(1, sizeof(MediaStreamInfo));
    if (!info) return NULL;
    
    AVStream *st = reader->fmt_ctx->streams[stream_index];
    AVCodecContext *codec_ctx = reader->codec_ctxs[stream_index];
    
    info->type = media_reader_get_stream_type(reader, stream_index);
    info->codec_id = (MediaCodecID)st->codecpar->codec_id;
    info->time_base.num = st->time_base.num;
    info->time_base.den = st->time_base.den;
    info->duration = st->duration;
    info->nb_frames = st->nb_frames;
    info->bit_rate = st->codecpar->bit_rate;
    
    if (info->type == MEDIA_STREAM_TYPE_VIDEO) {
        info->width = st->codecpar->width;
        info->height = st->codecpar->height;
        info->pixel_format = (MediaPixelFormat)st->codecpar->format;
        info->frame_rate = media_rational_make(st->r_frame_rate.num, st->r_frame_rate.den);
        info->aspect_ratio = media_rational_make(st->codecpar->sample_aspect_ratio.num,
                                                  st->codecpar->sample_aspect_ratio.den);
    } else if (info->type == MEDIA_STREAM_TYPE_AUDIO) {
        info->sample_rate = st->codecpar->sample_rate;
        info->channels = st->codecpar->channels;
        info->sample_format = (MediaSampleFormat)st->codecpar->format;
        info->channel_layout = st->codecpar->channel_layout;
    }
    
    return info;
}

int32_t media_reader_get_video_stream_index(MediaReader *reader)
{
    if (!reader) return -1;
    return reader->video_stream_idx;
}

int32_t media_reader_get_audio_stream_index(MediaReader *reader)
{
    if (!reader) return -1;
    return reader->audio_stream_idx;
}

/* ============================================================================
 * 文件信息获取
 * ============================================================================ */

double media_reader_get_duration(MediaReader *reader)
{
    if (!reader) return 0.0;
    return (double)reader->duration / AV_TIME_BASE;
}

double media_reader_get_start_time(MediaReader *reader)
{
    if (!reader) return 0.0;
    return (double)reader->start_time / AV_TIME_BASE;
}

int64_t media_reader_get_bit_rate(MediaReader *reader)
{
    if (!reader || !reader->fmt_ctx) return 0;
    return reader->fmt_ctx->bit_rate;
}

const char* media_reader_get_filename(MediaReader *reader)
{
    if (!reader) return NULL;
    return reader->filename;
}

const char* media_reader_get_format_name(MediaReader *reader)
{
    if (!reader || !reader->fmt_ctx || !reader->fmt_ctx->iformat) return NULL;
    return reader->fmt_ctx->iformat->name;
}

/* ============================================================================
 * 视频信息获取
 * ============================================================================ */

int32_t media_reader_get_video_width(MediaReader *reader)
{
    if (!reader) return 0;
    return reader->video_width;
}

int32_t media_reader_get_video_height(MediaReader *reader)
{
    if (!reader) return 0;
    return reader->video_height;
}

MediaPixelFormat media_reader_get_video_pixel_format(MediaReader *reader)
{
    if (!reader) return MEDIA_PIX_FMT_NONE;
    return reader->video_format;
}

MediaRational media_reader_get_video_frame_rate(MediaReader *reader)
{
    if (!reader) return media_rational_make(0, 1);
    return reader->video_frame_rate;
}

/* ============================================================================
 * 音频信息获取
 * ============================================================================ */

int32_t media_reader_get_audio_sample_rate(MediaReader *reader)
{
    if (!reader) return 0;
    return reader->audio_sample_rate;
}

int32_t media_reader_get_audio_channels(MediaReader *reader)
{
    if (!reader) return 0;
    return reader->audio_channels;
}

MediaSampleFormat media_reader_get_audio_sample_format(MediaReader *reader)
{
    if (!reader) return MEDIA_SAMPLE_FMT_NONE;
    return reader->audio_format;
}

/* ============================================================================
 * 寻址
 * ============================================================================ */

MediaErrorCode media_reader_seek(MediaReader *reader, double timestamp)
{
    if (!reader || !reader->fmt_ctx) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    int64_t ts = (int64_t)(timestamp * AV_TIME_BASE);
    
    int ret = av_seek_frame(reader->fmt_ctx, -1, ts, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Seek failed to timestamp %.2f\n", timestamp);
        return MEDIA_ERROR_SEEK_FAILED;
    }
    
    /* 刷新解码器缓冲区 */
    for (int i = 0; i < reader->nb_streams; i++) {
        if (reader->codec_ctxs[i]) {
            avcodec_flush_buffers(reader->codec_ctxs[i]);
        }
    }
    
    reader->state = MEDIA_READER_STATE_READY;
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_reader_seek_to_frame(MediaReader *reader, 
                                           int64_t frame_number, 
                                           int32_t stream_index)
{
    if (!reader || !reader->fmt_ctx) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (stream_index < 0 || stream_index >= reader->nb_streams) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    AVStream *st = reader->fmt_ctx->streams[stream_index];
    
    /* 计算时间戳 */
    int64_t ts = av_rescale_q(frame_number, 
                              av_inv_q(st->r_frame_rate), 
                              st->time_base);
    
    int ret = av_seek_frame(reader->fmt_ctx, stream_index, ts, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        return MEDIA_ERROR_SEEK_FAILED;
    }
    
    /* 刷新解码器缓冲区 */
    for (int i = 0; i < reader->nb_streams; i++) {
        if (reader->codec_ctxs[i]) {
            avcodec_flush_buffers(reader->codec_ctxs[i]);
        }
    }
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_reader_seek_to_start(MediaReader *reader)
{
    return media_reader_seek(reader, 0.0);
}

MediaErrorCode media_reader_seek_to_end(MediaReader *reader)
{
    if (!reader) return MEDIA_ERROR_NULL_POINTER;
    return media_reader_seek(reader, (double)reader->duration / AV_TIME_BASE - 1.0);
}

double media_reader_get_position(MediaReader *reader)
{
    if (!reader || !reader->frame) return 0.0;
    
    int64_t pts = reader->frame->pts;
    if (pts == AV_NOPTS_VALUE) return 0.0;
    
    /* 转换为秒 */
    AVStream *st = NULL;
    if (reader->video_stream_idx >= 0) {
        st = reader->fmt_ctx->streams[reader->video_stream_idx];
    } else if (reader->audio_stream_idx >= 0) {
        st = reader->fmt_ctx->streams[reader->audio_stream_idx];
    }
    
    if (!st) return 0.0;
    
    return (double)pts * st->time_base.num / st->time_base.den;
}

/* ============================================================================
 * 缓冲控制
 * ============================================================================ */

double media_reader_get_buffer_usage(MediaReader *reader)
{
    /* 简化实现 */
    return 0.0;
}

void media_reader_flush_buffer(MediaReader *reader)
{
    if (!reader) return;
    
    for (int i = 0; i < reader->nb_streams; i++) {
        if (reader->codec_ctxs[i]) {
            avcodec_flush_buffers(reader->codec_ctxs[i]);
        }
    }
}

MediaErrorCode media_reader_set_buffer_size(MediaReader *reader, int32_t size)
{
    if (!reader) return MEDIA_ERROR_NULL_POINTER;
    reader->config.buffer_size = size;
    return MEDIA_SUCCESS;
}
