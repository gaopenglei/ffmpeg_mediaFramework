/**
 * @file media_utils.c
 * @brief 媒体工具函数实现
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 */

#include "utils/media_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

/* 平台检测 */
#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#else
    #include <unistd.h>
    #include <sys/sysinfo.h>
    #include <pthread.h>
#endif

/* FFmpeg头文件 */
extern "C" {
#include <libavutil/error.h>
#include <libavutil/mem.h>
#include <libavutil/log.h>
}

/* ============================================================================
 * 日志系统
 * ============================================================================ */

static int g_log_level = MEDIA_LOG_LEVEL_INFO;
static MediaUtilsLogCallback g_log_callback = NULL;

void media_utils_set_log_level(int level)
{
    g_log_level = level;
    av_log_set_level(level);
}

void media_utils_set_log_callback(MediaUtilsLogCallback callback)
{
    g_log_callback = callback;
}

void media_utils_log(int level, const char *fmt, ...)
{
    if (level > g_log_level) {
        return;
    }
    
    va_list vl;
    va_start(vl, fmt);
    
    if (g_log_callback) {
        g_log_callback(NULL, level, fmt, vl);
    } else {
        /* 默认输出到stderr */
        const char *level_str = "";
        switch (level) {
            case MEDIA_LOG_LEVEL_PANIC:   level_str = "[PANIC] "; break;
            case MEDIA_LOG_LEVEL_FATAL:   level_str = "[FATAL] "; break;
            case MEDIA_LOG_LEVEL_ERROR:   level_str = "[ERROR] "; break;
            case MEDIA_LOG_LEVEL_WARNING: level_str = "[WARN]  "; break;
            case MEDIA_LOG_LEVEL_INFO:    level_str = "[INFO]  "; break;
            case MEDIA_LOG_LEVEL_DEBUG:   level_str = "[DEBUG] "; break;
            case MEDIA_LOG_LEVEL_TRACE:   level_str = "[TRACE] "; break;
            default: break;
        }
        
        fprintf(stderr, "%s", level_str);
        vfprintf(stderr, fmt, vl);
    }
    
    va_end(vl);
}

/* ============================================================================
 * 内存管理
 * ============================================================================ */

void* media_malloc(size_t size)
{
    return av_malloc(size);
}

void* media_mallocz(size_t size)
{
    return av_mallocz(size);
}

void* media_calloc(size_t count, size_t size)
{
    return av_calloc(count, size);
}

void* media_realloc(void *ptr, size_t size)
{
    return av_realloc(ptr, size);
}

void* media_reallocp(void *ptr, size_t size)
{
    return av_reallocp(ptr, size);
}

void media_free(void *ptr)
{
    av_free(ptr);
}

void media_freep(void **ptr)
{
    av_freep(ptr);
}

char* media_strdup(const char *str)
{
    return av_strdup(str);
}

char* media_strndup(const char *str, size_t size)
{
    return av_strndup(str, size);
}

/* ============================================================================
 * 字符串处理
 * ============================================================================ */

char* media_stristr(const char *haystack, const char *needle)
{
    if (!haystack || !needle) {
        return NULL;
    }
    
    const char *h = haystack;
    const char *n = needle;
    
    while (*h) {
        if (tolower(*h) == tolower(*n)) {
            const char *h2 = h;
            const char *n2 = n;
            
            while (*h2 && *n2 && tolower(*h2) == tolower(*n2)) {
                h2++;
                n2++;
            }
            
            if (*n2 == '\0') {
                return (char*)h;
            }
        }
        h++;
    }
    
    return NULL;
}

size_t media_strlcpy(char *dst, const char *src, size_t size)
{
    size_t src_len = strlen(src);
    
    if (size > 0) {
        size_t copy_len = (src_len < size - 1) ? src_len : size - 1;
        memcpy(dst, src, copy_len);
        dst[copy_len] = '\0';
    }
    
    return src_len;
}

