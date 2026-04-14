/**
 * @file media_streaming.c
 * @brief 流媒体处理实现
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 */

#include "output/media_streaming.h"
#include "utils/media_utils.h"
#include <stdlib.h>
#include <string.h>

/* FFmpeg头文件 */
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
}

/* 内部流媒体器结构体 */
struct MediaStreamer {
    AVFormatContext *input_ctx;     ///< 输入格式上下文
    AVFormatContext *output_ctx;    ///< 输出格式上下文
    AVCodecContext **codec_ctxs;    ///< 编解码器上下文数组
    
    char url[1024];                 ///< 服务器URL
    MediaStreamerState state;       ///< 状态
    MediaStreamConfig config;       ///< 配置
    
    MediaStreamDirection direction; ///< 方向
    
    /* 统计 */
    int64_t bytes_sent;
    int64_t bytes_received;
    int64_t frames_sent;
    int64_t frames_received;
    int64_t packets_lost;
    int32_t current_bitrate;
    int32_t latency_ms;
    
    /* 线程控制 */
    int running;
    void *thread_handle;
    
    /* 回调 */
    MediaStreamCallback stream_callback;
    void *callback_user_data;
};

/* ============================================================================
 * 配置默认值
 * ============================================================================ */

void media_stream_config_default(MediaStreamConfig *config)
{
    if (!config) return;
    
    memset(config, 0, sizeof(MediaStreamConfig));
    config->reconnect = 1;
    config->reconnect_interval = 1000;  /* 1秒 */
    config->reconnect_count = 5;
    config->timeout = 5000;             /* 5秒 */
    config->buffer_size = 1024 * 1024;  /* 1MB */
    config->low_latency = 1;
    config->realtime = 1;
}

void media_streamer_hls_config_default(HLSConfig *config)
{
    if (!config) return;
    
    memset(config, 0, sizeof(HLSConfig));
    config->segment_duration = 2;       /* 2秒 */
    config->playlist_type = 0;          /* VOD */
    config->hls_time = 2;
    config->hls_list_size = 5;
    config->hls_flags = 0;
    strcpy(config->segment_name_template, "segment_%03d.ts");
    strcpy(config->playlist_name, "playlist.m3u8");
}

void media_streamer_rtmp_config_default(RTMPConfig *config)
{
    if (!config) return;
    
    memset(config, 0, sizeof(RTMPConfig));
    config->chunk_size = 4096;
    config->buffer_size = 1024 * 1024;
    config->live = 1;
    config->publish_timeout = 5000;
}

void media_streamer_rtsp_config_default(RTSPConfig *config)
{
    if (!config) return;
    
    memset(config, 0, sizeof(RTSPConfig));
    config->transport = MEDIA_RTSP_TRANSPORT_TCP;
    config->buffer_size = 512 * 1024;
    config->timeout = 5000;
    config->stimeout = 5000000;  /* 5秒（微秒） */
}

/* ============================================================================
 * 创建与销毁
 * ============================================================================ */

MediaStreamer* media_streamer_create(const char *url, MediaStreamDirection direction)
{
    if (!url) {
        return NULL;
    }
    
    MediaStreamer *streamer = (MediaStreamer*)media_calloc(1, sizeof(MediaStreamer));
    if (!streamer) {
        return NULL;
    }
    
    /* 设置默认配置 */
    media_stream_config_default(&streamer->config);
    
    /* 复制URL */
    strncpy(streamer->url, url, sizeof(streamer->url) - 1);
    streamer->direction = direction;
    streamer->state = MEDIA_STREAMER_STATE_IDLE;
    
    return streamer;
}

void media_streamer_free(MediaStreamer *streamer)
{
    if (!streamer) {
        return;
    }
    
    media_streamer_disconnect(streamer);
    
    if (streamer->codec_ctxs) {
        media_free(streamer->codec_ctxs);
    }
    
    media_free(streamer);
}

/* ============================================================================
 * 连接与断开
 * ============================================================================ */

