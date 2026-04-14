/**
 * @file media_writer.c
 * @brief 媒体写入器实现
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 */

#include "output/media_writer.h"
#include "utils/media_utils.h"
#include <stdlib.h>
#include <string.h>

/* FFmpeg头文件 */
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

/* 内部写入器结构体 */
struct MediaWriter {
    AVFormatContext *fmt_ctx;       ///< 格式上下文
    AVCodecContext **codec_ctxs;    ///< 编解码器上下文数组
    AVStream **streams;             ///< 流数组
    AVFrame *frame;                 ///< 编码帧
    AVPacket *packet;               ///< 数据包
    
    char *filename;                 ///< 文件名
    MediaWriterState state;         ///< 状态
    MediaWriterConfig config;       ///< 配置
    
    int nb_streams;                 ///< 流数量
    int max_streams;                ///< 最大流数量
    
    /* 统计 */
    int64_t bytes_written;
    int64_t *frames_written;        ///< 每个流的已写入帧数
    int64_t *last_pts;              ///< 每个流的最后PTS
    
    /* 编码器选项 */
    AVDictionary **codec_opts;      ///< 每个流的编码器选项
    
    /* 元数据 */
    AVDictionary *metadata;         ///< 文件元数据
};

/* ============================================================================
 * 配置默认值
 * ============================================================================ */

void media_writer_config_default(MediaWriterConfig *config)
{
    if (!config) return;
    
    memset(config, 0, sizeof(MediaWriterConfig));
    config->container = MEDIA_CONTAINER_MP4;
    config->buffer_size = 4 * 1024 * 1024;  /* 4MB */
    config->preload = 0;
    config->max_delay = 0;
    config->use_wallclock_ts = 0;
    config->fast_start = 1;
    config->movflags = 0;
    config->overwrite = 1;
    config->thread_count = 0;  /* 自动 */
    config->format = NULL;
    config->user_data = NULL;
}

/* ============================================================================
 * 创建与销毁
 * ============================================================================ */

MediaWriter* media_writer_create(void)
{
    MediaWriter *writer = (MediaWriter*)calloc(1, sizeof(MediaWriter));
    if (!writer) {
        MEDIA_LOG_ERROR("Failed to allocate writer\n");
        return NULL;
    }
    
    writer->state = MEDIA_WRITER_STATE_CLOSED;
    writer->max_streams = 16;
    
    writer->codec_ctxs = (AVCodecContext**)calloc(writer->max_streams, 
                                                    sizeof(AVCodecContext*));
    writer->streams = (AVStream**)calloc(writer->max_streams, 
                                          sizeof(AVStream*));
    writer->frames_written = (int64_t*)calloc(writer->max_streams, 
                                               sizeof(int64_t));
    writer->last_pts = (int64_t*)calloc(writer->max_streams, 
                                         sizeof(int64_t));
    writer->codec_opts = (AVDictionary**)calloc(writer->max_streams, 
                                                 sizeof(AVDictionary*));
    
    if (!writer->codec_ctxs || !writer->streams || 
        !writer->frames_written || !writer->last_pts || !writer->codec_opts) {
        media_writer_free(writer);
        return NULL;
    }
    
    media_writer_config_default(&writer->config);
    
    writer->packet = av_packet_alloc();
    writer->frame = av_frame_alloc();
    
    if (!writer->packet || !writer->frame) {
        media_writer_free(writer);
        return NULL;
    }
    
    MEDIA_LOG_INFO("Writer created\n");
    return writer;
}

MediaWriter* media_writer_create_with_config(const MediaWriterConfig *config)
{
    MediaWriter *writer = media_writer_create();
    if (!writer) return NULL;
    
    if (config) {
        memcpy(&writer->config, config, sizeof(MediaWriterConfig));
    }
    
    return writer;
}

