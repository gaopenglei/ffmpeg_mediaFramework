/**
 * @file media_advanced.c
 * @brief 高级功能实现
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 */

#include "processing/media_advanced.h"
#include "utils/media_utils.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* FFmpeg头文件 */
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

/* ============================================================================
 * 多声道处理
 * ============================================================================ */

const char* media_channel_layout_name(int32_t layout)
{
    switch (layout) {
        case MEDIA_CH_LAYOUT_MONO:          return "mono";
        case MEDIA_CH_LAYOUT_STEREO:        return "stereo";
        case MEDIA_CH_LAYOUT_2POINT1:       return "2.1";
        case MEDIA_CH_LAYOUT_4POINT0:       return "4.0";
        case MEDIA_CH_LAYOUT_4POINT1:       return "4.1";
        case MEDIA_CH_LAYOUT_5POINT0:       return "5.0";
        case MEDIA_CH_LAYOUT_5POINT1:       return "5.1";
        case MEDIA_CH_LAYOUT_6POINT0:       return "6.0";
        case MEDIA_CH_LAYOUT_6POINT1:       return "6.1";
        case MEDIA_CH_LAYOUT_7POINT0:       return "7.0";
        case MEDIA_CH_LAYOUT_7POINT1:       return "7.1";
        case MEDIA_CH_LAYOUT_7POINT1_WIDE:  return "7.1(wide)";
        default:                            return "unknown";
    }
}

int32_t media_channel_layout_channels(int32_t layout)
{
    int count = 0;
    while (layout) {
        count += layout & 1;
        layout >>= 1;
    }
    return count;
}

int32_t media_channel_layout_from_mask(uint64_t mask)
{
    return (int32_t)mask;
}

int32_t media_channel_layout_from_count(int32_t count)
{
    switch (count) {
        case 1: return MEDIA_CH_LAYOUT_MONO;
        case 2: return MEDIA_CH_LAYOUT_STEREO;
        case 3: return MEDIA_CH_LAYOUT_2POINT1;
        case 4: return MEDIA_CH_LAYOUT_4POINT0;
        case 5: return MEDIA_CH_LAYOUT_5POINT0;
        case 6: return MEDIA_CH_LAYOUT_5POINT1;
        case 7: return MEDIA_CH_LAYOUT_6POINT1;
        case 8: return MEDIA_CH_LAYOUT_7POINT1;
        default: return MEDIA_CH_LAYOUT_STEREO;
    }
}

