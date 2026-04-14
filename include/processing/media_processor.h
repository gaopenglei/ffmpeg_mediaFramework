/**
 * @file media_processor.h
 * @brief 媒体处理器接口
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 本文件定义了媒体处理的核心接口，包括格式转换、剪辑、拼接等功能。
 */

#ifndef MEDIA_PROCESSOR_H
#define MEDIA_PROCESSOR_H

#include "core/media_types.h"
#include "core/media_frame.h"
#include "core/media_packet.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 处理器结构体（不透明指针）
 */
typedef struct MediaProcessor MediaProcessor;

/**
 * @brief 处理器状态
 */
typedef enum {
    MEDIA_PROCESSOR_STATE_IDLE = 0,
    MEDIA_PROCESSOR_STATE_INITIALIZED = 1,
    MEDIA_PROCESSOR_STATE_PROCESSING = 2,
    MEDIA_PROCESSOR_STATE_PAUSED = 3,
    MEDIA_PROCESSOR_STATE_COMPLETED = 4,
    MEDIA_PROCESSOR_STATE_ERROR = 5,
} MediaProcessorState;

/**
 * @brief 处理任务类型
 */
typedef enum {
    MEDIA_TASK_TRANSCODE = 1,      ///< 转码
    MEDIA_TASK_CONVERT = 2,        ///< 格式转换
    MEDIA_TASK_CLIP = 3,           ///< 剪辑
    MEDIA_TASK_CONCAT = 4,         ///< 拼接
    MEDIA_TASK_CROP = 5,           ///< 裁剪
    MEDIA_TASK_SCALE = 6,          ///< 缩放
    MEDIA_TASK_FILTER = 7,         ///< 滤镜处理
    MEDIA_TASK_CUSTOM = 8,         ///< 自定义处理
} MediaTaskType;

/**
 * @brief 处理进度回调
 * @param processor 处理器指针
 * @param progress 进度（0.0-1.0）
 * @param current_frame 当前帧号
 * @param total_frames 总帧数
 * @param user_data 用户数据
 */
typedef void (*MediaProgressCallback)(MediaProcessor *processor, 
                                       double progress, 
                                       int64_t current_frame, 
                                       int64_t total_frames, 
                                       void *user_data);

/**
 * @brief 处理完成回调
 * @param processor 处理器指针
 * @param result 处理结果
 * @param user_data 用户数据
 */
typedef void (*MediaCompleteCallback)(MediaProcessor *processor, 
                                       MediaErrorCode result, 
                                       void *user_data);

/**
 * @brief 处理器配置
 */
typedef struct {
    int32_t thread_count;          ///< 处理线程数
    int32_t buffer_size;           ///< 缓冲区大小
    int32_t max_frame_queue;       ///< 最大帧队列长度
    int32_t copy_ts;               ///< 复制时间戳
    int32_t copy_metadata;         ///< 复制元数据
    int32_t fast_start;            ///< 快速启动（MP4）
    int32_t overwrite;             ///< 覆盖输出文件
    int32_t realtime;              ///< 实时处理模式
    void *user_data;               ///< 用户数据
} MediaProcessorConfig;

/* ============================================================================
 * 处理器创建与销毁
 * ============================================================================ */

/**
 * @brief 创建媒体处理器
 * @return 处理器指针，失败返回NULL
 */
MediaProcessor* media_processor_create(void);

/**
 * @brief 创建带配置的媒体处理器
 * @param config 配置结构体
 * @return 处理器指针，失败返回NULL
 */
MediaProcessor* media_processor_create_with_config(const MediaProcessorConfig *config);

/**
 * @brief 释放媒体处理器
 * @param processor 处理器指针
 */
void media_processor_free(MediaProcessor *processor);

/**
 * @brief 重置处理器配置为默认值
 * @param config 配置结构体指针
 */
void media_processor_config_default(MediaProcessorConfig *config);

/**
 * @brief 获取处理器状态
 * @param processor 处理器指针
 * @return 状态值
 */
MediaProcessorState media_processor_get_state(MediaProcessor *processor);

/* ============================================================================
 * 任务配置
 * ============================================================================ */

