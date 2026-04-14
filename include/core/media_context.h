/**
 * @file media_context.h
 * @brief 媒体上下文管理接口
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 本文件定义了媒体上下文的创建和管理接口。
 * MediaContext是整个框架的核心管理结构，管理全局状态和资源。
 */

#ifndef MEDIA_CONTEXT_H
#define MEDIA_CONTEXT_H

#include "media_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 媒体上下文结构体（不透明指针）
 */
typedef struct MediaContext MediaContext;

/**
 * @brief 日志级别
 */
typedef enum {
    MEDIA_LOG_QUIET = -8,
    MEDIA_LOG_PANIC = 0,
    MEDIA_LOG_FATAL = 8,
    MEDIA_LOG_ERROR = 16,
    MEDIA_LOG_WARNING = 24,
    MEDIA_LOG_INFO = 32,
    MEDIA_LOG_VERBOSE = 40,
    MEDIA_LOG_DEBUG = 48,
    MEDIA_LOG_TRACE = 56,
} MediaLogLevel;

/**
 * @brief 日志回调函数类型
 * @param level 日志级别
 * @param file 源文件名
 * @param line 行号
 * @param func 函数名
 * @param message 日志消息
 * @param user_data 用户数据
 */
typedef void (*MediaLogCallback)(MediaLogLevel level, 
                                  const char *file, 
                                  int line,
                                  const char *func, 
                                  const char *message, 
                                  void *user_data);

/* ============================================================================
 * 全局初始化与清理
 * ============================================================================ */

/**
 * @brief 初始化媒体框架
 * 
 * 必须在使用任何其他API之前调用。
 * 此函数会初始化FFmpeg库和所有必要的资源。
 * 
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_init(void);

/**
 * @brief 清理媒体框架
 * 
 * 在程序退出前调用，释放所有全局资源。
 */
void media_cleanup(void);

/**
 * @brief 检查媒体框架是否已初始化
 * @return true表示已初始化
 */
bool media_is_initialized(void);

/**
 * @brief 获取框架版本字符串
 * @return 版本字符串
 */
const char* media_version(void);

/**
 * @brief 获取框架配置信息
 * @return 配置信息字符串
 */
const char* media_configuration(void);

/* ============================================================================
 * 日志管理
 * ============================================================================ */

/**
 * @brief 设置日志级别
 * @param level 日志级别
 */
void media_set_log_level(MediaLogLevel level);

/**
 * @brief 获取当前日志级别
 * @return 日志级别
 */
MediaLogLevel media_get_log_level(void);

/**
 * @brief 设置日志回调函数
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void media_set_log_callback(MediaLogCallback callback, void *user_data);

/**
 * @brief 输出日志
 * @param level 日志级别
 * @param file 源文件名
 * @param line 行号
 * @param func 函数名
 * @param format 格式字符串
 * @param ... 可变参数
 */
void media_log(MediaLogLevel level, 
               const char *file, 
               int line, 
               const char *func,
               const char *format, ...);

/* 便捷日志宏 */
#define MEDIA_LOG_PANIC(...)   media_log(MEDIA_LOG_PANIC, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MEDIA_LOG_FATAL(...)   media_log(MEDIA_LOG_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MEDIA_LOG_ERROR(...)   media_log(MEDIA_LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MEDIA_LOG_WARNING(...) media_log(MEDIA_LOG_WARNING, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MEDIA_LOG_INFO(...)    media_log(MEDIA_LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MEDIA_LOG_VERBOSE(...) media_log(MEDIA_LOG_VERBOSE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MEDIA_LOG_DEBUG(...)   media_log(MEDIA_LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MEDIA_LOG_TRACE(...)   media_log(MEDIA_LOG_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)

/* ============================================================================
 * 上下文创建与销毁
 * ============================================================================ */

/**
 * @brief 创建媒体上下文
 * @return 上下文指针，失败返回NULL
 */
MediaContext* media_context_create(void);

/**
 * @brief 释放媒体上下文
 * @param ctx 上下文指针
 */
void media_context_free(MediaContext *ctx);

/**
 * @brief 重置媒体上下文
 * @param ctx 上下文指针
 */
void media_context_reset(MediaContext *ctx);

/* ============================================================================
 * 上下文配置
 * ============================================================================ */