MediaErrorCode media_channel_extract(MediaFrame *frame, 
                                      int32_t channel_index,
                                      MediaFrame **output)
{
    if (!frame || !output) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    /* 创建单声道输出帧 */
    *output = media_frame_create_audio(frame->nb_samples, 
                                        frame->format, 1, frame->sample_rate);
    if (!*output) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    /* 提取指定声道数据 */
    int data_size = av_get_bytes_per_sample((enum AVSampleFormat)frame->format);
    if (data_size <= 0) {
        media_frame_free(output);
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    for (int i = 0; i < frame->nb_samples; i++) {
        memcpy((*output)->data[0] + i * data_size,
               frame->data[channel_index] + i * data_size,
               data_size);
    }
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_channel_remix(MediaFrame *frame,
                                    int32_t input_layout,
                                    int32_t output_layout,
                                    MediaFrame **output)
{
    if (!frame || !output) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    int in_channels = media_channel_layout_channels(input_layout);
    int out_channels = media_channel_layout_channels(output_layout);
    
    /* 创建输出帧 */
    *output = media_frame_create_audio(frame->nb_samples,
                                        frame->format, out_channels, frame->sample_rate);
    if (!*output) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    /* 简单的声道混合实现 */
    if (in_channels == 2 && out_channels == 1) {
        /* 立体声转单声道 */
        int data_size = av_get_bytes_per_sample((enum AVSampleFormat)frame->format);
        for (int i = 0; i < frame->nb_samples; i++) {
            for (int j = 0; j < data_size; j++) {
                (*output)->data[0][i * data_size + j] = 
                    (frame->data[0][i * data_size + j] + 
                     frame->data[1][i * data_size + j]) / 2;
            }
        }
    } else if (in_channels == 1 && out_channels == 2) {
        /* 单声道转立体声 */
        int data_size = av_get_bytes_per_sample((enum AVSampleFormat)frame->format);
        memcpy((*output)->data[0], frame->data[0], frame->nb_samples * data_size);
        memcpy((*output)->data[1], frame->data[0], frame->nb_samples * data_size);
    } else {
        /* 其他情况：直接复制或填充 */
        int copy_channels = (in_channels < out_channels) ? in_channels : out_channels;
        int data_size = av_get_bytes_per_sample((enum AVSampleFormat)frame->format);
        
        for (int ch = 0; ch < copy_channels; ch++) {
            memcpy((*output)->data[ch], frame->data[ch], frame->nb_samples * data_size);
        }
        
        /* 填充剩余声道 */
        for (int ch = copy_channels; ch < out_channels; ch++) {
            memset((*output)->data[ch], 0, frame->nb_samples * data_size);
        }
    }
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 音频混合
 * ============================================================================ */

AudioMixer* audio_mixer_create(int32_t sample_rate, 
                                MediaSampleFormat format,
                                int32_t channels)
{
    AudioMixer *mixer = (AudioMixer*)calloc(1, sizeof(AudioMixer));
    if (!mixer) {
        return NULL;
    }
    
    mixer->sample_rate = sample_rate;
    mixer->format = format;
    mixer->channels = channels;
    mixer->track_count = 0;
    mixer->max_tracks = 16;
    mixer->tracks = (AudioTrack*)calloc(mixer->max_tracks, sizeof(AudioTrack));
    
    if (!mixer->tracks) {
        free(mixer);
        return NULL;
    }
    
    return mixer;
}

void audio_mixer_destroy(AudioMixer **mixer)
{
    if (!mixer || !*mixer) return;
    
    AudioMixer *m = *mixer;
    
    if (m->tracks) {
        free(m->tracks);
    }
    
    free(m);
    *mixer = NULL;
}

MediaErrorCode audio_mixer_add_track(AudioMixer *mixer,
                                      MediaFrame *frame,
                                      double volume,
                                      double start_time,
                                      int32_t *track_index)
{
    if (!mixer || !frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (mixer->track_count >= mixer->max_tracks) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    AudioTrack *track = &mixer->tracks[mixer->track_count];
    track->frame = frame;
    track->volume = volume;
    track->start_time = start_time;
    track->enabled = true;
    
    if (track_index) {
        *track_index = mixer->track_count;
    }
    
    mixer->track_count++;
    return MEDIA_SUCCESS;
}

MediaErrorCode audio_mixer_remove_track(AudioMixer *mixer, int32_t track_index)
{
    if (!mixer) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (track_index < 0 || track_index >= mixer->track_count) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    /* 移动后续轨道 */
    for (int i = track_index; i < mixer->track_count - 1; i++) {
        mixer->tracks[i] = mixer->tracks[i + 1];
    }
    
    mixer->track_count--;
    return MEDIA_SUCCESS;
}

MediaErrorCode audio_mixer_set_volume(AudioMixer *mixer,
                                       int32_t track_index,
                                       double volume)
{
    if (!mixer) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (track_index < 0 || track_index >= mixer->track_count) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    mixer->tracks[track_index].volume = volume;
    return MEDIA_SUCCESS;
}

MediaErrorCode audio_mixer_mix(AudioMixer *mixer, MediaFrame **output)
{
    if (!mixer || !output) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    if (mixer->track_count == 0) {
        return MEDIA_ERROR_INVALID_PARAM;
    }
    
    /* 计算输出帧长度 */
    int max_samples = 0;
    for (int i = 0; i < mixer->track_count; i++) {
        AudioTrack *track = &mixer->tracks[i];
        int samples = track->frame->nb_samples + 
                      (int)(track->start_time * mixer->sample_rate);
        if (samples > max_samples) {
            max_samples = samples;
        }
    }
    
    /* 创建输出帧 */
    *output = media_frame_create_audio(max_samples, mixer->format,
                                        mixer->channels, mixer->sample_rate);
    if (!*output) {
        return MEDIA_ERROR_OUT_OF_MEMORY;
    }
    
    /* 初始化为静音 */
    int data_size = av_get_bytes_per_sample((enum AVSampleFormat)mixer->format);
    for (int ch = 0; ch < mixer->channels; ch++) {
        memset((*output)->data[ch], 0, max_samples * data_size);
    }
    
    /* 混合所有轨道 */
    for (int i = 0; i < mixer->track_count; i++) {
        AudioTrack *track = &mixer->tracks[i];
        if (!track->enabled) continue;
        
        int offset = (int)(track->start_time * mixer->sample_rate);
        
        /* 简单的加法混合（实际应用中需要更复杂的处理） */
        for (int ch = 0; ch < mixer->channels && ch < track->frame->channels; ch++) {
            for (int s = 0; s < track->frame->nb_samples && s + offset < max_samples; s++) {
                /* 假设是16位有符号整数 */
                if (mixer->format == MEDIA_SAMPLE_FORMAT_S16) {
                    int16_t *out = (int16_t*)(*output)->data[ch];
                    int16_t *in = (int16_t*)track->frame->data[ch];
                    int mixed = out[s + offset] + (int)(in[s] * track->volume);
                    /* 防止溢出 */
                    if (mixed > 32767) mixed = 32767;
                    if (mixed < -32768) mixed = -32768;
                    out[s + offset] = (int16_t)mixed;
                }
            }
        }
    }
    
    return MEDIA_SUCCESS;
}

/* ============================================================================
 * 360°视频处理
 * ============================================================================ */

const char* media_projection_format_name(MediaProjectionFormat format)
{
    switch (format) {
        case MEDIA_PROJECTION_EQUIRECTANGULAR: return "equirectangular";
        case MEDIA_PROJECTION_CUBEMAP:         return "cubemap";
        case MEDIA_PROJECTION_EQUIRECTANGULAR_3D: return "equirectangular_3d";
        default: return "unknown";
    }
}

void media_video360_config_default(Video360Config *config)
{
    if (!config) return;
    memset(config, 0, sizeof(Video360Config));
    config->projection = MEDIA_PROJECTION_EQUIRECTANGULAR;
    config->yaw = 0.0;
    config->pitch = 0.0;
    config->roll = 0.0;
    config->fov = 180.0;
    config->width = 3840;
    config->height = 1920;
}

void media_video360_metadata_default(Video360Metadata *metadata)
{
    if (!metadata) return;
    memset(metadata, 0, sizeof(Video360Metadata));
    metadata->projection = MEDIA_PROJECTION_EQUIRECTANGULAR;
    metadata->stereo_mode = MEDIA_STEREO_MODE_NONE;
    metadata->yaw = 0.0;
    metadata->pitch = 0.0;
    metadata->roll = 0.0;
}

void media_video360_hotspot_default(Video360Hotspot *hotspot)
{
    if (!hotspot) return;
    memset(hotspot, 0, sizeof(Video360Hotspot));
    hotspot->yaw = 0.0;
    hotspot->pitch = 0.0;
    hotspot->radius = 10.0;
    hotspot->color = 0xFFFFFFFF;
    hotspot->opacity = 1.0;
}

void* media_video360_processor_create(const Video360Config *config)
{
    Video360Config *processor = (Video360Config*)malloc(sizeof(Video360Config));
    if (!processor) return NULL;
    
    if (config) {
        *processor = *config;
    } else {
        media_video360_config_default(processor);
    }
    
    return processor;
}

void media_video360_processor_destroy(void **processor)
{
    if (!processor || !*processor) return;
    free(*processor);
    *processor = NULL;
}

MediaErrorCode media_video360_set_view(void *processor,
                                        double yaw, double pitch, double roll)
{
    if (!processor) return MEDIA_ERROR_NULL_POINTER;
    
    Video360Config *config = (Video360Config*)processor;
    config->yaw = yaw;
    config->pitch = pitch;
    config->roll = roll;
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_video360_set_fov(void *processor, double fov)
{
    if (!processor) return MEDIA_ERROR_NULL_POINTER;
    
    Video360Config *config = (Video360Config*)processor;
    config->fov = fov;
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_video360_convert_projection(MediaFrame *frame,
                                                  MediaProjectionFormat src_format,
                                                  MediaProjectionFormat dst_format)
{
    if (!frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    /* 投影转换需要复杂的几何变换，这里提供框架 */
    MEDIA_LOG_INFO("Converting projection from %s to %s\n",
                   media_projection_format_name(src_format),
                   media_projection_format_name(dst_format));
    
    /* 实际实现需要使用专门的投影转换算法 */
    switch (src_format) {
        case MEDIA_PROJECTION_EQUIRECTANGULAR:
            if (dst_format == MEDIA_PROJECTION_CUBEMAP) {
                /* Equirectangular到Cubemap转换 */
                /* 需要实现具体的转换算法 */
            }
            break;
            
        case MEDIA_PROJECTION_CUBEMAP:
            if (dst_format == MEDIA_PROJECTION_EQUIRECTANGULAR) {
                /* Cubemap到Equirectangular转换 */
            }
            break;
            
        default:
            return MEDIA_ERROR_NOT_SUPPORTED;
    }
    
    return MEDIA_SUCCESS;
}

MediaErrorCode media_video360_add_hotspot(void *processor,
                                           const Video360Hotspot *hotspot,
                                           const char *hotspot_id)
{
    if (!processor || !hotspot) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    MEDIA_LOG_INFO("Adding hotspot at yaw=%.2f, pitch=%.2f\n",
                   hotspot->yaw, hotspot->pitch);
    
    /* 实际实现需要维护热点列表 */
    return MEDIA_SUCCESS;
}

MediaErrorCode media_video360_remove_hotspot(void *processor,
                                              const char *hotspot_id)
{
    if (!processor || !hotspot_id) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    MEDIA_LOG_INFO("Removing hotspot: %s\n", hotspot_id);
    return MEDIA_SUCCESS;
}

MediaErrorCode media_video360_render_hotspots(void *processor,
                                               MediaFrame *frame,
                                               int64_t timestamp)
{
    if (!processor || !frame) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    /* 渲染热点到帧上 */
    /* 实际实现需要绘制热点图形 */
    return MEDIA_SUCCESS;
}

MediaErrorCode media_video360_add_guide_line(void *processor,
                                              double start_yaw, double start_pitch,
                                              double end_yaw, double end_pitch,
                                              uint32_t color, int32_t width)
{
    if (!processor) {
        return MEDIA_ERROR_NULL_POINTER;
    }
    
    MEDIA_LOG_INFO("Adding guide line from (%.2f, %.2f) to (%.2f, %.2f)\n",
                   start_yaw, start_pitch, end_yaw, end_pitch);
    
    return MEDIA_SUCCESS;
}