/**
 * @brief 设置输入文件
 * @param processor 处理器指针
 * @param input_file 输入文件路径
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_input(MediaProcessor *processor, 
                                          const char *input_file);

/**
 * @brief 设置多个输入文件（用于拼接）
 * @param processor 处理器指针
 * @param input_files 输入文件路径数组
 * @param count 文件数量
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_inputs(MediaProcessor *processor, 
                                           const char **input_files, 
                                           int32_t count);

/**
 * @brief 设置输出文件
 * @param processor 处理器指针
 * @param output_file 输出文件路径
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_output(MediaProcessor *processor, 
                                           const char *output_file);

/**
 * @brief 设置输出格式
 * @param processor 处理器指针
 * @param format 容器格式
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_output_format(MediaProcessor *processor, 
                                                  MediaContainerFormat format);

/**
 * @brief 设置任务类型
 * @param processor 处理器指针
 * @param task_type 任务类型
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_task_type(MediaProcessor *processor, 
                                              MediaTaskType task_type);

/* ============================================================================
 * 视频参数配置
 * ============================================================================ */

/**
 * @brief 设置视频编码器
 * @param processor 处理器指针
 * @param codec_id 编码器ID
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_video_codec(MediaProcessor *processor, 
                                                MediaCodecID codec_id);

/**
 * @brief 设置视频编码器（按名称）
 * @param processor 处理器指针
 * @param codec_name 编码器名称
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_video_codec_by_name(MediaProcessor *processor, 
                                                        const char *codec_name);

/**
 * @brief 设置视频比特率
 * @param processor 处理器指针
 * @param bit_rate 比特率（bps）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_video_bitrate(MediaProcessor *processor, 
                                                  int32_t bit_rate);

/**
 * @brief 设置视频分辨率
 * @param processor 处理器指针
 * @param width 宽度
 * @param height 高度
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_video_resolution(MediaProcessor *processor, 
                                                     int32_t width, int32_t height);

/**
 * @brief 设置视频帧率
 * @param processor 处理器指针
 * @param frame_rate 帧率
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_video_frame_rate(MediaProcessor *processor, 
                                                     MediaRational frame_rate);

/**
 * @brief 设置视频像素格式
 * @param processor 处理器指针
 * @param format 像素格式
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_video_pixel_format(MediaProcessor *processor, 
                                                       MediaPixelFormat format);

/**
 * @brief 设置视频GOP大小
 * @param processor 处理器指针
 * @param gop_size GOP大小
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_video_gop_size(MediaProcessor *processor, 
                                                   int32_t gop_size);

/**
 * @brief 设置视频编码配置
 * @param processor 处理器指针
 * @param profile 配置
 * @param level 级别
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_video_profile(MediaProcessor *processor, 
                                                  int32_t profile, int32_t level);

/* ============================================================================
 * 音频参数配置
 * ============================================================================ */

/**
 * @brief 设置音频编码器
 * @param processor 处理器指针
 * @param codec_id 编码器ID
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_audio_codec(MediaProcessor *processor, 
                                                MediaCodecID codec_id);

/**
 * @brief 设置音频编码器（按名称）
 * @param processor 处理器指针
 * @param codec_name 编码器名称
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_audio_codec_by_name(MediaProcessor *processor, 
                                                        const char *codec_name);

/**
 * @brief 设置音频比特率
 * @param processor 处理器指针
 * @param bit_rate 比特率（bps）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_audio_bitrate(MediaProcessor *processor, 
                                                  int32_t bit_rate);

/**
 * @brief 设置音频采样率
 * @param processor 处理器指针
 * @param sample_rate 采样率（Hz）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_audio_sample_rate(MediaProcessor *processor, 
                                                      int32_t sample_rate);

/**
 * @brief 设置音频声道数
 * @param processor 处理器指针
 * @param channels 声道数
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_audio_channels(MediaProcessor *processor, 
                                                   int32_t channels);

/**
 * @brief 设置音频采样格式
 * @param processor 处理器指针
 * @param format 采样格式
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_audio_sample_format(MediaProcessor *processor, 
                                                        MediaSampleFormat format);

/* ============================================================================
 * 剪辑配置
 * ============================================================================ */

/**
 * @brief 设置剪辑起始时间
 * @param processor 处理器指针
 * @param start_time 起始时间（秒）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_clip_start(MediaProcessor *processor, 
                                               double start_time);

/**
 * @brief 设置剪辑结束时间
 * @param processor 处理器指针
 * @param end_time 结束时间（秒）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_clip_end(MediaProcessor *processor, 
                                             double end_time);

/**
 * @brief 设置剪辑时长
 * @param processor 处理器指针
 * @param duration 时长（秒）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_clip_duration(MediaProcessor *processor, 
                                                  double duration);

/**
 * @brief 设置剪辑起始帧
 * @param processor 处理器指针
 * @param start_frame 起始帧号
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_clip_start_frame(MediaProcessor *processor, 
                                                     int64_t start_frame);

/**
 * @brief 设置剪辑结束帧
 * @param processor 处理器指针
 * @param end_frame 结束帧号
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_clip_end_frame(MediaProcessor *processor, 
                                                   int64_t end_frame);

/* ============================================================================
 * 裁剪与缩放配置
 * ============================================================================ */