void media_writer_free(MediaWriter *writer)
{
    if (!writer) return;
    
    media_writer_close(writer);
    
    if (writer->packet) {
        av_packet_free(&writer->packet);
    }
    if (writer->frame) {
        av_frame_free(&writer->frame);
    }
    
    for (int i = 0; i < writer->nb_streams; i++) {
        if (writer->codec_ctxs && writer->codec_ctxs[i]) {
            avcodec_free_context(&writer->codec_ctxs[i]);
        }
        if (writer->codec_opts && writer->codec_opts[i]) {
            av_dict_free(&writer->codec_opts[i]);
        }
    }
    
    free(writer->codec_ctxs);
    free(writer->streams);
    free(writer->frames_written);
    free(writer->last_pts);
    free(writer->codec_opts);
    free(writer->filename);
    
    if (writer->metadata) {
        av_dict_free(&writer->metadata);
    }
    
    free(writer);
    MEDIA_LOG_INFO("Writer freed\n");
}

/* ============================================================================
 * 文件打开与关闭
 * ============================================================================ */

MediaErrorCode media_writer_open(MediaWriter *writer, const char *filename)
{
    if (!writer || !filename) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (writer->state != MEDIA_WRITER_STATE_CLOSED) {
        return MEDIA_ERROR_INVALID_STATE;
    }
    
    writer->state = MEDIA_WRITER_STATE_OPENING;
    
    /* 分配输出格式上下文 */
    int ret = avformat_alloc_output_context2(&writer->fmt_ctx, NULL, 
                                              writer->config.format, filename);
    if (ret < 0 || !writer->fmt_ctx) {
        writer->state = MEDIA_WRITER_STATE_ERROR;
        MEDIA_LOG_ERROR("Failed to allocate output context\n");
        return MEDIA_ERROR_FORMAT_NOT_SUPPORTED;
    }
    
    /* 保存文件名 */
    writer->filename = strdup(filename);
    if (!writer->filename) {
        avformat_free_context(writer->fmt_ctx);
        writer->fmt_ctx = NULL;
        writer->state = MEDIA_WRITER_STATE_ERROR;
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    /* 设置输出选项 */
    if (writer->config.fast_start && 
        strcmp(writer->fmt_ctx->oformat->name, "mp4") == 0) {
        av_opt_set(writer->fmt_ctx->priv_data, "movflags", 
                   "faststart", 0);
    }
    
    /* 打开输出文件（如果不是网络流） */
    if (!(writer->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&writer->fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            MEDIA_LOG_ERROR("Failed to open output file: %s\n", filename);
            avformat_free_context(writer->fmt_ctx);
            writer->fmt_ctx = NULL;
            free(writer->filename);
            writer->filename = NULL;
            writer->state = MEDIA_WRITER_STATE_ERROR;
            return MEDIA_ERROR_FILE_OPEN_FAILED;
        }
    }
    
    writer->state = MEDIA_WRITER_STATE_READY;
    MEDIA_LOG_INFO("Writer opened: %s\n", filename);
    return MEDIA_SUCCESS;
}

MediaErrorCode media_writer_open_with_config(MediaWriter *writer, 
                                              const char *filename, 
                                              const MediaWriterConfig *config)
{
    if (!writer || !filename || !config) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    memcpy(&writer->config, config, sizeof(MediaWriterConfig));
    return media_writer_open(writer, filename);
}

MediaErrorCode media_writer_close(MediaWriter *writer)
{
    if (!writer) return MEDIA_ERROR_NULL_POINTER;
    
    if (writer->state == MEDIA_WRITER_STATE_CLOSED) {
        return MEDIA_SUCCESS;
    }
    
    writer->state = MEDIA_WRITER_STATE_FINALIZING;
    
    /* 写入文件尾 */
    if (writer->fmt_ctx) {
        if (writer->nb_streams > 0) {
            av_write_trailer(writer->fmt_ctx);
        }
        
        /* 关闭输出文件 */
        if (writer->fmt_ctx->pb && 
            !(writer->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&writer->fmt_ctx->pb);
        }
        
        avformat_free_context(writer->fmt_ctx);
        writer->fmt_ctx = NULL;
    }
    
    /* 重置流信息 */
    for (int i = 0; i < writer->nb_streams; i++) {
        if (writer->codec_ctxs && writer->codec_ctxs[i]) {
            avcodec_free_context(&writer->codec_ctxs[i]);
        }
        writer->streams[i] = NULL;
        writer->frames_written[i] = 0;
        writer->last_pts[i] = 0;
    }
    writer->nb_streams = 0;
    
    writer->state = MEDIA_WRITER_STATE_CLOSED;
    MEDIA_LOG_INFO("Writer closed\n");
    return MEDIA_SUCCESS;
}

bool media_writer_is_open(MediaWriter *writer)
{
    if (!writer) return false;
    return writer->state >= MEDIA_WRITER_STATE_READY && 
           writer->state <= MEDIA_WRITER_STATE_FINALIZING;
}

MediaWriterState media_writer_get_state(MediaWriter *writer)
{
    if (!writer) return MEDIA_WRITER_STATE_CLOSED;
    return writer->state;
}

/* ============================================================================
 * 流配置
 * ============================================================================ */

int32_t media_writer_add_video_stream(MediaWriter *writer, 
                                       const VideoParams *params)
{
    if (!writer || !params || !writer->fmt_ctx) {
        return -1;
    }
    
    if (writer->nb_streams >= writer->max_streams) {
        MEDIA_LOG_ERROR("Maximum stream count reached\n");
        return -1;
    }
    
    /* 查找编码器 */
    const AVCodec *codec = NULL;
    if (params->codec_id != MEDIA_CODEC_ID_NONE) {
        codec = avcodec_find_encoder((enum AVCodecID)params->codec_id);
    }
    if (!codec && params->codec_name) {
        codec = avcodec_find_encoder_by_name(params->codec_name);
    }
    if (!codec) {
        /* 默认使用H264 */
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    if (!codec) {
        MEDIA_LOG_ERROR("Failed to find encoder\n");
        return -1;
    }
    
    /* 创建流 */
    AVStream *st = avformat_new_stream(writer->fmt_ctx, codec);
    if (!st) {
        MEDIA_LOG_ERROR("Failed to create stream\n");
        return -1;
    }
    
    int stream_index = writer->nb_streams;
    st->id = stream_index;
    
    /* 创建编码器上下文 */
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        MEDIA_LOG_ERROR("Failed to allocate codec context\n");
        return -1;
    }
    
    /* 设置视频参数 */
    codec_ctx->width = params->width > 0 ? params->width : 1920;
    codec_ctx->height = params->height > 0 ? params->height : 1080;
    codec_ctx->bit_rate = params->bit_rate > 0 ? params->bit_rate : 4000000;
    codec_ctx->time_base = (AVRational){params->time_base.num, params->time_base.den};
    if (codec_ctx->time_base.num == 0 || codec_ctx->time_base.den == 0) {
        codec_ctx->time_base = (AVRational){1, 25};
    }
    codec_ctx->framerate = (AVRational){params->frame_rate.num, params->frame_rate.den};
    if (codec_ctx->framerate.num == 0 || codec_ctx->framerate.den == 0) {
        codec_ctx->framerate = (AVRational){25, 1};
    }
    codec_ctx->gop_size = params->gop_size > 0 ? params->gop_size : 12;
    codec_ctx->max_b_frames = params->max_b_frames;
    codec_ctx->pix_fmt = (enum AVPixelFormat)params->pixel_format;
    if (codec_ctx->pix_fmt == AV_PIX_FMT_NONE) {
        codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    }
    
    /* 设置流时间基 */
    st->time_base = codec_ctx->time_base;
    
    /* 打开编码器 */
    int ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Failed to open codec: %d\n", ret);
        avcodec_free_context(&codec_ctx);
        return -1;
    }
    
    /* 复制编码器参数到流 */
    ret = avcodec_parameters_from_context(st->codecpar, codec_ctx);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Failed to copy codec parameters\n");
        avcodec_free_context(&codec_ctx);
        return -1;
    }
    
    writer->codec_ctxs[stream_index] = codec_ctx;
    writer->streams[stream_index] = st;
    writer->nb_streams++;
    
    MEDIA_LOG_INFO("Added video stream %d: %dx%d\n", stream_index, 
                   codec_ctx->width, codec_ctx->height);
    return stream_index;
}

