/**
 * @file media_capture.h
 * @brief 媒体设备捕获接口
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 本文件定义了从摄像头、麦克风等设备实时捕获音视频数据的接口。
 * 支持Windows和Linux平台。
 */

#ifndef MEDIA_CAPTURE_H
#define MEDIA_CAPTURE_H

#include "core/media_types.h"
#include "core/media_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 捕获器结构体（不透明指针）
 */
typedef struct MediaCapture MediaCapture;

/**
 * @brief 设备类型
 */
typedef enum {
    MEDIA_DEVICE_TYPE_UNKNOWN = 0,
    MEDIA_DEVICE_TYPE_VIDEO = 1,    ///< 视频设备（摄像头）
    MEDIA_DEVICE_TYPE_AUDIO = 2,    ///< 音频设备（麦克风）
    MEDIA_DEVICE_TYPE_SCREEN = 3,   ///< 屏幕捕获
    MEDIA_DEVICE_TYPE_WINDOW = 4,   ///< 窗口捕获
} MediaDeviceType;

/**
 * @brief 捕获器状态
 */
typedef enum {
    MEDIA_CAPTURE_STATE_STOPPED = 0,
    MEDIA_CAPTURE_STATE_STARTING = 1,
    MEDIA_CAPTURE_STATE_CAPTURING = 2,
    MEDIA_CAPTURE_STATE_PAUSED = 3,
    MEDIA_CAPTURE_STATE_ERROR = 4,
} MediaCaptureState;

/**
 * @brief 设备信息
 */
typedef struct {
    char device_id[256];          ///< 设备ID
    char name[256];               ///< 设备名称
    char description[512];        ///< 设备描述
    MediaDeviceType type;         ///< 设备类型
    int32_t is_default;           ///< 是否为默认设备
    int32_t supported_formats_count; ///< 支持的格式数量
    char **supported_formats;     ///< 支持的格式列表
} MediaDeviceInfo;

/**
 * @brief 视频捕获配置
 */
typedef struct {
    int32_t width;                ///< 视频宽度
    int32_t height;               ///< 视频高度
    MediaPixelFormat pixel_format;///< 像素格式
    MediaRational frame_rate;     ///< 帧率
    int32_t buffer_count;         ///< 缓冲区数量
    int32_t use_hw_accel;         ///< 是否使用硬件加速
    char hw_device[256];          ///< 硬件设备路径
} VideoCaptureConfig;

/**
 * @brief 音频捕获配置
 */
typedef struct {
    int32_t sample_rate;          ///< 采样率
    int32_t channels;             ///< 声道数
    MediaSampleFormat sample_format; ///< 采样格式
    int32_t buffer_size;          ///< 缓冲区大小（帧数）
    int32_t latency_ms;           ///< 延迟（毫秒）
} AudioCaptureConfig;

/**
 * @brief 捕获配置
 */
typedef struct {
    MediaDeviceType type;         ///< 设备类型
    char device_id[256];          ///< 设备ID
    union {
        VideoCaptureConfig video; ///< 视频配置
        AudioCaptureConfig audio; ///< 音频配置
    } config;
    int32_t thread_count;         ///< 线程数
    void *user_data;              ///< 用户数据
} MediaCaptureConfig;

/**
 * @brief 捕获回调函数类型
 * @param capture 捕获器指针
 * @param frame 捕获的帧
 * @param user_data 用户数据
 * @return 成功返回MEDIA_SUCCESS
 */
typedef MediaErrorCode (*MediaCaptureCallback)(MediaCapture *capture, 
                                                MediaFrame *frame, 
                                                void *user_data);

/* ============================================================================
 * 设备枚举
 * ============================================================================ */

/**
 * @brief 枚举所有视频设备
 * @param devices 设备信息数组（输出）
 * @param max_count 数组最大容量
 * @param actual_count 实际设备数量（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_enum_video_devices(MediaDeviceInfo *devices, 
                                                 int32_t max_count, 
                                                 int32_t *actual_count);

/**
 * @brief 枚举所有音频设备
 * @param devices 设备信息数组（输出）
 * @param max_count 数组最大容量
 * @param actual_count 实际设备数量（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_enum_audio_devices(MediaDeviceInfo *devices, 
                                                 int32_t max_count, 
                                                 int32_t *actual_count);

/**
 * @brief 枚举所有设备
 * @param type 设备类型
 * @param devices 设备信息数组（输出）
 * @param max_count 数组最大容量
 * @param actual_count 实际设备数量（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_enum_devices(MediaDeviceType type, 
                                           MediaDeviceInfo *devices, 
                                           int32_t max_count, 
                                           int32_t *actual_count);

/**
 * @brief 获取默认视频设备
 * @param device_info 设备信息（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_get_default_video_device(MediaDeviceInfo *device_info);

/**
 * @brief 获取默认音频设备
 * @param device_info 设备信息（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_get_default_audio_device(MediaDeviceInfo *device_info);

/**
 * @brief 释放设备信息
 * @param device_info 设备信息指针
 */
void media_capture_free_device_info(MediaDeviceInfo *device_info);

/* ============================================================================
 * 捕获器创建与销毁
 * ============================================================================ */