size_t media_strlcat(char *dst, const char *src, size_t size)
{
    size_t dst_len = strlen(dst);
    size_t src_len = strlen(src);
    
    if (dst_len >= size) {
        return size + src_len;
    }
    
    size_t remaining = size - dst_len - 1;
    size_t copy_len = (src_len < remaining) ? src_len : remaining;
    
    memcpy(dst + dst_len, src, copy_len);
    dst[dst_len + copy_len] = '\0';
    
    return dst_len + src_len;
}

int media_strcasecmp(const char *a, const char *b)
{
    if (!a || !b) {
        return (a != NULL) - (b != NULL);
    }
    
    while (*a && *b) {
        int diff = tolower(*a) - tolower(*b);
        if (diff != 0) {
            return diff;
        }
        a++;
        b++;
    }
    
    return tolower(*a) - tolower(*b);
}

int media_strncasecmp(const char *a, const char *b, size_t n)
{
    if (!a || !b) {
        return (a != NULL) - (b != NULL);
    }
    
    while (n > 0 && *a && *b) {
        int diff = tolower(*a) - tolower(*b);
        if (diff != 0) {
            return diff;
        }
        a++;
        b++;
        n--;
    }
    
    if (n == 0) {
        return 0;
    }
    
    return tolower(*a) - tolower(*b);
}

/* ============================================================================
 * 时间函数
 * ============================================================================ */

