/**
 * @file media_reader.h
 * @brief 媒体读取器接口
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 本文件定义了媒体文件读取的接口。
 * MediaReader负责从文件或网络流读取音视频数据。
 */

#ifndef MEDIA_READER_H
#define MEDIA_READER_H

#include "core/media_types.h"
#include "core/media_frame.h"
#include "core/media_packet.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 媒体读取器结构体（不透明指针）
 */
typedef struct MediaReader MediaReader;

/**
 * @brief 读取器状态
 */
typedef enum {
    MEDIA_READER_STATE_CLOSED = 0,
    MEDIA_READER_STATE_OPENING = 1,
    MEDIA_READER_STATE_READY = 2,
    MEDIA_READER_STATE_READING = 3,
    MEDIA_READER_STATE_EOF = 4,
    MEDIA_READER_STATE_ERROR = 5,
} MediaReaderState;

/**
 * @brief 读取器配置
 */
typedef struct {
    int32_t buffer_size;          ///< 缓冲区大小（字节）
    int32_t probe_size;           ///< 探测大小（字节）
    int32_t analyze_duration;     ///< 分析时长（微秒）
    int32_t max_stream_count;     ///< 最大流数量
    int32_t thread_count;         ///< 解码线程数
    int32_t low_res;              ///< 低分辨率解码（0=禁用）
    int32_t fast_seek;            ///< 快速寻址（可能不精确）
    int32_t discard_corrupt;      ///< 丢弃损坏帧
    int32_t gen_pts;              ///< 生成缺失的PTS
    int32_t ignore_dts;           ///< 忽略DTS
    const char *format;           ///< 强制指定格式
    const char *codec_whitelist;  ///< 编解码器白名单
    const char *format_whitelist; ///< 格式白名单
    void *user_data;              ///< 用户数据
} MediaReaderConfig;

/**
 * @brief 读取回调函数类型
 * @param reader 读取器指针
 * @param packet 数据包
 * @param user_data 用户数据
 * @return 成功返回MEDIA_SUCCESS，负值表示错误
 */
typedef MediaErrorCode (*MediaReadCallback)(MediaReader *reader, 
                                             MediaPacket *packet, 
                                             void *user_data);

/* ============================================================================
 * 读取器创建与销毁
 * ============================================================================ */

/**
 * @brief 创建媒体读取器
 * @return 读取器指针，失败返回NULL
 */
MediaReader* media_reader_create(void);

/**
 * @brief 创建带配置的媒体读取器
 * @param config 配置结构体
 * @return 读取器指针，失败返回NULL
 */
MediaReader* media_reader_create_with_config(const MediaReaderConfig *config);

/**
 * @brief 释放媒体读取器
 * @param reader 读取器指针
 */
void media_reader_free(MediaReader *reader);

/**
 * @brief 重置读取器配置为默认值
 * @param config 配置结构体指针
 */
void media_reader_config_default(MediaReaderConfig *config);

/* ============================================================================
 * 文件打开与关闭
 * ============================================================================ */

/**
 * @brief 打开媒体文件
 * @param reader 读取器指针
 * @param filename 文件名或URL
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_open(MediaReader *reader, const char *filename);

/**
 * @brief 打开媒体文件（带选项）
 * @param reader 读取器指针
 * @param filename 文件名或URL
 * @param options 选项字典（key-value对）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_open_with_options(MediaReader *reader, 
                                               const char *filename, 
                                               const char **options);

/**
 * @brief 关闭媒体文件
 * @param reader 读取器指针
 */
void media_reader_close(MediaReader *reader);

/**
 * @brief 检查读取器是否已打开
 * @param reader 读取器指针
 * @return true表示已打开
 */
bool media_reader_is_open(MediaReader *reader);

/**
 * @brief 获取读取器状态
 * @param reader 读取器指针
 * @return 状态值
 */
MediaReaderState media_reader_get_state(MediaReader *reader);

/* ============================================================================
 * 文件信息获取
 * ============================================================================ */

/**
 * @brief 获取媒体文件信息
 * @param reader 读取器指针
 * @return 文件信息结构体指针（只读）
 */
const MediaFileInfo* media_reader_get_file_info(MediaReader *reader);

/**
 * @brief 获取流信息
 * @param reader 读取器指针
 * @param stream_index 流索引
 * @return 流信息结构体指针（只读）
 */
const MediaStreamInfo* media_reader_get_stream_info(MediaReader *reader, 
                                                     int32_t stream_index);

/**
 * @brief 获取最佳视频流索引
 * @param reader 读取器指针
 * @return 流索引，-1表示无视频流
 */
int32_t media_reader_get_best_video_stream(MediaReader *reader);

/**
 * @brief 获取最佳音频流索引
 * @param reader 读取器指针
 * @return 流索引，-1表示无音频流
 */
int32_t media_reader_get_best_audio_stream(MediaReader *reader);

/**
 * @brief 获取最佳字幕流索引
 * @param reader 读取器指针
 * @return 流索引，-1表示无字幕流
 */