/**
 * @brief 创建捕获器
 * @param type 设备类型
 * @return 捕获器指针，失败返回NULL
 */
MediaCapture* media_capture_create(MediaDeviceType type);

/**
 * @brief 创建带配置的捕获器
 * @param config 配置结构体
 * @return 捕获器指针，失败返回NULL
 */
MediaCapture* media_capture_create_with_config(const MediaCaptureConfig *config);

/**
 * @brief 释放捕获器
 * @param capture 捕获器指针
 */
void media_capture_free(MediaCapture *capture);

/**
 * @brief 重置捕获配置为默认值
 * @param config 配置结构体指针
 * @param type 设备类型
 */
void media_capture_config_default(MediaCaptureConfig *config, MediaDeviceType type);

/* ============================================================================
 * 设备打开与关闭
 * ============================================================================ */

/**
 * @brief 打开设备
 * @param capture 捕获器指针
 * @param device_id 设备ID（NULL使用默认设备）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_open(MediaCapture *capture, const char *device_id);

/**
 * @brief 打开设备（带配置）
 * @param capture 捕获器指针
 * @param config 配置结构体
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_open_with_config(MediaCapture *capture, 
                                               const MediaCaptureConfig *config);

/**
 * @brief 关闭设备
 * @param capture 捕获器指针
 */
void media_capture_close(MediaCapture *capture);

/**
 * @brief 检查设备是否已打开
 * @param capture 捕获器指针
 * @return true表示已打开
 */
bool media_capture_is_open(MediaCapture *capture);

/**
 * @brief 获取捕获器状态
 * @param capture 捕获器指针
 * @return 状态值
 */
MediaCaptureState media_capture_get_state(MediaCapture *capture);

/* ============================================================================
 * 捕获控制
 * ============================================================================ */

/**
 * @brief 开始捕获
 * @param capture 捕获器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_start(MediaCapture *capture);

/**
 * @brief 停止捕获
 * @param capture 捕获器指针
 */
void media_capture_stop(MediaCapture *capture);

/**
 * @brief 暂停捕获
 * @param capture 捕获器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_pause(MediaCapture *capture);

/**
 * @brief 恢复捕获
 * @param capture 捕获器指针
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_resume(MediaCapture *capture);

/**
 * @brief 设置捕获回调
 * @param capture 捕获器指针
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void media_capture_set_callback(MediaCapture *capture, 
                                 MediaCaptureCallback callback, 
                                 void *user_data);

/* ============================================================================
 * 捕获参数获取
 * ============================================================================ */

/**
 * @brief 获取视频参数
 * @param capture 捕获器指针
 * @param params 视频参数（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_get_video_params(MediaCapture *capture, 
                                               VideoParams *params);

/**
 * @brief 获取音频参数
 * @param capture 捕获器指针
 * @param params 音频参数（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_get_audio_params(MediaCapture *capture, 
                                               AudioParams *params);

/**
 * @brief 获取设备类型
 * @param capture 捕获器指针
 * @return 设备类型
 */
MediaDeviceType media_capture_get_device_type(MediaCapture *capture);

/**
 * @brief 获取设备名称
 * @param capture 捕获器指针
 * @return 设备名称
 */
const char* media_capture_get_device_name(MediaCapture *capture);

/* ============================================================================
 * 捕获参数设置
 * ============================================================================ */

/**
 * @brief 设置视频参数
 * @param capture 捕获器指针
 * @param params 视频参数
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_set_video_params(MediaCapture *capture, 
                                               const VideoParams *params);

/**
 * @brief 设置音频参数
 * @param capture 捕获器指针
 * @param params 音频参数
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_set_audio_params(MediaCapture *capture, 
                                               const AudioParams *params);

/**
 * @brief 设置帧率
 * @param capture 捕获器指针
 * @param frame_rate 帧率
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_set_frame_rate(MediaCapture *capture, 
                                             MediaRational frame_rate);

/**
 * @brief 设置分辨率
 * @param capture 捕获器指针
 * @param width 宽度
 * @param height 高度
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_set_resolution(MediaCapture *capture, 
                                             int32_t width, int32_t height);

/* ============================================================================
 * 屏幕捕获
 * ============================================================================ */

/**
 * @brief 枚举显示器
 * @param displays 显示器信息数组（输出）
 * @param max_count 数组最大容量
 * @param actual_count 实际数量（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_enum_displays(MediaDeviceInfo *displays, 
                                            int32_t max_count, 
                                            int32_t *actual_count);

/**
 * @brief 枚举窗口
 * @param windows 窗口信息数组（输出）
 * @param max_count 数组最大容量
 * @param actual_count 实际数量（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_enum_windows(MediaDeviceInfo *windows, 
                                           int32_t max_count, 
                                           int32_t *actual_count);

/**
 * @brief 设置捕获区域
 * @param capture 捕获器指针
 * @param rect 捕获区域（屏幕坐标）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_set_capture_region(MediaCapture *capture, 
                                                 MediaRect rect);

/**
 * @brief 设置捕获光标
 * @param capture 捕获器指针
 * @param show_cursor 是否显示光标
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_capture_set_show_cursor(MediaCapture *capture, 
                                              bool show_cursor);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_CAPTURE_H */