int32_t media_writer_add_audio_stream(MediaWriter *writer, 
                                       const AudioParams *params)
{
    if (!writer || !params || !writer->fmt_ctx) {
        return -1;
    }
    
    if (writer->nb_streams >= writer->max_streams) {
        MEDIA_LOG_ERROR("Maximum stream count reached\n");
        return -1;
    }
    
    /* 查找编码器 */
    const AVCodec *codec = NULL;
    if (params->codec_id != MEDIA_CODEC_ID_NONE) {
        codec = avcodec_find_encoder((enum AVCodecID)params->codec_id);
    }
    if (!codec && params->codec_name) {
        codec = avcodec_find_encoder_by_name(params->codec_name);
    }
    if (!codec) {
        /* 默认使用AAC */
        codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    }
    if (!codec) {
        MEDIA_LOG_ERROR("Failed to find encoder\n");
        return -1;
    }
    
    /* 创建流 */
    AVStream *st = avformat_new_stream(writer->fmt_ctx, codec);
    if (!st) {
        MEDIA_LOG_ERROR("Failed to create stream\n");
        return -1;
    }
    
    int stream_index = writer->nb_streams;
    st->id = stream_index;
    
    /* 创建编码器上下文 */
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        MEDIA_LOG_ERROR("Failed to allocate codec context\n");
        return -1;
    }
    
    /* 设置音频参数 */
    codec_ctx->sample_rate = params->sample_rate > 0 ? params->sample_rate : 44100;
    codec_ctx->channels = params->channels > 0 ? params->channels : 2;
    codec_ctx->channel_layout = av_get_default_channel_layout(codec_ctx->channels);
    codec_ctx->bit_rate = params->bit_rate > 0 ? params->bit_rate : 128000;
    codec_ctx->sample_fmt = (enum AVSampleFormat)params->sample_format;
    if (codec_ctx->sample_fmt == AV_SAMPLE_FMT_NONE) {
        codec_ctx->sample_fmt = codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    }
    codec_ctx->time_base = (AVRational){1, codec_ctx->sample_rate};
    
    /* 设置流时间基 */
    st->time_base = codec_ctx->time_base;
    
    /* 打开编码器 */
    int ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Failed to open codec: %d\n", ret);
        avcodec_free_context(&codec_ctx);
        return -1;
    }
    
    /* 复制编码器参数到流 */
    ret = avcodec_parameters_from_context(st->codecpar, codec_ctx);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Failed to copy codec parameters\n");
        avcodec_free_context(&codec_ctx);
        return -1;
    }
    
    writer->codec_ctxs[stream_index] = codec_ctx;
    writer->streams[stream_index] = st;
    writer->nb_streams++;
    
    MEDIA_LOG_INFO("Added audio stream %d: %dHz, %dch\n", stream_index, 
                   codec_ctx->sample_rate, codec_ctx->channels);
    return stream_index;
}

