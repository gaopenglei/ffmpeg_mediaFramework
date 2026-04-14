/**
 * @file media_writer.h
 * @brief 媒体写入器接口
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 本文件定义了媒体文件写入的接口。
 * MediaWriter负责将音视频数据写入文件或网络流。
 */

#ifndef MEDIA_WRITER_H
#define MEDIA_WRITER_H

#include "core/media_types.h"
#include "core/media_frame.h"
#include "core/media_packet.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 媒体写入器结构体（不透明指针）
 */
typedef struct MediaWriter MediaWriter;

/**
 * @brief 写入器状态
 */
typedef enum {
    MEDIA_WRITER_STATE_CLOSED = 0,
    MEDIA_WRITER_STATE_OPENING = 1,
    MEDIA_WRITER_STATE_READY = 2,
    MEDIA_WRITER_STATE_WRITING = 3,
    MEDIA_WRITER_STATE_FINALIZING = 4,
    MEDIA_WRITER_STATE_ERROR = 5,
} MediaWriterState;

/**
 * @brief 写入器配置
 */
typedef struct {
    MediaContainerFormat container; ///< 容器格式
    int32_t buffer_size;            ///< 缓冲区大小
    int32_t preload;                ///< 预加载时间（毫秒）
    int32_t max_delay;              ///< 最大延迟（微秒）
    int32_t use_wallclock_ts;       ///< 使用实时时钟时间戳
    int32_t fast_start;             ///< 快速启动（MP4）
    int32_t movflags;               ///< MOV标志
    int32_t overwrite;              ///< 覆盖已存在文件
    int32_t thread_count;           ///< 编码线程数
    const char *format;             ///< 强制指定格式
    void *user_data;                ///< 用户数据
} MediaWriterConfig;

/**
 * @brief 流配置
 */
typedef struct {
    MediaStreamType type;           ///< 流类型
    MediaCodecID codec_id;          ///< 编码器ID
    const char *codec_name;         ///< 编码器名称
    MediaRational time_base;        ///< 时间基
    int64_t duration;               ///< 预期时长
    union {
        VideoParams video;          ///< 视频参数
        AudioParams audio;          ///< 音频参数
    } params;
    int32_t copy_stream;            ///< 是否复制流（不重新编码）
    const char *bitstream_filter;   ///< 比特流滤镜
} MediaStreamConfig;

/* ============================================================================
 * 写入器创建与销毁
 * ============================================================================ */

/**
 * @brief 创建媒体写入器
 * @return 写入器指针，失败返回NULL
 */
MediaWriter* media_writer_create(void);

/**
 * @brief 创建带配置的媒体写入器
 * @param config 配置结构体
 * @return 写入器指针，失败返回NULL
 */
MediaWriter* media_writer_create_with_config(const MediaWriterConfig *config);

/**
 * @brief 释放媒体写入器
 * @param writer 写入器指针
 */
void media_writer_free(MediaWriter *writer);

/**
 * @brief 重置写入器配置为默认值
 * @param config 配置结构体指针
 */
void media_writer_config_default(MediaWriterConfig *config);

/* ============================================================================
 * 文件打开与关闭
 * ============================================================================ */

/**
 * @brief 打开输出文件
 * @param writer 写入器指针
 * @param filename 文件名或URL
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_open(MediaWriter *writer, const char *filename);

/**
 * @brief 打开输出文件（带配置）
 * @param writer 写入器指针
 * @param filename 文件名或URL
 * @param config 配置结构体
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_open_with_config(MediaWriter *writer, 
                                              const char *filename, 
                                              const MediaWriterConfig *config);

/**
 * @brief 关闭输出文件
 * @param writer 写入器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_close(MediaWriter *writer);

/**
 * @brief 检查写入器是否已打开
 * @param writer 写入器指针
 * @return true表示已打开
 */
bool media_writer_is_open(MediaWriter *writer);

/**
 * @brief 获取写入器状态
 * @param writer 写入器指针
 * @return 状态值
 */
MediaWriterState media_writer_get_state(MediaWriter *writer);

/* ============================================================================
 * 流配置
 * ============================================================================ */

/**
 * @brief 添加视频流
 * @param writer 写入器指针
 * @param params 视频参数
 * @return 流索引，失败返回-1
 */
int32_t media_writer_add_video_stream(MediaWriter *writer, 
                                       const VideoParams *params);

/**
 * @brief 添加音频流
 * @param writer 写入器指针
 * @param params 音频参数
 * @return 流索引，失败返回-1
 */
int32_t media_writer_add_audio_stream(MediaWriter *writer, 
                                       const AudioParams *params);

/**
 * @brief 添加流（通用）
 * @param writer 写入器指针
 * @param config 流配置
 * @return 流索引，失败返回-1
 */
int32_t media_writer_add_stream(MediaWriter *writer, 
                                 const MediaStreamConfig *config);

/**
 * @brief 移除流
 * @param writer 写入器指针
 * @param stream_index 流索引
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_remove_stream(MediaWriter *writer, 
                                           int32_t stream_index);

/**
 * @brief 获取流数量
 * @param writer 写入器指针
 * @return 流数量
 */
int32_t media_writer_get_stream_count(MediaWriter *writer);

/**
 * @brief 获取流配置
 * @param writer 写入器指针
 * @param stream_index 流索引
 * @param config 流配置（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_get_stream_config(MediaWriter *writer, 
                                               int32_t stream_index, 
                                               MediaStreamConfig *config);

/* ============================================================================
 * 编码器配置
 * ============================================================================ */