int64_t media_gettime_us(void)
{
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (int64_t)((counter.QuadPart * 1000000LL) / frequency.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL;
#endif
}

int64_t media_gettime_ns(void)
{
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (int64_t)((counter.QuadPart * 1000000000LL) / frequency.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
}

void media_sleep_us(int64_t us)
{
#ifdef _WIN32
    Sleep((DWORD)(us / 1000));
#else
    usleep((useconds_t)us);
#endif
}

void media_sleep_ms(int64_t ms)
{
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep((useconds_t)(ms * 1000));
#endif
}

char* media_time_to_string(int64_t time_us, char *buf, size_t buf_size)
{
    if (!buf || buf_size < 16) {
        return NULL;
    }
    
    int64_t seconds = time_us / 1000000;
    int64_t minutes = seconds / 60;
    int64_t hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    snprintf(buf, buf_size, "%02lld:%02lld:%02lld", 
             (long long)hours, (long long)minutes, (long long)seconds);
    
    return buf;
}

int64_t media_string_to_time(const char *time_str)
{
    if (!time_str) {
        return 0;
    }
    
    int hours = 0, minutes = 0;
    double seconds = 0.0;
    
    if (sscanf(time_str, "%d:%d:%lf", &hours, &minutes, &seconds) == 3) {
        return (int64_t)((hours * 3600 + minutes * 60 + seconds) * 1000000);
    } else if (sscanf(time_str, "%d:%lf", &minutes, &seconds) == 2) {
        return (int64_t)((minutes * 60 + seconds) * 1000000);
    } else if (sscanf(time_str, "%lf", &seconds) == 1) {
        return (int64_t)(seconds * 1000000);
    }
    
    return 0;
}

/* ============================================================================
 * 数学函数
 * ============================================================================ */

int64_t media_rescale_q(int64_t a, MediaRational bq, MediaRational cq)
{
    return av_rescale_q(a, (AVRational){bq.num, bq.den}, (AVRational){cq.num, cq.den});
}

int64_t media_rescale_q_rnd(int64_t a, MediaRational bq, MediaRational cq,
                            enum AVRounding rnd)
{
    return av_rescale_q_rnd(a, (AVRational){bq.num, bq.den}, 
                            (AVRational){cq.num, cq.den}, rnd);
}

int64_t media_rescale(int64_t a, int64_t b, int64_t c)
{
    return av_rescale(a, b, c);
}

int64_t media_clip_int64(int64_t a, int64_t amin, int64_t amax)
{
    if (a < amin) return amin;
    if (a > amax) return amax;
    return a;
}

int32_t media_clip_int32(int32_t a, int32_t amin, int32_t amax)
{
    if (a < amin) return amin;
    if (a > amax) return amax;
    return a;
}

int64_t media_div_round(int64_t a, int64_t b)
{
    return (a + (b >> 1)) / b;
}

int64_t media_div_ceil(int64_t a, int64_t b)
{
    return (a + b - 1) / b;
}

int64_t media_div_floor(int64_t a, int64_t b)
{
    return a / b;
}

/* ============================================================================
 * 错误处理
 * ============================================================================ */

const char* media_error_string(MediaErrorCode error_code)
{
    switch (error_code) {
        case MEDIA_SUCCESS:             return "Success";
        case MEDIA_NEED_MORE_DATA:      return "Need more data";
        case MEDIA_EOF:                 return "End of file";
        case MEDIA_BUFFER_FULL:         return "Buffer full";
        
        case MEDIA_ERROR_UNKNOWN:       return "Unknown error";
        case MEDIA_ERROR_INVALID_PARAM: return "Invalid parameter";
        case MEDIA_ERROR_NULL_POINTER:  return "Null pointer";
        case MEDIA_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case MEDIA_ERROR_NOT_SUPPORTED: return "Not supported";
        case MEDIA_ERROR_NOT_INITIALIZED: return "Not initialized";
        case MEDIA_ERROR_ALREADY_INITIALIZED: return "Already initialized";
        case MEDIA_ERROR_TIMEOUT:       return "Timeout";
        case MEDIA_ERROR_INTERRUPTED:   return "Interrupted";
        
        case MEDIA_ERROR_FILE_NOT_FOUND: return "File not found";
        case MEDIA_ERROR_FILE_OPEN_FAILED: return "Failed to open file";
        case MEDIA_ERROR_FILE_READ_FAILED: return "Failed to read file";
        case MEDIA_ERROR_FILE_WRITE_FAILED: return "Failed to write file";
        case MEDIA_ERROR_FILE_SEEK_FAILED: return "Failed to seek file";
        
        case MEDIA_ERROR_STREAM_NOT_FOUND: return "Stream not found";
        case MEDIA_ERROR_CODEC_NOT_FOUND: return "Codec not found";
        case MEDIA_ERROR_CODEC_OPEN_FAILED: return "Failed to open codec";
        case MEDIA_ERROR_DECODE_FAILED:  return "Decode failed";
        case MEDIA_ERROR_ENCODE_FAILED:  return "Encode failed";
        
        case MEDIA_ERROR_DEVICE_OPEN_FAILED: return "Failed to open device";
        case MEDIA_ERROR_DEVICE_BUSY:    return "Device busy";
        
        case MEDIA_ERROR_FILTER_INIT_FAILED: return "Failed to initialize filter";
        case MEDIA_ERROR_FILTER_PROCESS_FAILED: return "Filter process failed";
        
        case MEDIA_ERROR_INVALID_STATE:  return "Invalid state";
        case MEDIA_ERROR_OPERATION_FAILED: return "Operation failed";
        
        default: return "Unknown error code";
    }
}

char* media_error_string_r(MediaErrorCode error_code, char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) {
        return NULL;
    }
    
    media_strlcpy(buf, media_error_string(error_code), buf_size);
    return buf;
}

/* ============================================================================
 * 平台检测
 * ============================================================================ */

bool media_is_windows(void)
{
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

bool media_is_linux(void)
{
#ifdef __linux__
    return true;
#else
    return false;
#endif
}

bool media_is_64bit(void)
{
    return sizeof(void*) == 8;
}

int media_cpu_count(void)
{
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

int64_t media_system_memory(void)
{
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return (int64_t)status.ullTotalPhys;
#else
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return (int64_t)info.totalram * info.mem_unit;
    }
    return 0;
#endif
}