int32_t media_writer_add_stream(MediaWriter *writer, 
                                 const MediaStreamConfig *config)
{
    if (!writer || !config) return -1;
    
    if (config->type == MEDIA_STREAM_TYPE_VIDEO) {
        return media_writer_add_video_stream(writer, &config->params.video);
    } else if (config->type == MEDIA_STREAM_TYPE_AUDIO) {
        return media_writer_add_audio_stream(writer, &config->params.audio);
    }
    
    return -1;
}

MediaErrorCode media_writer_remove_stream(MediaWriter *writer, 
                                           int32_t stream_index)
{
    if (!writer) return MEDIA_ERROR_NULL_POINTER;
    if (stream_index < 0 || stream_index >= writer->nb_streams) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    /* 注意：FFmpeg不支持动态移除流，这里只是标记 */
    MEDIA_LOG_WARNING("Stream removal not fully supported\n");
    return MEDIA_ERROR_NOT_SUPPORTED;
}

int32_t media_writer_get_stream_count(MediaWriter *writer)
{
    if (!writer) return 0;
    return writer->nb_streams;
}

MediaErrorCode media_writer_get_stream_config(MediaWriter *writer, 
                                               int32_t stream_index, 
                                               MediaStreamConfig *config)
{
    if (!writer || !config) return MEDIA_ERROR_NULL_POINTER;
    if (stream_index < 0 || stream_index >= writer->nb_streams) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    AVStream *st = writer->streams[stream_index];
    AVCodecContext *codec_ctx = writer->codec_ctxs[stream_index];
    
    if (!st || !codec_ctx) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    memset(config, 0, sizeof(MediaStreamConfig));
    
    if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        config->type = MEDIA_STREAM_TYPE_VIDEO;
        config->params.video.width = codec_ctx->width;
        config->params.video.height = codec_ctx->height;
        config->params.video.bit_rate = codec_ctx->bit_rate;
        config->params.video.frame_rate.num = codec_ctx->framerate.num;
        config->params.video.frame_rate.den = codec_ctx->framerate.den;
        config->params.video.pixel_format = (MediaPixelFormat)codec_ctx->pix_fmt;
        config->params.video.gop_size = codec_ctx->gop_size;
        config->params.video.max_b_frames = codec_ctx->max_b_frames;
    } else if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        config->type = MEDIA_STREAM_TYPE_AUDIO;
        config->params.audio.sample_rate = codec_ctx->sample_rate;
        config->params.audio.channels = codec_ctx->channels;
        config->params.audio.bit_rate = codec_ctx->bit_rate;
        config->params.audio.sample_format = (MediaSampleFormat)codec_ctx->sample_fmt;
    }
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 编码器配置
 * ============================================================================ */