/**
 * @brief 设置上下文选项
 * @param ctx 上下文指针
 * @param key 选项名称
 * @param value 选项值
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_context_set_option(MediaContext *ctx, 
                                         const char *key, 
                                         const char *value);

/**
 * @brief 获取上下文选项
 * @param ctx 上下文指针
 * @param key 选项名称
 * @param value_buf 值缓冲区
 * @param buf_size 缓冲区大小
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_context_get_option(MediaContext *ctx, 
                                         const char *key, 
                                         char *value_buf, 
                                         int32_t buf_size);

/**
 * @brief 设置线程数
 * @param ctx 上下文指针
 * @param thread_count 线程数（0为自动）
 */
void media_context_set_thread_count(MediaContext *ctx, int32_t thread_count);

/**
 * @brief 获取线程数
 * @param ctx 上下文指针
 * @return 线程数
 */
int32_t media_context_get_thread_count(MediaContext *ctx);

/* ============================================================================
 * 资源管理
 * ============================================================================ */

/**
 * @brief 获取已分配内存总量
 * @param ctx 上下文指针
 * @return 内存量（字节）
 */
int64_t media_context_get_allocated_memory(MediaContext *ctx);

/**
 * @brief 获取打开的文件数量
 * @param ctx 上下文指针
 * @return 文件数量
 */
int32_t media_context_get_open_files(MediaContext *ctx);

/**
 * @brief 获取打开的流数量
 * @param ctx 上下文指针
 * @return 流数量
 */
int32_t media_context_get_open_streams(MediaContext *ctx);

/* ============================================================================
 * 编解码器查询
 * ============================================================================ */

/**
 * @brief 检查编码器是否可用
 * @param ctx 上下文指针
 * @param codec_id 编码器ID
 * @return true表示可用
 */
bool media_context_codec_available(MediaContext *ctx, MediaCodecID codec_id);

/**
 * @brief 检查解码器是否可用
 * @param ctx 上下文指针
 * @param codec_id 解码器ID
 * @return true表示可用
 */
bool media_context_decoder_available(MediaContext *ctx, MediaCodecID codec_id);

/**
 * @brief 检查编码器是否可用（按名称）
 * @param ctx 上下文指针
 * @param name 编码器名称
 * @return true表示可用
 */
bool media_context_encoder_available_by_name(MediaContext *ctx, const char *name);

/**
 * @brief 检查解码器是否可用（按名称）
 * @param ctx 上下文指针
 * @param name 解码器名称
 * @return true表示可用
 */
bool media_context_decoder_available_by_name(MediaContext *ctx, const char *name);

/**
 * @brief 获取支持的编码器列表
 * @param ctx 上下文指针
 * @param codecs 编码器ID数组
 * @param max_count 数组最大容量
 * @return 实际编码器数量
 */
int32_t media_context_get_supported_encoders(MediaContext *ctx, 
                                              MediaCodecID *codecs, 
                                              int32_t max_count);

/**
 * @brief 获取支持的解码器列表
 * @param ctx 上下文指针
 * @param codecs 解码器ID数组
 * @param max_count 数组最大容量
 * @return 实际解码器数量
 */
int32_t media_context_get_supported_decoders(MediaContext *ctx, 
                                              MediaCodecID *codecs, 
                                              int32_t max_count);

/* ============================================================================
 * 滤镜查询
 * ============================================================================ */

/**
 * @brief 检查滤镜是否可用
 * @param ctx 上下文指针
 * @param filter_name 滤镜名称
 * @return true表示可用
 */
bool media_context_filter_available(MediaContext *ctx, const char *filter_name);

/**
 * @brief 获取支持的滤镜列表
 * @param ctx 上下文指针
 * @param filters 滤镜名称数组
 * @param max_count 数组最大容量
 * @return 实际滤镜数量
 */
int32_t media_context_get_supported_filters(MediaContext *ctx, 
                                             char **filters, 
                                             int32_t max_count);

/* ============================================================================
 * 格式查询
 * ============================================================================ */

/**
 * @brief 检查容器格式是否支持
 * @param ctx 上下文指针
 * @param container 容器格式
 * @return true表示支持
 */
bool media_context_container_supported(MediaContext *ctx, 
                                        MediaContainerFormat container);

/**
 * @brief 检查协议是否支持
 * @param ctx 上下文指针
 * @param protocol 协议类型
 * @return true表示支持
 */
bool media_context_protocol_supported(MediaContext *ctx, MediaProtocol protocol);

/**
 * @brief 检查协议是否支持（按名称）
 * @param ctx 上下文指针
 * @param protocol_name 协议名称
 * @return true表示支持
 */
bool media_context_protocol_supported_by_name(MediaContext *ctx, 
                                               const char *protocol_name);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_CONTEXT_H */