/**
 * @brief 设置视频裁剪区域
 * @param processor 处理器指针
 * @param rect 裁剪区域
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_crop(MediaProcessor *processor, 
                                         MediaRect rect);

/**
 * @brief 设置视频缩放算法
 * @param processor 处理器指针
 * @param algorithm 缩放算法
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_scale_algorithm(MediaProcessor *processor, 
                                                    MediaScaleAlgorithm algorithm);

/**
 * @brief 设置视频缩放（保持宽高比）
 * @param processor 处理器指针
 * @param width 目标宽度（0为自动）
 * @param height 目标高度（0为自动）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_set_scale_keep_aspect(MediaProcessor *processor, 
                                                      int32_t width, int32_t height);

/* ============================================================================
 * 流选择
 * ============================================================================ */

/**
 * @brief 选择要处理的流
 * @param processor 处理器指针
 * @param stream_indices 流索引数组
 * @param count 流数量
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_select_streams(MediaProcessor *processor, 
                                               const int32_t *stream_indices, 
                                               int32_t count);

/**
 * @brief 选择所有视频流
 * @param processor 处理器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_select_all_video_streams(MediaProcessor *processor);

/**
 * @brief 选择所有音频流
 * @param processor 处理器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_select_all_audio_streams(MediaProcessor *processor);

/**
 * @brief 禁用视频流
 * @param processor 处理器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_disable_video(MediaProcessor *processor);

/**
 * @brief 禁用音频流
 * @param processor 处理器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_disable_audio(MediaProcessor *processor);

/**
 * @brief 复制视频流（不重新编码）
 * @param processor 处理器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_copy_video(MediaProcessor *processor);

/**
 * @brief 复制音频流（不重新编码）
 * @param processor 处理器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_copy_audio(MediaProcessor *processor);

/* ============================================================================
 * 处理控制
 * ============================================================================ */

/**
 * @brief 初始化处理器
 * @param processor 处理器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_init(MediaProcessor *processor);

/**
 * @brief 开始处理
 * @param processor 处理器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_start(MediaProcessor *processor);

/**
 * @brief 开始异步处理
 * @param processor 处理器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_start_async(MediaProcessor *processor);

/**
 * @brief 停止处理
 * @param processor 处理器指针
 */
void media_processor_stop(MediaProcessor *processor);

/**
 * @brief 暂停处理
 * @param processor 处理器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_pause(MediaProcessor *processor);

/**
 * @brief 恢复处理
 * @param processor 处理器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_resume(MediaProcessor *processor);

/**
 * @brief 等待处理完成
 * @param processor 处理器指针
 * @param timeout_ms 超时时间（毫秒，-1为无限等待）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_processor_wait(MediaProcessor *processor, int32_t timeout_ms);

/* ============================================================================
 * 回调设置
 * ============================================================================ */

/**
 * @brief 设置进度回调
 * @param processor 处理器指针
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void media_processor_set_progress_callback(MediaProcessor *processor, 
                                            MediaProgressCallback callback, 
                                            void *user_data);

/**
 * @brief 设置完成回调
 * @param processor 处理器指针
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void media_processor_set_complete_callback(MediaProcessor *processor, 
                                            MediaCompleteCallback callback, 
                                            void *user_data);

/* ============================================================================
 * 状态查询
 * ============================================================================ */

/**
 * @brief 获取处理进度
 * @param processor 处理器指针
 * @return 进度（0.0-1.0）
 */
double media_processor_get_progress(MediaProcessor *processor);

/**
 * @brief 获取已处理帧数
 * @param processor 处理器指针
 * @return 已处理帧数
 */
int64_t media_processor_get_processed_frames(MediaProcessor *processor);

/**
 * @brief 获取总帧数
 * @param processor 处理器指针
 * @return 总帧数
 */
int64_t media_processor_get_total_frames(MediaProcessor *processor);

/**
 * @brief 获取处理速度
 * @param processor 处理器指针
 * @return 处理速度（帧/秒）
 */
double media_processor_get_speed(MediaProcessor *processor);

/**
 * @brief 获取预估剩余时间
 * @param processor 处理器指针
 * @return 剩余时间（秒）
 */
double media_processor_get_remaining_time(MediaProcessor *processor);

/**
 * @brief 获取错误信息
 * @param processor 处理器指针
 * @return 错误信息字符串
 */
const char* media_processor_get_error_message(MediaProcessor *processor);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_PROCESSOR_H */