MediaErrorCode media_writer_set_codec(MediaWriter *writer, 
                                       int32_t stream_index, 
                                       MediaCodecID codec_id)
{
    if (!writer) return MEDIA_ERROR_NULL_POINTER;
    if (stream_index < 0 || stream_index >= writer->nb_streams) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    /* 编码器在添加流时已设置，这里只能记录 */
    MEDIA_LOG_INFO("Set codec for stream %d: %d\n", stream_index, codec_id);
    return MEDIA_SUCCESS;
}

MediaErrorCode media_writer_set_codec_by_name(MediaWriter *writer, 
                                               int32_t stream_index, 
                                               const char *codec_name)
{
    if (!writer || !codec_name) return MEDIA_ERROR_NULL_POINTER;
    if (stream_index < 0 || stream_index >= writer->nb_streams) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    MEDIA_LOG_INFO("Set codec for stream %d: %s\n", stream_index, codec_name);
    return MEDIA_SUCCESS;
}

MediaErrorCode media_writer_set_codec_option(MediaWriter *writer, 
                                              int32_t stream_index, 
                                              const char *key, 
                                              const char *value)
{
    if (!writer || !key || !value) return MEDIA_ERROR_NULL_POINTER;
    if (stream_index < 0 || stream_index >= writer->nb_streams) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    return av_dict_set(&writer->codec_opts[stream_index], key, value, 0) < 0 ?
           MEDIA_ERROR_OPERATION_FAILED : MEDIA_SUCCESS;
}

