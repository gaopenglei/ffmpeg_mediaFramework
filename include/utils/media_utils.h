/**
 * @file media_utils.h
 * @brief 媒体工具函数接口
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 本文件定义了各种工具函数，包括日志、内存管理、字符串处理等。
 */

#ifndef MEDIA_UTILS_H
#define MEDIA_UTILS_H

#include "core/media_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 日志系统
 * ============================================================================ */

/**
 * @brief 日志级别
 */
typedef enum {
    MEDIA_LOG_LEVEL_QUIET = -8,
    MEDIA_LOG_LEVEL_PANIC = 0,
    MEDIA_LOG_LEVEL_FATAL = 8,
    MEDIA_LOG_LEVEL_ERROR = 16,
    MEDIA_LOG_LEVEL_WARNING = 24,
    MEDIA_LOG_LEVEL_INFO = 32,
    MEDIA_LOG_LEVEL_VERBOSE = 40,
    MEDIA_LOG_LEVEL_DEBUG = 48,
    MEDIA_LOG_LEVEL_TRACE = 56,
} MediaUtilsLogLevel;

/**
 * @brief 日志回调函数类型
 */
typedef void (*MediaUtilsLogCallback)(void *ptr, int level, 
                                       const char *fmt, va_list vl);

/**
 * @brief 设置日志级别
 * @param level 日志级别
 */
void media_utils_set_log_level(int level);

/**
 * @brief 设置日志回调
 * @param callback 回调函数
 */
void media_utils_set_log_callback(MediaUtilsLogCallback callback);

/**
 * @brief 输出日志
 * @param level 日志级别
 * @param fmt 格式字符串
 * @param ... 可变参数
 */
void media_utils_log(int level, const char *fmt, ...);

/* 日志宏 */
#define MEDIA_LOG_PANIC(fmt, ...)   media_utils_log(MEDIA_LOG_LEVEL_PANIC, fmt, ##__VA_ARGS__)
#define MEDIA_LOG_FATAL(fmt, ...)   media_utils_log(MEDIA_LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)
#define MEDIA_LOG_ERROR(fmt, ...)   media_utils_log(MEDIA_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define MEDIA_LOG_WARN(fmt, ...)    media_utils_log(MEDIA_LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define MEDIA_LOG_INFO(fmt, ...)    media_utils_log(MEDIA_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define MEDIA_LOG_VERBOSE(fmt, ...) media_utils_log(MEDIA_LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define MEDIA_LOG_DEBUG(fmt, ...)   media_utils_log(MEDIA_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define MEDIA_LOG_TRACE(fmt, ...)   media_utils_log(MEDIA_LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)

/* ============================================================================
 * 内存管理
 * ============================================================================ */

/**
 * @brief 分配内存
 * @param size 大小
 * @return 内存指针，失败返回NULL
 */
void* media_malloc(size_t size);

/**
 * @brief 分配并清零内存
 * @param size 大小
 * @return 内存指针，失败返回NULL
 */
void* media_calloc(size_t count, size_t size);

/**
 * @brief 重新分配内存
 * @param ptr 原内存指针
 * @param size 新大小
 * @return 新内存指针，失败返回NULL
 */
void* media_realloc(void *ptr, size_t size);

/**
 * @brief 释放内存
 * @param ptr 内存指针
 */
void media_free(void *ptr);

/**
 * @brief 分配对齐内存
 * @param size 大小
 * @param alignment 对齐值
 * @return 内存指针，失败返回NULL
 */
void* media_malloc_aligned(size_t size, size_t alignment);

/**
 * @brief 释放对齐内存
 * @param ptr 内存指针
 */
void media_free_aligned(void *ptr);

/**
 * @brief 内存复制
 * @param dst 目标地址
 * @param src 源地址
 * @param size 大小
 * @return 目标地址
 */
void* media_memcpy(void *dst, const void *src, size_t size);

/**
 * @brief 内存设置
 * @param dst 目标地址
 * @param value 值
 * @param size 大小
 * @return 目标地址
 */
void* media_memset(void *dst, int value, size_t size);

/**
 * @brief 内存比较
 * @param s1 地址1
 * @param s2 地址2
 * @param size 大小
 * @return 比较结果
 */
int media_memcmp(const void *s1, const void *s2, size_t size);

/* ============================================================================
 * 字符串处理
 * ============================================================================ */

/**
 * @brief 复制字符串
 * @param dst 目标缓冲区
 * @param src 源字符串
 * @param size 缓冲区大小
 * @return 目标地址
 */
char* media_strlcpy(char *dst, const char *src, size_t size);

/**
 * @brief 连接字符串
 * @param dst 目标缓冲区
 * @param src 源字符串
 * @param size 缓冲区大小
 * @return 目标地址
 */
char* media_strlcat(char *dst, const char *src, size_t size);

/**
 * @brief 复制字符串（动态分配）
 * @param s 源字符串
 * @return 新字符串指针，失败返回NULL
 */
char* media_strdup(const char *s);

/**
 * @brief 比较字符串（忽略大小写）
 * @param s1 字符串1
 * @param s2 字符串2
 * @return 比较结果
 */
int media_strcasecmp(const char *s1, const char *s2);