MediaErrorCode media_streamer_connect(MediaStreamer *streamer)
{
    if (!streamer) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    streamer->state = MEDIA_STREAMER_STATE_CONNECTING;
    
    /* 根据方向初始化 */
    if (streamer->direction == MEDIA_STREAM_DIRECTION_PULL) {
        /* 拉流：打开输入 */
        AVDictionary *opts = NULL;
        
        /* 设置选项 */
        av_dict_set(&opts, "rtsp_transport", "tcp", 0);
        av_dict_set_int(&opts, "stimeout", streamer->config.timeout * 1000, 0);
        
        int ret = avformat_open_input(&streamer->input_ctx, streamer->url, NULL, &opts);
        av_dict_free(&opts);
        
        if (ret < 0) {
            MEDIA_LOG_ERROR("Failed to open input stream: %s\n", streamer->url);
            streamer->state = MEDIA_STREAMER_STATE_ERROR;
            return MEDIA_ERROR_STREAM_OPEN_FAILED;
        }
        
        /* 获取流信息 */
        ret = avformat_find_stream_info(streamer->input_ctx, NULL);
        if (ret < 0) {
            MEDIA_LOG_ERROR("Failed to find stream info\n");
            avformat_close_input(&streamer->input_ctx);
            streamer->state = MEDIA_STREAMER_STATE_ERROR;
            return MEDIA_ERROR_STREAM_INFO;
        }
        
        MEDIA_LOG_INFO("Connected to input stream: %s\n", streamer->url);
        
    } else if (streamer->direction == MEDIA_STREAM_DIRECTION_PUSH) {
        /* 推流：打开输出 */
        int ret = avformat_alloc_output_context2(&streamer->output_ctx, NULL, NULL, streamer->url);
        if (ret < 0 || !streamer->output_ctx) {
            MEDIA_LOG_ERROR("Failed to allocate output context\n");
            streamer->state = MEDIA_STREAMER_STATE_ERROR;
            return MEDIA_ERROR_OUT_OF_MEMORY;
        }
        
        /* 打开网络IO */
        if (!(streamer->output_ctx->oformat->flags & AVFMT_NOFILE)) {
            AVDictionary *opts = NULL;
            av_dict_set_int(&opts, "timeout", streamer->config.timeout * 1000, 0);
            
            ret = avio_open2(&streamer->output_ctx->pb, streamer->url, AVIO_FLAG_WRITE, NULL, &opts);
            av_dict_free(&opts);
            
            if (ret < 0) {
                MEDIA_LOG_ERROR("Failed to open output URL: %s\n", streamer->url);
                avformat_free_context(streamer->output_ctx);
                streamer->output_ctx = NULL;
                streamer->state = MEDIA_STREAMER_STATE_ERROR;
                return MEDIA_ERROR_STREAM_OPEN_FAILED;
            }
        }
        
        MEDIA_LOG_INFO("Opened output stream: %s\n", streamer->url);
    }
    
    streamer->state = MEDIA_STREAMER_STATE_CONNECTED;
    return MEDIA_SUCCESS;
}

void media_streamer_disconnect(MediaStreamer *streamer)
{
    if (!streamer) {
        return;
    }
    
    streamer->running = 0;
    
    /* 关闭输入 */
    if (streamer->input_ctx) {
        avformat_close_input(&streamer->input_ctx);
        streamer->input_ctx = NULL;
    }
    
    /* 关闭输出 */
    if (streamer->output_ctx) {
        if (streamer->output_ctx->pb && !(streamer->output_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&streamer->output_ctx->pb);
        }
        avformat_free_context(streamer->output_ctx);
        streamer->output_ctx = NULL;
    }
    
    streamer->state = MEDIA_STREAMER_STATE_CLOSED;
}

/* ============================================================================
 * 推流
 * ============================================================================ */

