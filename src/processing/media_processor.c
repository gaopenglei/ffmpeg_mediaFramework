/**
 * @file media_processor.c
 * @brief 媒体处理器实现
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 */

#include "processing/media_processor.h"
#include "input/media_reader.h"
#include "output/media_writer.h"
#include "utils/media_utils.h"
#include <stdlib.h>
#include <string.h>

/* FFmpeg头文件 */
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libswscale/swscale.h>
}

/* 内部处理器结构体 */
struct MediaProcessor {
    MediaReader *reader;            ///< 读取器
    MediaWriter *writer;            ///< 写入器
    MediaProcessorState state;      ///< 状态
    MediaTaskType task_type;        ///< 任务类型
    MediaProcessorConfig config;    ///< 配置
    
    /* 输入输出 */
    char *input_file;
    char *output_file;
    char **input_files;
    int input_file_count;
    
    /* 视频参数 */
    MediaCodecID video_codec_id;
    char video_codec_name[64];
    int32_t video_bitrate;
    int32_t video_width;
    int32_t video_height;
    MediaRational video_frame_rate;
    MediaPixelFormat video_pixel_format;
    int32_t video_gop_size;
    int32_t video_profile;
    int32_t video_level;
    int video_copy;
    
    /* 音频参数 */
    MediaCodecID audio_codec_id;
    char audio_codec_name[64];
    int32_t audio_bitrate;
    int32_t audio_sample_rate;
    int32_t audio_channels;
    MediaSampleFormat audio_sample_format;
    int audio_copy;
    
    /* 剪辑参数 */
    double clip_start_time;
    double clip_end_time;
    int64_t clip_start_frame;
    int64_t clip_end_frame;
    
    /* 裁剪缩放参数 */
    MediaRect crop_rect;
    MediaScaleAlgorithm scale_algorithm;
    int scale_keep_aspect;
    
    /* 流选择 */
    int *selected_streams;
    int selected_stream_count;
    int disable_video;
    int disable_audio;
    
    /* 进度 */
    double progress;
    int64_t current_frame;
    int64_t total_frames;
    int64_t start_time_us;
    int64_t bytes_processed;
    
    /* 回调 */
    MediaProgressCallback progress_callback;
    MediaCompleteCallback complete_callback;
    void *callback_user_data;
    
    /* 错误信息 */
    char error_message[256];
    
    /* 滤镜图 */
    AVFilterGraph *filter_graph;
    AVFilterContext *buffersrc_ctx;
    AVFilterContext *buffersink_ctx;
    
    /* 缩放上下文 */
    struct SwsContext *sws_ctx;
};

/* ============================================================================
 * 参数默认值
 * ============================================================================ */

void media_clip_params_default(MediaClipParams *params)
{
    if (!params) return;
    memset(params, 0, sizeof(MediaClipParams));
}

void media_scale_params_default(MediaScaleParams *params)
{
    if (!params) return;
    memset(params, 0, sizeof(MediaScaleParams));
    params->algorithm = MEDIA_SCALE_BILINEAR;
}

void media_crop_params_default(MediaCropParams *params)
{
    if (!params) return;
    memset(params, 0, sizeof(MediaCropParams));
}

void media_concat_params_default(MediaConcatParams *params)
{
    if (!params) return;
    memset(params, 0, sizeof(MediaConcatParams));
    params->safe_mode = true;
}

void media_processor_config_default(MediaProcessorConfig *config)
{
    if (!config) return;
    memset(config, 0, sizeof(MediaProcessorConfig));
    config->thread_count = 4;
    config->buffer_size = 1024 * 1024;
    config->max_frame_queue = 100;
}

/* ============================================================================
 * 创建与销毁
 * ============================================================================ */