MediaErrorCode media_writer_set_video_encode_params(MediaWriter *writer, 
                                                     int32_t stream_index, 
                                                     int32_t bit_rate, 
                                                     int32_t gop_size, 
                                                     int32_t max_b_frames)
{
    if (!writer) return MEDIA_ERROR_NULL_POINTER;
    if (stream_index < 0 || stream_index >= writer->nb_streams) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    AVCodecContext *ctx = writer->codec_ctxs[stream_index];
    if (!ctx || ctx->codec_type != AVMEDIA_TYPE_VIDEO) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    if (bit_rate > 0) ctx->bit_rate = bit_rate;
    if (gop_size > 0) ctx->gop_size = gop_size;
    ctx->max_b_frames = max_b_frames;
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_writer_set_audio_encode_params(MediaWriter *writer, 
                                                     int32_t stream_index, 
                                                     int32_t bit_rate)
{
    if (!writer) return MEDIA_ERROR_NULL_POINTER;
    if (stream_index < 0 || stream_index >= writer->nb_streams) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    AVCodecContext *ctx = writer->codec_ctxs[stream_index];
    if (!ctx || ctx->codec_type != AVMEDIA_TYPE_AUDIO) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    if (bit_rate > 0) ctx->bit_rate = bit_rate;
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 数据写入
 * ============================================================================ */

MediaErrorCode media_writer_write_packet(MediaWriter *writer, 
                                          const MediaPacket *packet)
{
    if (!writer || !packet || !writer->fmt_ctx) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (writer->state != MEDIA_WRITER_STATE_READY && 
        writer->state != MEDIA_WRITER_STATE_WRITING) {
        return MEDIA_ERROR_INVALID_STATE;
    }
    
    writer->state = MEDIA_WRITER_STATE_WRITING;
    
    /* 写入数据包 */
    int ret = av_interleaved_write_frame(writer->fmt_ctx, packet->av_packet);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Failed to write packet: %d\n", ret);
        return MEDIA_ERROR_WRITE_FAILED;
    }
    
    writer->bytes_written += packet->av_packet->size;
    writer->frames_written[packet->av_packet->stream_index]++;
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_writer_write_frame(MediaWriter *writer, 
                                         const MediaFrame *frame, 
                                         int32_t stream_index)
{
    if (!writer || !frame || !writer->fmt_ctx) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (stream_index < 0 || stream_index >= writer->nb_streams) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    if (writer->state != MEDIA_WRITER_STATE_READY && 
        writer->state != MEDIA_WRITER_STATE_WRITING) {
        return MEDIA_ERROR_INVALID_STATE;
    }
    
    writer->state = MEDIA_WRITER_STATE_WRITING;
    
    AVCodecContext *codec_ctx = writer->codec_ctxs[stream_index];
    if (!codec_ctx) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    /* 发送帧到编码器 */
    int ret = avcodec_send_frame(codec_ctx, frame->av_frame);
    if (ret < 0) {
        MEDIA_LOG_ERROR("Failed to send frame to encoder: %d\n", ret);
        return MEDIA_ERROR_ENCODE_FAILED;
    }
    
    /* 接收编码后的数据包 */
    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_ctx, writer->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            MEDIA_LOG_ERROR("Failed to receive packet from encoder: %d\n", ret);
            return MEDIA_ERROR_ENCODE_FAILED;
        }
        
        /* 设置流索引和时间戳 */
        writer->packet->stream_index = stream_index;
        
        /* 写入数据包 */
        ret = av_interleaved_write_frame(writer->fmt_ctx, writer->packet);
        if (ret < 0) {
            MEDIA_LOG_ERROR("Failed to write packet: %d\n", ret);
            return MEDIA_ERROR_WRITE_FAILED;
        }
        
        writer->bytes_written += writer->packet->size;
        writer->frames_written[stream_index]++;
    }
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_writer_encode_and_write(MediaWriter *writer, 
                                              const MediaFrame *frame, 
                                              int32_t stream_index)
{
    return media_writer_write_frame(writer, frame, stream_index);
}

MediaErrorCode media_writer_flush_encoder(MediaWriter *writer, 
                                           int32_t stream_index)
{
    if (!writer || !writer->fmt_ctx) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (stream_index == -1) {
        /* 刷新所有编码器 */
        for (int i = 0; i < writer->nb_streams; i++) {
            if (writer->codec_ctxs[i]) {
                avcodec_send_frame(writer->codec_ctxs[i], NULL);
            }
        }
    } else {
        if (stream_index < 0 || stream_index >= writer->nb_streams) {
            return MEDIA_ERROR_INVALID_PARAM;
        }
        
        if (writer->codec_ctxs[stream_index]) {
            avcodec_send_frame(writer->codec_ctxs[stream_index], NULL);
        }
    }
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 元数据管理
 * ============================================================================ */

MediaErrorCode media_writer_set_metadata(MediaWriter *writer, 
                                          const char *key, 
                                          const char *value)
{
    if (!writer || !key || !value) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    return av_dict_set(&writer->metadata, key, value, 0) < 0 ?
           MEDIA_ERROR_OPERATION_FAILED : MEDIA_SUCCESS;
}

MediaErrorCode media_writer_set_stream_metadata(MediaWriter *writer, 
                                                 int32_t stream_index, 
                                                 const char *key, 
                                                 const char *value)
{
    if (!writer || !key || !value) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (stream_index < 0 || stream_index >= writer->nb_streams) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    AVStream *st = writer->streams[stream_index];
    if (!st) return MEDIA_ERROR_INVALID_PARAM;
    
    return av_dict_set(&st->metadata, key, value, 0) < 0 ?
           MEDIA_ERROR_OPERATION_FAILED : MEDIA_SUCCESS;
}

MediaErrorCode media_writer_set_cover_art(MediaWriter *writer, 
                                           const char *cover_file)
{
    if (!writer || !cover_file) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    /* TODO: 实现封面图设置 */
    MEDIA_LOG_INFO("Cover art not yet implemented\n");
    return MEDIA_ERROR_NOT_SUPPORTED;
}

/* ============================================================================
 * 时间戳管理
 * ============================================================================ */

MediaErrorCode media_writer_set_time_base(MediaWriter *writer, 
                                           int32_t stream_index, 
                                           MediaRational time_base)
{
    if (!writer) return MEDIA_ERROR_NULL_POINTER;
    if (stream_index < 0 || stream_index >= writer->nb_streams) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    AVStream *st = writer->streams[stream_index];
    if (!st) return MEDIA_ERROR_INVALID_PARAM;
    
    st->time_base = (AVRational){time_base.num, time_base.den};
    return MEDIA_SUCCESS;
}

MediaRational media_writer_get_time_base(MediaWriter *writer, 
                                          int32_t stream_index)
{
    MediaRational default_tb = {0, 1};
    
    if (!writer) return default_tb;
    if (stream_index < 0 || stream_index >= writer->nb_streams) {
        return default_tb;
    }
    
    AVStream *st = writer->streams[stream_index];
    if (!st) return default_tb;
    
    MediaRational tb = {st->time_base.num, st->time_base.den};
    return tb;
}

int64_t media_writer_rescale_ts(MediaWriter *writer, 
                                 int64_t ts, 
                                 MediaRational src_tb, 
                                 int32_t dst_stream_index)
{
    if (!writer || dst_stream_index < 0 || dst_stream_index >= writer->nb_streams) {
        return ts;
    }
    
    AVStream *st = writer->streams[dst_stream_index];
    if (!st) return ts;
    
    return av_rescale_q(ts, (AVRational){src_tb.num, src_tb.den}, st->time_base);
}

/* ============================================================================
 * 状态查询
 * ============================================================================ */

int64_t media_writer_get_bytes_written(MediaWriter *writer)
{
    if (!writer) return 0;
    return writer->bytes_written;
}

int64_t media_writer_get_frames_written(MediaWriter *writer, 
                                         int32_t stream_index)
{
    if (!writer) return 0;
    if (stream_index < 0 || stream_index >= writer->nb_streams) {
        return 0;
    }
    return writer->frames_written[stream_index];
}

double media_writer_get_duration_written(MediaWriter *writer, 
                                          int32_t stream_index)
{
    if (!writer) return 0.0;
    if (stream_index < 0 || stream_index >= writer->nb_streams) {
        return 0.0;
    }
    
    AVStream *st = writer->streams[stream_index];
    if (!st || st->time_base.den == 0) return 0.0;
    
    return (double)writer->last_pts[stream_index] * st->time_base.num / st->time_base.den;
}

const char* media_writer_get_filename(MediaWriter *writer)
{
    if (!writer) return NULL;
    return writer->filename;
}

MediaContainerFormat media_writer_get_container_format(MediaWriter *writer)
{
    if (!writer || !writer->fmt_ctx || !writer->fmt_ctx->oformat) {
        return MEDIA_CONTAINER_NONE;
    }
    
    const char *name = writer->fmt_ctx->oformat->name;
    
    if (strstr(name, "mp4")) return MEDIA_CONTAINER_MP4;
    if (strstr(name, "matroska") || strstr(name, "mkv")) return MEDIA_CONTAINER_MKV;
    if (strstr(name, "avi")) return MEDIA_CONTAINER_AVI;
    if (strstr(name, "mpegts") || strstr(name, "ts")) return MEDIA_CONTAINER_MPEGTS;
    if (strstr(name, "flv")) return MEDIA_CONTAINER_FLV;
    if (strstr(name, "webm")) return MEDIA_CONTAINER_WEBM;
    if (strstr(name, "wav")) return MEDIA_CONTAINER_WAV;
    if (strstr(name, "mp3")) return MEDIA_CONTAINER_MP3;
    if (strstr(name, "ogg")) return MEDIA_CONTAINER_OGG;
    
    return MEDIA_CONTAINER_NONE;
}