/**
 * @brief 比较字符串（指定长度，忽略大小写）
 * @param s1 字符串1
 * @param s2 字符串2
 * @param n 最大长度
 * @return 比较结果
 */
int media_strncasecmp(const char *s1, const char *s2, size_t n);

/**
 * @brief 查找子字符串
 * @param haystack 主字符串
 * @param needle 子字符串
 * @return 子字符串位置，未找到返回NULL
 */
char* media_strstr(const char *haystack, const char *needle);

/**
 * @brief 格式化字符串
 * @param fmt 格式字符串
 * @param ... 可变参数
 * @return 新字符串指针，失败返回NULL
 */
char* media_asprintf(const char *fmt, ...);

/**
 * @brief 去除首尾空白字符
 * @param str 字符串
 * @return 新字符串指针
 */
char* media_strtrim(char *str);

/* ============================================================================
 * 时间工具
 * ============================================================================ */

/**
 * @brief 获取当前时间（微秒）
 * @return 当前时间
 */
int64_t media_gettime_us(void);

/**
 * @brief 获取当前时间（毫秒）
 * @return 当前时间
 */
int64_t media_gettime_ms(void);

/**
 * @brief 获取当前时间（秒）
 * @return 当前时间
 */
double media_gettime_sec(void);

/**
 * @brief 睡眠（微秒）
 * @param us 微秒数
 */
void media_sleep_us(int64_t us);

/**
 * @brief 睡眠（毫秒）
 * @param ms 毫秒数
 */
void media_sleep_ms(int64_t ms);

/* ============================================================================
 * 文件工具
 * ============================================================================ */

/**
 * @brief 检查文件是否存在
 * @param path 文件路径
 * @return true表示存在
 */
bool media_file_exists(const char *path);

/**
 * @brief 检查是否为目录
 * @param path 路径
 * @return true表示是目录
 */
bool media_is_directory(const char *path);

/**
 * @brief 获取文件大小
 * @param path 文件路径
 * @return 文件大小，失败返回-1
 */
int64_t media_file_size(const char *path);

/**
 * @brief 获取文件扩展名
 * @param path 文件路径
 * @return 扩展名（不含点）
 */
const char* media_file_extension(const char *path);

/**
 * @brief 获取文件名（不含路径）
 * @param path 文件路径
 * @return 文件名
 */
const char* media_file_name(const char *path);

/**
 * @brief 创建目录
 * @param path 目录路径
 * @return 成功返回0
 */
int media_mkdir(const char *path);

/**
 * @brief 删除文件
 * @param path 文件路径
 * @return 成功返回0
 */
int media_file_delete(const char *path);

/**
 * @brief 重命名文件
 * @param old_path 原路径
 * @param new_path 新路径
 * @return 成功返回0
 */
int media_file_rename(const char *old_path, const char *new_path);

/* ============================================================================
 * 数学工具
 * ============================================================================ */

/**
 * @brief 限制值在范围内
 * @param a 值
 * @param min 最小值
 * @param max 最大值
 * @return 限制后的值
 */
int32_t media_clamp(int32_t a, int32_t min, int32_t max);

/**
 * @brief 限制值在范围内（64位）
 * @param a 值
 * @param min 最小值
 * @param max 最大值
 * @return 限制后的值
 */
int64_t media_clamp64(int64_t a, int64_t min, int64_t max);

/**
 * @brief 获取最小值
 */
#define MEDIA_MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @brief 获取最大值
 */
#define MEDIA_MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @brief 获取绝对值
 */
#define MEDIA_ABS(a) ((a) >= 0 ? (a) : -(a))

/**
 * @brief 四舍五入除法
 * @param a 被除数
 * @param b 除数
 * @return 结果
 */
int64_t media_div_round(int64_t a, int64_t b);

/**
 * @brief 向上取整除法
 * @param a 被除数
 * @param b 除数
 * @return 结果
 */
int64_t media_div_ceil(int64_t a, int64_t b);

/**
 * @brief 向下取整除法
 * @param a 被除数
 * @param b 除数
 * @return 结果
 */
int64_t media_div_floor(int64_t a, int64_t b);

/* ============================================================================
 * 错误处理
 * ============================================================================ */

/**
 * @brief 获取错误描述
 * @param error_code 错误码
 * @return 错误描述字符串
 */
const char* media_error_string(MediaErrorCode error_code);

/**
 * @brief 获取错误描述（线程安全）
 * @param error_code 错误码
 * @param buf 缓冲区
 * @param buf_size 缓冲区大小
 * @return 错误描述字符串
 */
char* media_error_string_r(MediaErrorCode error_code, char *buf, size_t buf_size);

/* ============================================================================
 * 平台检测
 * ============================================================================ */

/**
 * @brief 检测是否为Windows平台
 * @return true表示是Windows
 */
bool media_is_windows(void);

/**
 * @brief 检测是否为Linux平台
 * @return true表示是Linux
 */
bool media_is_linux(void);

/**
 * @brief 检测是否为64位系统
 * @return true表示是64位
 */
bool media_is_64bit(void);

/**
 * @brief 获取CPU核心数
 * @return CPU核心数
 */
int media_cpu_count(void);

/**
 * @brief 获取系统内存大小
 * @return 内存大小（字节）
 */
int64_t media_system_memory(void);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_UTILS_H */