/**
 * @brief 设置流编码器
 * @param writer 写入器指针
 * @param stream_index 流索引
 * @param codec_id 编码器ID
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_set_codec(MediaWriter *writer, 
                                       int32_t stream_index, 
                                       MediaCodecID codec_id);

/**
 * @brief 设置流编码器（按名称）
 * @param writer 写入器指针
 * @param stream_index 流索引
 * @param codec_name 编码器名称
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_set_codec_by_name(MediaWriter *writer, 
                                               int32_t stream_index, 
                                               const char *codec_name);

/**
 * @brief 设置编码器选项
 * @param writer 写入器指针
 * @param stream_index 流索引
 * @param key 选项名称
 * @param value 选项值
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_set_codec_option(MediaWriter *writer, 
                                              int32_t stream_index, 
                                              const char *key, 
                                              const char *value);

/**
 * @brief 设置视频编码参数
 * @param writer 写入器指针
 * @param stream_index 流索引
 * @param bit_rate 比特率
 * @param gop_size GOP大小
 * @param max_b_frames 最大B帧数
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_set_video_encode_params(MediaWriter *writer, 
                                                     int32_t stream_index, 
                                                     int32_t bit_rate, 
                                                     int32_t gop_size, 
                                                     int32_t max_b_frames);

/**
 * @brief 设置音频编码参数
 * @param writer 写入器指针
 * @param stream_index 流索引
 * @param bit_rate 比特率
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_set_audio_encode_params(MediaWriter *writer, 
                                                     int32_t stream_index, 
                                                     int32_t bit_rate);

/* ============================================================================
 * 数据写入
 * ============================================================================ */

/**
 * @brief 写入数据包
 * @param writer 写入器指针
 * @param packet 数据包
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_write_packet(MediaWriter *writer, 
                                          const MediaPacket *packet);

/**
 * @brief 写入帧
 * @param writer 写入器指针
 * @param frame 帧
 * @param stream_index 流索引
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_write_frame(MediaWriter *writer, 
                                         const MediaFrame *frame, 
                                         int32_t stream_index);

/**
 * @brief 编码并写入帧
 * @param writer 写入器指针
 * @param frame 帧
 * @param stream_index 流索引
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_encode_and_write(MediaWriter *writer, 
                                              const MediaFrame *frame, 
                                              int32_t stream_index);

/**
 * @brief 刷新编码器
 * @param writer 写入器指针
 * @param stream_index 流索引（-1表示所有流）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_flush_encoder(MediaWriter *writer, 
                                           int32_t stream_index);

/* ============================================================================
 * 元数据管理
 * ============================================================================ */

/**
 * @brief 设置文件元数据
 * @param writer 写入器指针
 * @param key 键名
 * @param value 值
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_set_metadata(MediaWriter *writer, 
                                          const char *key, 
                                          const char *value);

/**
 * @brief 设置流元数据
 * @param writer 写入器指针
 * @param stream_index 流索引
 * @param key 键名
 * @param value 值
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_set_stream_metadata(MediaWriter *writer, 
                                                 int32_t stream_index, 
                                                 const char *key, 
                                                 const char *value);

/**
 * @brief 设置封面图
 * @param writer 写入器指针
 * @param cover_file 封面图文件路径
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_set_cover_art(MediaWriter *writer, 
                                           const char *cover_file);

/* ============================================================================
 * 时间戳管理
 * ============================================================================ */

/**
 * @brief 设置时间基
 * @param writer 写入器指针
 * @param stream_index 流索引
 * @param time_base 时间基
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_writer_set_time_base(MediaWriter *writer, 
                                           int32_t stream_index, 
                                           MediaRational time_base);

/**
 * @brief 获取时间基
 * @param writer 写入器指针
 * @param stream_index 流索引
 * @return 时间基
 */
MediaRational media_writer_get_time_base(MediaWriter *writer, 
                                          int32_t stream_index);

/**
 * @brief 转换时间戳
 * @param writer 写入器指针
 * @param ts 时间戳
 * @param src_tb 源时间基
 * @param dst_stream_index 目标流索引
 * @return 转换后的时间戳
 */
int64_t media_writer_rescale_ts(MediaWriter *writer, 
                                 int64_t ts, 
                                 MediaRational src_tb, 
                                 int32_t dst_stream_index);

/* ============================================================================
 * 状态查询
 * ============================================================================ */

/**
 * @brief 获取已写入字节数
 * @param writer 写入器指针
 * @return 字节数
 */
int64_t media_writer_get_bytes_written(MediaWriter *writer);

/**
 * @brief 获取已写入帧数
 * @param writer 写入器指针
 * @param stream_index 流索引
 * @return 帧数
 */
int64_t media_writer_get_frames_written(MediaWriter *writer, 
                                         int32_t stream_index);

/**
 * @brief 获取已写入时长
 * @param writer 写入器指针
 * @param stream_index 流索引
 * @return 时长（秒）
 */
double media_writer_get_duration_written(MediaWriter *writer, 
                                          int32_t stream_index);

/**
 * @brief 获取输出文件名
 * @param writer 写入器指针
 * @return 文件名
 */
const char* media_writer_get_filename(MediaWriter *writer);

/**
 * @brief 获取输出格式
 * @param writer 写入器指针
 * @return 容器格式
 */
MediaContainerFormat media_writer_get_container_format(MediaWriter *writer);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_WRITER_H */