int32_t media_reader_get_best_subtitle_stream(MediaReader *reader);

/**
 * @brief 获取文件时长（秒）
 * @param reader 读取器指针
 * @return 时长（秒），失败返回-1
 */
double media_reader_get_duration(MediaReader *reader);

/**
 * @brief 获取文件比特率
 * @param reader 读取器指针
 * @return 比特率（bps）
 */
int64_t media_reader_get_bit_rate(MediaReader *reader);

/**
 * @brief 获取元数据
 * @param reader 读取器指针
 * @param key 元数据键名
 * @param value_buf 值缓冲区
 * @param buf_size 缓冲区大小
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_get_metadata(MediaReader *reader, 
                                          const char *key, 
                                          char *value_buf, 
                                          int32_t buf_size);

/* ============================================================================
 * 流选择与配置
 * ============================================================================ */

/**
 * @brief 选择要读取的流
 * @param reader 读取器指针
 * @param stream_indices 流索引数组
 * @param count 流数量
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_select_streams(MediaReader *reader, 
                                            const int32_t *stream_indices, 
                                            int32_t count);

/**
 * @brief 启用/禁用流
 * @param reader 读取器指针
 * @param stream_index 流索引
 * @param enable 是否启用
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_enable_stream(MediaReader *reader, 
                                           int32_t stream_index, 
                                           bool enable);

/**
 * @brief 设置读取流类型
 * @param reader 读取器指针
 * @param type 流类型（视频、音频等）
 * @param enable 是否启用
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_enable_stream_type(MediaReader *reader, 
                                                MediaStreamType type, 
                                                bool enable);

/* ============================================================================
 * 数据读取
 * ============================================================================ */

/**
 * @brief 读取一个数据包
 * @param reader 读取器指针
 * @param packet 数据包指针（输出）
 * @return 成功返回MEDIA_SUCCESS，EOF返回MEDIA_EOF
 */
MediaErrorCode media_reader_read_packet(MediaReader *reader, MediaPacket *packet);

/**
 * @brief 读取一个解码帧
 * @param reader 读取器指针
 * @param frame 帧指针（输出）
 * @param stream_index 流索引（输出，可为NULL）
 * @return 成功返回MEDIA_SUCCESS，EOF返回MEDIA_EOF
 */
MediaErrorCode media_reader_read_frame(MediaReader *reader, 
                                        MediaFrame *frame, 
                                        int32_t *stream_index);

/**
 * @brief 读取指定流的帧
 * @param reader 读取器指针
 * @param frame 帧指针（输出）
 * @param stream_index 流索引
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_read_frame_from_stream(MediaReader *reader, 
                                                    MediaFrame *frame, 
                                                    int32_t stream_index);

/**
 * @brief 批量读取数据包
 * @param reader 读取器指针
 * @param packets 数据包数组
 * @param max_count 最大数量
 * @param actual_count 实际数量（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_read_packets(MediaReader *reader, 
                                          MediaPacket **packets, 
                                          int32_t max_count, 
                                          int32_t *actual_count);

/**
 * @brief 设置读取回调
 * @param reader 读取器指针
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void media_reader_set_read_callback(MediaReader *reader, 
                                     MediaReadCallback callback, 
                                     void *user_data);

/* ============================================================================
 * 寻址操作
 * ============================================================================ */

/**
 * @brief 寻址到指定时间点
 * @param reader 读取器指针
 * @param timestamp 时间戳（秒）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_seek(MediaReader *reader, double timestamp);

/**
 * @brief 寻址到指定时间点（精确）
 * @param reader 读取器指针
 * @param timestamp 时间戳（秒）
 * @param stream_index 参考流索引
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_seek_precise(MediaReader *reader, 
                                          double timestamp, 
                                          int32_t stream_index);

/**
 * @brief 寻址到指定帧
 * @param reader 读取器指针
 * @param frame_number 帧号
 * @param stream_index 流索引
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_seek_to_frame(MediaReader *reader, 
                                           int64_t frame_number, 
                                           int32_t stream_index);

/**
 * @brief 寻址到文件开头
 * @param reader 读取器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_seek_to_start(MediaReader *reader);

/**
 * @brief 寻址到文件结尾
 * @param reader 读取器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_seek_to_end(MediaReader *reader);

/**
 * @brief 获取当前位置（秒）
 * @param reader 读取器指针
 * @return 当前位置（秒）
 */
double media_reader_get_position(MediaReader *reader);

/* ============================================================================
 * 缓冲控制
 * ============================================================================ */

/**
 * @brief 获取缓冲区使用率
 * @param reader 读取器指针
 * @return 使用率（0.0-1.0）
 */
double media_reader_get_buffer_usage(MediaReader *reader);

/**
 * @brief 清空缓冲区
 * @param reader 读取器指针
 */
void media_reader_flush_buffer(MediaReader *reader);

/**
 * @brief 设置缓冲区大小
 * @param reader 读取器指针
 * @param size 大小（字节）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_reader_set_buffer_size(MediaReader *reader, int32_t size);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_READER_H */