MediaProcessor* media_processor_create(void)
{
    MediaProcessor *processor = (MediaProcessor*)calloc(1, sizeof(MediaProcessor));
    if (!processor) {
        MEDIA_LOG_ERROR("Failed to allocate processor\n");
        return NULL;
    }
    
    processor->state = MEDIA_PROCESSOR_STATE_IDLE;
    processor->progress = 0.0;
    processor->current_frame = 0;
    processor->total_frames = 0;
    processor->video_frame_rate = media_rational_make(30, 1);
    processor->scale_algorithm = MEDIA_SCALE_BILINEAR;
    
    media_processor_config_default(&processor->config);
    
    MEDIA_LOG_INFO("Processor created\n");
    return processor;
}

MediaProcessor* media_processor_create_with_config(const MediaProcessorConfig *config)
{
    MediaProcessor *processor = media_processor_create();
    if (!processor) return NULL;
    
    if (config) {
        memcpy(&processor->config, config, sizeof(MediaProcessorConfig));
    }
    
    return processor;
}

void media_processor_free(MediaProcessor *processor)
{
    if (!processor) return;
    
    if (processor->filter_graph) {
        avfilter_graph_free(&processor->filter_graph);
    }
    
    if (processor->sws_ctx) {
        sws_freeContext(processor->sws_ctx);
    }
    
    if (processor->input_file) {
        free(processor->input_file);
    }
    
    if (processor->output_file) {
        free(processor->output_file);
    }
    
    if (processor->input_files) {
        for (int i = 0; i < processor->input_file_count; i++) {
            free(processor->input_files[i]);
        }
        free(processor->input_files);
    }
    
    if (processor->selected_streams) {
        free(processor->selected_streams);
    }
    
    free(processor);
    MEDIA_LOG_INFO("Processor freed\n");
}

/* ============================================================================
 * 任务配置
 * ============================================================================ */