MediaErrorCode media_streamer_push_frame(MediaStreamer *streamer, 
                                          MediaFrame *frame, 
                                          int32_t stream_index)
{
    if (!streamer || !frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (streamer->state != MEDIA_STREAMER_STATE_STREAMING) {
        return MEDIA_ERROR_INVALID_STATE;
    }
    
    /* 编码并发送帧 */
    /* 简化实现，实际需要完整的编码流程 */
    
    streamer->frames_sent++;
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_streamer_push_packet(MediaStreamer *streamer, 
                                           MediaPacket *packet)
{
    if (!streamer || !packet) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (streamer->state != MEDIA_STREAMER_STATE_STREAMING) {
        return MEDIA_ERROR_INVALID_STATE;
    }
    
    AVPacket *av_pkt = (AVPacket*)packet;
    
    /* 写入包 */
    int ret = av_interleaved_write_frame(streamer->output_ctx, av_pkt);
    if (ret < 0) {
        return MEDIA_ERROR_WRITE_FAILED;
    }
    
    streamer->frames_sent++;
    streamer->bytes_sent += av_pkt->size;
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 拉流
 * ============================================================================ */

MediaErrorCode media_streamer_pull_frame(MediaStreamer *streamer, 
                                          MediaFrame **frame)
{
    if (!streamer || !frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (streamer->state != MEDIA_STREAMER_STATE_STREAMING) {
        return MEDIA_ERROR_INVALID_STATE;
    }
    
    /* 读取并解码帧 */
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    int ret = av_read_frame(streamer->input_ctx, packet);
    if (ret < 0) {
        av_packet_free(&packet);
        if (ret == AVERROR_EOF) {
            return MEDIA_EOF;
        }
        return MEDIA_ERROR_READ_FAILED;
    }
    
    streamer->frames_received++;
    streamer->bytes_received += packet->size;
    
    /* 解码帧（简化实现） */
    /* 实际需要调用解码器 */
    
    av_packet_free(&packet);
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_streamer_pull_packet(MediaStreamer *streamer, 
                                           MediaPacket **packet)
{
    if (!streamer || !packet) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (streamer->state != MEDIA_STREAMER_STATE_STREAMING) {
        return MEDIA_ERROR_INVALID_STATE;
    }
    
    AVPacket *av_pkt = av_packet_alloc();
    if (!av_pkt) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    int ret = av_read_frame(streamer->input_ctx, av_pkt);
    if (ret < 0) {
        av_packet_free(&av_pkt);
        if (ret == AVERROR_EOF) {
            return MEDIA_EOF;
        }
        return MEDIA_ERROR_READ_FAILED;
    }
    
    *packet = (MediaPacket*)av_pkt;
    
    streamer->frames_received++;
    streamer->bytes_received += av_pkt->size;
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 流控制
 * ============================================================================ */

MediaErrorCode media_streamer_start(MediaStreamer *streamer)
{
    if (!streamer) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (streamer->state != MEDIA_STREAMER_STATE_CONNECTED) {
        return MEDIA_ERROR_INVALID_STATE;
    }
    
    streamer->state = MEDIA_STREAMER_STATE_STREAMING;
    streamer->running = 1;
    
    /* 如果是推流，写入文件头 */
    if (streamer->direction == MEDIA_STREAM_DIRECTION_PUSH && streamer->output_ctx) {
        int ret = avformat_write_header(streamer->output_ctx, NULL);
        if (ret < 0) {
            MEDIA_LOG_ERROR("Failed to write stream header\n");
            return MEDIA_ERROR_WRITE_FAILED;
        }
    }
    
    MEDIA_LOG_INFO("Streaming started\n");
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_streamer_stop(MediaStreamer *streamer)
{
    if (!streamer) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    streamer->running = 0;
    
    /* 如果是推流，写入文件尾 */
    if (streamer->direction == MEDIA_STREAM_DIRECTION_PUSH && streamer->output_ctx) {
        av_write_trailer(streamer->output_ctx);
    }
    
    streamer->state = MEDIA_STREAMER_STATE_CONNECTED;
    
    MEDIA_LOG_INFO("Streaming stopped\n");
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 回调设置
 * ============================================================================ */

void media_streamer_set_callback(MediaStreamer *streamer, 
                                  MediaStreamCallback callback, 
                                  void *user_data)
{
    if (!streamer) return;
    
    streamer->stream_callback = callback;
    streamer->callback_user_data = user_data;
}

/* ============================================================================
 * 状态查询
 * ============================================================================ */

MediaStreamerState media_streamer_get_state(MediaStreamer *streamer)
{
    if (!streamer) return MEDIA_STREAMER_STATE_IDLE;
    return streamer->state;
}

const char* media_streamer_get_url(MediaStreamer *streamer)
{
    if (!streamer) return NULL;
    return streamer->url;
}

int64_t media_streamer_get_bytes_sent(MediaStreamer *streamer)
{
    if (!streamer) return 0;
    return streamer->bytes_sent;
}

int64_t media_streamer_get_bytes_received(MediaStreamer *streamer)
{
    if (!streamer) return 0;
    return streamer->bytes_received;
}

int64_t media_streamer_get_frames_sent(MediaStreamer *streamer)
{
    if (!streamer) return 0;
    return streamer->frames_sent;
}

int64_t media_streamer_get_frames_received(MediaStreamer *streamer)
{
    if (!streamer) return 0;
    return streamer->frames_received;
}

int32_t media_streamer_get_bitrate(MediaStreamer *streamer)
{
    if (!streamer) return 0;
    return streamer->current_bitrate;
}

int32_t media_streamer_get_latency(MediaStreamer *streamer)
{
    if (!streamer) return 0;
    return streamer->latency_ms;
}

int64_t media_streamer_get_packets_lost(MediaStreamer *streamer)
{
    if (!streamer) return 0;
    return streamer->packets_lost;
}