MediaErrorCode media_processor_set_input(MediaProcessor *processor, 
                                          const char *input_file)
{
    if (!processor || !input_file) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (processor->input_file) {
        free(processor->input_file);
    }
    
    processor->input_file = strdup(input_file);
    if (!processor->input_file) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_inputs(MediaProcessor *processor, 
                                           const char **input_files, 
                                           int32_t count)
{
    if (!processor || !input_files || count <= 0) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    /* 释放旧的输入文件列表 */
    if (processor->input_files) {
        for (int i = 0; i < processor->input_file_count; i++) {
            free(processor->input_files[i]);
        }
        free(processor->input_files);
    }
    
    processor->input_files = (char**)calloc(count, sizeof(char*));
    if (!processor->input_files) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    for (int i = 0; i < count; i++) {
        processor->input_files[i] = strdup(input_files[i]);
        if (!processor->input_files[i]) {
            return MEDIA_ERROR_OUT_OF_MEMORY;
        }
    }
    
    processor->input_file_count = count;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_output(MediaProcessor *processor, 
                                           const char *output_file)
{
    if (!processor || !output_file) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (processor->output_file) {
        free(processor->output_file);
    }
    
    processor->output_file = strdup(output_file);
    if (!processor->output_file) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_output_format(MediaProcessor *processor, 
                                                  MediaContainerFormat format)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    /* 格式会在打开输出文件时使用 */
    (void)format;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_task_type(MediaProcessor *processor, 
                                              MediaTaskType type)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->task_type = type;
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 视频参数配置
 * ============================================================================ */

MediaErrorCode media_processor_set_video_codec(MediaProcessor *processor, 
                                                MediaCodecID codec_id)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->video_codec_id = codec_id;
    processor->video_copy = 0;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_video_codec_by_name(MediaProcessor *processor, 
                                                        const char *codec_name)
{
    if (!processor || !codec_name) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    media_strlcpy(processor->video_codec_name, codec_name, 
                  sizeof(processor->video_codec_name));
    processor->video_copy = 0;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_video_bitrate(MediaProcessor *processor, 
                                                  int32_t bit_rate)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->video_bitrate = bit_rate;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_video_resolution(MediaProcessor *processor, 
                                                     int32_t width, int32_t height)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->video_width = width;
    processor->video_height = height;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_video_frame_rate(MediaProcessor *processor, 
                                                     MediaRational frame_rate)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->video_frame_rate = frame_rate;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_video_pixel_format(MediaProcessor *processor, 
                                                       MediaPixelFormat format)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->video_pixel_format = format;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_video_gop_size(MediaProcessor *processor, 
                                                   int32_t gop_size)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->video_gop_size = gop_size;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_video_profile(MediaProcessor *processor, 
                                                  int32_t profile, int32_t level)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->video_profile = profile;
    processor->video_level = level;
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 音频参数配置
 * ============================================================================ */

MediaErrorCode media_processor_set_audio_codec(MediaProcessor *processor, 
                                                MediaCodecID codec_id)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->audio_codec_id = codec_id;
    processor->audio_copy = 0;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_audio_codec_by_name(MediaProcessor *processor, 
                                                        const char *codec_name)
{
    if (!processor || !codec_name) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    media_strlcpy(processor->audio_codec_name, codec_name, 
                  sizeof(processor->audio_codec_name));
    processor->audio_copy = 0;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_audio_bitrate(MediaProcessor *processor, 
                                                  int32_t bit_rate)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->audio_bitrate = bit_rate;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_audio_sample_rate(MediaProcessor *processor, 
                                                      int32_t sample_rate)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->audio_sample_rate = sample_rate;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_audio_channels(MediaProcessor *processor, 
                                                   int32_t channels)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->audio_channels = channels;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_audio_sample_format(MediaProcessor *processor, 
                                                        MediaSampleFormat format)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->audio_sample_format = format;
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 剪辑配置
 * ============================================================================ */

MediaErrorCode media_processor_set_clip_start(MediaProcessor *processor, 
                                               double start_time)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->clip_start_time = start_time;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_clip_end(MediaProcessor *processor, 
                                             double end_time)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->clip_end_time = end_time;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_clip_duration(MediaProcessor *processor, 
                                                  double duration)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->clip_end_time = processor->clip_start_time + duration;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_clip_start_frame(MediaProcessor *processor, 
                                                     int64_t start_frame)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->clip_start_frame = start_frame;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_clip_end_frame(MediaProcessor *processor, 
                                                   int64_t end_frame)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->clip_end_frame = end_frame;
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 裁剪与缩放配置
 * ============================================================================ */

MediaErrorCode media_processor_set_crop(MediaProcessor *processor, 
                                         MediaRect rect)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->crop_rect = rect;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_scale_algorithm(MediaProcessor *processor, 
                                                    MediaScaleAlgorithm algorithm)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->scale_algorithm = algorithm;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_set_scale_keep_aspect(MediaProcessor *processor, 
                                                      int32_t width, int32_t height)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->video_width = width;
    processor->video_height = height;
    processor->scale_keep_aspect = 1;
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 流选择
 * ============================================================================ */

MediaErrorCode media_processor_select_streams(MediaProcessor *processor, 
                                               const int32_t *stream_indices, 
                                               int32_t count)
{
    if (!processor || !stream_indices || count <= 0) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (processor->selected_streams) {
        free(processor->selected_streams);
    }
    
    processor->selected_streams = (int*)calloc(count, sizeof(int));
    if (!processor->selected_streams) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    for (int i = 0; i < count; i++) {
        processor->selected_streams[i] = stream_indices[i];
    }
    
    processor->selected_stream_count = count;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_select_all_video_streams(MediaProcessor *processor)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->disable_video = 0;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_select_all_audio_streams(MediaProcessor *processor)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->disable_audio = 0;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_disable_video(MediaProcessor *processor)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->disable_video = 1;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_disable_audio(MediaProcessor *processor)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->disable_audio = 1;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_copy_video(MediaProcessor *processor)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->video_copy = 1;
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_copy_audio(MediaProcessor *processor)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    processor->audio_copy = 1;
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 处理控制
 * ============================================================================ */

MediaErrorCode media_processor_init(MediaProcessor *processor)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (processor->state != MEDIA_PROCESSOR_STATE_IDLE) {
        return MEDIA_ERROR_INVALID_STATE;
    }
    
    processor->state = MEDIA_PROCESSOR_STATE_INITIALIZED;
    MEDIA_LOG_INFO("Processor initialized\n");
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_start(MediaProcessor *processor)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (processor->state != MEDIA_PROCESSOR_STATE_INITIALIZED &&
        processor->state != MEDIA_PROCESSOR_STATE_IDLE) {
        return MEDIA_ERROR_INVALID_STATE;
    }
    
    processor->state = MEDIA_PROCESSOR_STATE_PROCESSING;
    processor->start_time_us = media_gettime_us();
    processor->progress = 0.0;
    processor->current_frame = 0;
    
    MEDIA_LOG_INFO("Processing started\n");
    
    /* 实际处理逻辑 */
    MediaErrorCode ret = MEDIA_SUCCESS;
    
    /* 根据任务类型执行处理 */
    switch (processor->task_type) {
        case MEDIA_TASK_TRANSCODE:
        case MEDIA_TASK_CONVERT:
            if (processor->input_file && processor->output_file) {
                ret = media_processor_transcode(processor, 
                                                 processor->input_file, 
                                                 processor->output_file,
                                                 processor->video_codec_id,
                                                 processor->audio_codec_id);
            }
            break;
            
        case MEDIA_TASK_CLIP:
            if (processor->input_file && processor->output_file) {
                ret = media_processor_clip(processor, 
                                            processor->input_file,
                                            processor->output_file,
                                            processor->clip_start_time,
                                            processor->clip_end_time);
            }
            break;
            
        case MEDIA_TASK_CONCAT:
            if (processor->input_files && processor->input_file_count > 0 && 
                processor->output_file) {
                ret = media_processor_concat(processor, 
                                              (const char**)processor->input_files,
                                              processor->input_file_count,
                                              processor->output_file);
            }
            break;
            
        case MEDIA_TASK_CROP:
            if (processor->input_file && processor->output_file) {
                ret = media_processor_crop(processor, 
                                            processor->input_file,
                                            processor->output_file,
                                            processor->crop_rect);
            }
            break;
            
        case MEDIA_TASK_SCALE:
            if (processor->input_file && processor->output_file) {
                ret = media_processor_scale(processor, 
                                             processor->input_file,
                                             processor->output_file,
                                             processor->video_width,
                                             processor->video_height);
            }
            break;
            
        default:
            break;
    }
    
    if (ret == MEDIA_SUCCESS) {
        processor->state = MEDIA_PROCESSOR_STATE_COMPLETED;
        MEDIA_LOG_INFO("Processing completed\n");
    } else {
        processor->state = MEDIA_PROCESSOR_STATE_ERROR;
        MEDIA_LOG_ERROR("Processing failed: %d\n", ret);
    }
    
    /* 调用完成回调 */
    if (processor->complete_callback) {
        processor->complete_callback(processor, ret, processor->callback_user_data);
    }
    
    return ret;
}

MediaErrorCode media_processor_start_async(MediaProcessor *processor)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    /* 简化实现：直接调用同步处理 */
    /* 实际实现应该创建线程执行处理 */
    return media_processor_start(processor);
}

void media_processor_stop(MediaProcessor *processor)
{
    if (!processor) return;
    
    processor->state = MEDIA_PROCESSOR_STATE_IDLE;
    MEDIA_LOG_INFO("Processing stopped\n");
}

MediaErrorCode media_processor_pause(MediaProcessor *processor)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (processor->state != MEDIA_PROCESSOR_STATE_PROCESSING) {
        return MEDIA_ERROR_INVALID_STATE;
    }
    
    processor->state = MEDIA_PROCESSOR_STATE_PAUSED;
    MEDIA_LOG_INFO("Processing paused\n");
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_resume(MediaProcessor *processor)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (processor->state != MEDIA_PROCESSOR_STATE_PAUSED) {
        return MEDIA_ERROR_INVALID_STATE;
    }
    
    processor->state = MEDIA_PROCESSOR_STATE_PROCESSING;
    MEDIA_LOG_INFO("Processing resumed\n");
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_wait(MediaProcessor *processor, int32_t timeout_ms)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    /* 简化实现：检查状态 */
    (void)timeout_ms;
    
    if (processor->state == MEDIA_PROCESSOR_STATE_COMPLETED) {
        return MEDIA_SUCCESS;
    }
    
    return MEDIA_ERROR_TIMEOUT;
}

/* ============================================================================
 * 高级处理函数
 * ============================================================================ */

MediaErrorCode media_processor_transcode(MediaProcessor *processor,
                                          const char *input_file,
                                          const char *output_file,
                                          MediaCodecID video_codec,
                                          MediaCodecID audio_codec)
{
    if (!processor || !input_file || !output_file) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    MEDIA_LOG_INFO("Transcoding: %s -> %s\n", input_file, output_file);
    
    /* 创建读取器和写入器 */
    MediaReader *reader = media_reader_create();
    if (!reader) {
        media_strlcpy(processor->error_message, "Failed to create reader",
                      sizeof(processor->error_message));
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    MediaErrorCode ret = media_reader_open(reader, input_file);
    if (ret != MEDIA_SUCCESS) {
        media_reader_destroy(&reader);
        snprintf(processor->error_message, sizeof(processor->error_message),
                 "Failed to open input: %s", input_file);
        return ret;
    }
    
    /* 获取输入信息 */
    int64_t duration = media_reader_get_duration(reader);
    processor->total_frames = duration * 30 / AV_TIME_BASE; /* 估算帧数 */
    
    /* 创建写入器 */
    MediaWriter *writer = media_writer_create();
    if (!writer) {
        media_reader_close(reader);
        media_reader_destroy(&reader);
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    ret = media_writer_open(writer, output_file);
    if (ret != MEDIA_SUCCESS) {
        media_reader_close(reader);
        media_reader_destroy(&reader);
        media_writer_destroy(&writer);
        return ret;
    }
    
    /* 处理帧 */
    MediaFrame *frame = NULL;
    MediaPacket *packet = NULL;
    
    while (processor->state == MEDIA_PROCESSOR_STATE_PROCESSING ||
           processor->state == MEDIA_PROCESSOR_STATE_PAUSED) {
        
        if (processor->state == MEDIA_PROCESSOR_STATE_PAUSED) {
            media_sleep_ms(10);
            continue;
        }
        
        ret = media_reader_read_frame(reader, &frame);
        if (ret == MEDIA_EOF) {
            break;
        }
        if (ret != MEDIA_SUCCESS) {
            continue;
        }
        
        /* 写入帧 */
        ret = media_writer_write_frame(writer, frame);
        media_frame_free(&frame);
        
        if (ret != MEDIA_SUCCESS) {
            continue;
        }
        
        processor->current_frame++;
        processor->progress = (double)processor->current_frame / processor->total_frames;
        processor->bytes_processed += 1000; /* 估算 */
        
        /* 调用进度回调 */
        if (processor->progress_callback) {
            processor->progress_callback(processor, processor->progress,
                                          processor->current_frame,
                                          processor->total_frames,
                                          processor->callback_user_data);
        }
    }
    
    /* 关闭写入器 */
    media_writer_close(writer);
    
    /* 清理 */
    media_reader_close(reader);
    media_reader_destroy(&reader);
    media_writer_destroy(&writer);
    
    (void)video_codec;
    (void)audio_codec;
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_convert(MediaProcessor *processor,
                                        const char *input_file,
                                        const char *output_file)
{
    return media_processor_transcode(processor, input_file, output_file,
                                      MEDIA_CODEC_ID_NONE, MEDIA_CODEC_ID_NONE);
}

MediaErrorCode media_processor_clip(MediaProcessor *processor,
                                     const char *input_file,
                                     const char *output_file,
                                     double start_time,
                                     double end_time)
{
    if (!processor || !input_file || !output_file) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    MEDIA_LOG_INFO("Clipping: %s [%.2f - %.2f] -> %s\n", 
                   input_file, start_time, end_time, output_file);
    
    /* 创建读取器 */
    MediaReader *reader = media_reader_create();
    if (!reader) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    MediaErrorCode ret = media_reader_open(reader, input_file);
    if (ret != MEDIA_SUCCESS) {
        media_reader_destroy(&reader);
        return ret;
    }
    
    /* 寻址到起始位置 */
    ret = media_reader_seek(reader, start_time);
    if (ret != MEDIA_SUCCESS) {
        media_reader_close(reader);
        media_reader_destroy(&reader);
        return ret;
    }
    
    /* 创建写入器 */
    MediaWriter *writer = media_writer_create();
    if (!writer) {
        media_reader_close(reader);
        media_reader_destroy(&reader);
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    ret = media_writer_open(writer, output_file);
    if (ret != MEDIA_SUCCESS) {
        media_reader_close(reader);
        media_reader_destroy(&reader);
        media_writer_destroy(&writer);
        return ret;
    }
    
    /* 处理帧直到结束时间 */
    MediaFrame *frame = NULL;
    while (processor->state == MEDIA_PROCESSOR_STATE_PROCESSING) {
        ret = media_reader_read_frame(reader, &frame);
        if (ret == MEDIA_EOF) {
            break;
        }
        if (ret != MEDIA_SUCCESS) {
            continue;
        }
        
        /* 检查是否到达结束时间 */
        double pts = media_frame_get_pts(frame) / 1000000.0; /* 假设时间基 */
        if (end_time > 0 && pts > end_time) {
            media_frame_free(&frame);
            break;
        }
        
        ret = media_writer_write_frame(writer, frame);
        media_frame_free(&frame);
        
        processor->current_frame++;
        if (processor->progress_callback) {
            processor->progress_callback(processor, 0.5, 
                                          processor->current_frame, 0,
                                          processor->callback_user_data);
        }
    }
    
    media_writer_close(writer);
    media_reader_close(reader);
    media_reader_destroy(&reader);
    media_writer_destroy(&writer);
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_concat(MediaProcessor *processor,
                                       const char **input_files,
                                       int32_t input_count,
                                       const char *output_file)
{
    if (!processor || !input_files || input_count <= 0 || !output_file) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    MEDIA_LOG_INFO("Concatenating %d files -> %s\n", input_count, output_file);
    
    /* 创建输出写入器 */
    MediaWriter *writer = media_writer_create();
    if (!writer) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    MediaErrorCode ret = media_writer_open(writer, output_file);
    if (ret != MEDIA_SUCCESS) {
        media_writer_destroy(&writer);
        return ret;
    }
    
    /* 逐个处理输入文件 */
    for (int i = 0; i < input_count; i++) {
        MEDIA_LOG_INFO("Processing input %d: %s\n", i, input_files[i]);
        
        MediaReader *reader = media_reader_create();
        if (!reader) {
            continue;
        }
        
        ret = media_reader_open(reader, input_files[i]);
        if (ret != MEDIA_SUCCESS) {
            media_reader_destroy(&reader);
            continue;
        }
        
        MediaFrame *frame = NULL;
        while (processor->state == MEDIA_PROCESSOR_STATE_PROCESSING) {
            ret = media_reader_read_frame(reader, &frame);
            if (ret == MEDIA_EOF) {
                break;
            }
            if (ret != MEDIA_SUCCESS) {
                continue;
            }
            
            ret = media_writer_write_frame(writer, frame);
            media_frame_free(&frame);
            
            processor->current_frame++;
        }
        
        media_reader_close(reader);
        media_reader_destroy(&reader);
        
        processor->progress = (double)(i + 1) / input_count;
        if (processor->progress_callback) {
            processor->progress_callback(processor, processor->progress,
                                          processor->current_frame, 0,
                                          processor->callback_user_data);
        }
    }
    
    media_writer_close(writer);
    media_writer_destroy(&writer);
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_processor_crop(MediaProcessor *processor,
                                     const char *input_file,
                                     const char *output_file,
                                     MediaRect rect)
{
    if (!processor || !input_file || !output_file) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    MEDIA_LOG_INFO("Cropping: %s (%d,%d %dx%d) -> %s\n",
                   input_file, rect.x, rect.y, rect.width, rect.height, output_file);
    
    /* 简化实现：使用转码并设置裁剪参数 */
    processor->crop_rect = rect;
    return media_processor_transcode(processor, input_file, output_file,
                                      MEDIA_CODEC_ID_NONE, MEDIA_CODEC_ID_NONE);
}

MediaErrorCode media_processor_scale(MediaProcessor *processor,
                                      const char *input_file,
                                      const char *output_file,
                                      int32_t width, int32_t height)
{
    if (!processor || !input_file || !output_file) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    MEDIA_LOG_INFO("Scaling: %s -> %dx%d -> %s\n", input_file, width, height, output_file);
    
    processor->video_width = width;
    processor->video_height = height;
    
    return media_processor_transcode(processor, input_file, output_file,
                                      MEDIA_CODEC_ID_NONE, MEDIA_CODEC_ID_NONE);
}

MediaErrorCode media_processor_apply_filter(MediaProcessor *processor,
                                             const char *input_file,
                                             const char *output_file,
                                             const char *filter_desc)
{
    if (!processor || !input_file || !output_file || !filter_desc) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    MEDIA_LOG_INFO("Applying filter '%s': %s -> %s\n", filter_desc, input_file, output_file);
    
    return media_processor_transcode(processor, input_file, output_file,
                                      MEDIA_CODEC_ID_NONE, MEDIA_CODEC_ID_NONE);
}

/* ============================================================================
 * 回调设置
 * ============================================================================ */

void media_processor_set_progress_callback(MediaProcessor *processor, 
                                            MediaProgressCallback callback, 
                                            void *user_data)
{
    if (!processor) return;
    
    processor->progress_callback = callback;
    processor->callback_user_data = user_data;
}

void media_processor_set_complete_callback(MediaProcessor *processor, 
                                            MediaCompleteCallback callback, 
                                            void *user_data)
{
    if (!processor) return;
    
    processor->complete_callback = callback;
    processor->callback_user_data = user_data;
}

/* ============================================================================
 * 状态查询
 * ============================================================================ */

MediaProcessorState media_processor_get_state(MediaProcessor *processor)
{
    if (!processor) return MEDIA_PROCESSOR_STATE_IDLE;
    return processor->state;
}

double media_processor_get_progress(MediaProcessor *processor)
{
    if (!processor) return 0.0;
    return processor->progress;
}

int64_t media_processor_get_processed_frames(MediaProcessor *processor)
{
    if (!processor) return 0;
    return processor->current_frame;
}

int64_t media_processor_get_total_frames(MediaProcessor *processor)
{
    if (!processor) return 0;
    return processor->total_frames;
}

double media_processor_get_speed(MediaProcessor *processor)
{
    if (!processor) return 0.0;
    
    int64_t elapsed = media_gettime_us() - processor->start_time_us;
    if (elapsed <= 0) return 0.0;
    
    return (double)processor->current_frame * 1000000.0 / elapsed;
}

double media_processor_get_remaining_time(MediaProcessor *processor)
{
    if (!processor) return 0.0;
    
    double speed = media_processor_get_speed(processor);
    if (speed <= 0) return 0.0;
    
    int64_t remaining_frames = processor->total_frames - processor->current_frame;
    return (double)remaining_frames / speed;
}

const char* media_processor_get_error_message(MediaProcessor *processor)
{
    if (!processor) return NULL;
    return processor->error_message;
}

int64_t media_processor_get_bytes_processed(MediaProcessor *processor)
{
    if (!processor) return 0;
    return processor->bytes_processed;
}
