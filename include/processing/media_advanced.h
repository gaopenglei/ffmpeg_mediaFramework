/**
 * @file media_advanced.h
 * @brief 高级功能接口
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 本文件定义了高级功能接口，包括多声道处理、音频混合、360°视频处理等。
 */

#ifndef MEDIA_ADVANCED_H
#define MEDIA_ADVANCED_H

#include "core/media_types.h"
#include "core/media_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 多声道处理
 * ============================================================================ */

/**
 * @brief 声道布局定义
 */
typedef enum {
    MEDIA_CH_LAYOUT_MONO = 0x4,            ///< 单声道
    MEDIA_CH_LAYOUT_STEREO = 0x3,          ///< 立体声
    MEDIA_CH_LAYOUT_2POINT1 = 0xB,         ///< 2.1声道
    MEDIA_CH_LAYOUT_2_1 = 0x103,           ///< 2.1声道（另一种）
    MEDIA_CH_LAYOUT_SURROUND = 0x7,        ///< 环绕声
    MEDIA_CH_LAYOUT_4POINT0 = 0x33,        ///< 4.0声道
    MEDIA_CH_LAYOUT_4POINT1 = 0x3B,        ///< 4.1声道
    MEDIA_CH_LAYOUT_5POINT0 = 0x37,        ///< 5.0声道
    MEDIA_CH_LAYOUT_5POINT1 = 0x3F,        ///< 5.1声道
    MEDIA_CH_LAYOUT_6POINT0 = 0x137,       ///< 6.0声道
    MEDIA_CH_LAYOUT_6POINT1 = 0x13F,       ///< 6.1声道
    MEDIA_CH_LAYOUT_7POINT0 = 0x637,       ///< 7.0声道
    MEDIA_CH_LAYOUT_7POINT1 = 0x63F,       ///< 7.1声道
    MEDIA_CH_LAYOUT_7POINT1_WIDE = 0x1C3F, ///< 7.1宽声道
} MediaChannelLayout;

/**
 * @brief 声道位置
 */
typedef enum {
    MEDIA_CH_FRONT_LEFT = 0x1,
    MEDIA_CH_FRONT_RIGHT = 0x2,
    MEDIA_CH_FRONT_CENTER = 0x4,
    MEDIA_CH_LOW_FREQUENCY = 0x8,
    MEDIA_CH_BACK_LEFT = 0x10,
    MEDIA_CH_BACK_RIGHT = 0x20,
    MEDIA_CH_FRONT_LEFT_OF_CENTER = 0x40,
    MEDIA_CH_FRONT_RIGHT_OF_CENTER = 0x80,
    MEDIA_CH_BACK_CENTER = 0x100,
    MEDIA_CH_SIDE_LEFT = 0x200,
    MEDIA_CH_SIDE_RIGHT = 0x400,
    MEDIA_CH_TOP_CENTER = 0x800,
    MEDIA_CH_TOP_FRONT_LEFT = 0x1000,
    MEDIA_CH_TOP_FRONT_CENTER = 0x2000,
    MEDIA_CH_TOP_FRONT_RIGHT = 0x4000,
    MEDIA_CH_TOP_BACK_LEFT = 0x8000,
    MEDIA_CH_TOP_BACK_CENTER = 0x10000,
    MEDIA_CH_TOP_BACK_RIGHT = 0x20000,
} MediaChannelPosition;

/**
 * @brief 声道映射配置
 */
typedef struct {
    int32_t input_channels;         ///< 输入声道数
    int32_t output_channels;        ///< 输出声道数
    int32_t input_layout;           ///< 输入声道布局
    int32_t output_layout;          ///< 输出声道布局
    int32_t *channel_map;           ///< 声道映射表
    int32_t map_size;               ///< 映射表大小
} ChannelMapConfig;

/**
 * @brief 创建声道映射配置
 * @param input_layout 输入声道布局
 * @param output_layout 输出声道布局
 * @return 配置指针，失败返回NULL
 */
ChannelMapConfig* channel_map_config_create(int32_t input_layout, 
                                             int32_t output_layout);

/**
 * @brief 释放声道映射配置
 * @param config 配置指针
 */
void channel_map_config_free(ChannelMapConfig *config);

/**
 * @brief 重映射声道
 * @param frame 音频帧
 * @param config 声道映射配置
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_audio_remap_channels(MediaFrame *frame, 
                                           const ChannelMapConfig *config);

/**
 * @brief 提取声道
 * @param frame 音频帧
 * @param channel 声道位置
 * @param output 输出帧（单声道）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_audio_extract_channel(const MediaFrame *frame, 
                                            MediaChannelPosition channel, 
                                            MediaFrame *output);

/**
 * @brief 合并声道
 * @param frames 帧数组
 * @param count 帧数量
 * @param output_layout 输出声道布局
 * @param output 输出帧
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_audio_merge_channels(const MediaFrame **frames, 
                                           int32_t count, 
                                           int32_t output_layout, 
                                           MediaFrame *output);

/**
 * @brief 获取声道数
 * @param layout 声道布局
 * @return 声道数
 */
int32_t media_channel_layout_get_count(int32_t layout);

/**
 * @brief 获取声道布局名称
 * @param layout 声道布局
 * @return 名称字符串
 */
const char* media_channel_layout_get_name(int32_t layout);

/* ============================================================================
 * 音频混合
 * ============================================================================ */

/**
 * @brief 音频混合器结构体（不透明指针）
 */
typedef struct AudioMixer AudioMixer;

/**
 * @brief 混音轨道配置
 */
typedef struct {
    int32_t track_id;               ///< 轨道ID
    double volume;                  ///< 音量 (0.0 ~ 2.0)
    double pan;                     ///< 声像 (-1.0 ~ 1.0)
    int64_t start_time;             ///< 起始时间（采样）
    int64_t end_time;               ///< 结束时间（采样）
    int32_t muted;                  ///< 静音
    int32_t solo;                   ///< 独奏
    double fade_in;                 ///< 淡入时长（秒）
    double fade_out;                ///< 淡出时长（秒）
} AudioTrackConfig;

/**
 * @brief 创建音频混合器
 * @param sample_rate 采样率
 * @param channels 声道数
 * @param sample_format 采样格式
 * @return 混合器指针，失败返回NULL
 */
AudioMixer* audio_mixer_create(int32_t sample_rate, 
                                int32_t channels, 
                                MediaSampleFormat sample_format);

/**
 * @brief 释放音频混合器
 * @param mixer 混合器指针
 */
void audio_mixer_free(AudioMixer *mixer);

/**
 * @brief 添加音轨
 * @param mixer 混合器指针
 * @param config 轨道配置
 * @return 轨道ID，失败返回-1
 */
int32_t audio_mixer_add_track(AudioMixer *mixer, 
                               const AudioTrackConfig *config);

/**
 * @brief 移除音轨
 * @param mixer 混合器指针
 * @param track_id 轨道ID
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode audio_mixer_remove_track(AudioMixer *mixer, int32_t track_id);

/**
 * @brief 设置轨道音量
 * @param mixer 混合器指针
 * @param track_id 轨道ID
 * @param volume 音量
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode audio_mixer_set_track_volume(AudioMixer *mixer, 
                                             int32_t track_id, 
                                             double volume);

/**
 * @brief 设置轨道声像
 * @param mixer 混合器指针
 * @param track_id 轨道ID
 * @param pan 声像
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode audio_mixer_set_track_pan(AudioMixer *mixer, 
                                          int32_t track_id, 
                                          double pan);

/**
 * @brief 设置轨道静音
 * @param mixer 混合器指针
 * @param track_id 轨道ID
 * @param muted 静音状态
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode audio_mixer_set_track_muted(AudioMixer *mixer, 
                                            int32_t track_id, 
                                            bool muted);

/**
 * @brief 添加帧到轨道
 * @param mixer 混合器指针
 * @param track_id 轨道ID
 * @param frame 音频帧
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode audio_mixer_add_frame(AudioMixer *mixer, 
                                      int32_t track_id, 
                                      const MediaFrame *frame);

/**
 * @brief 获取混合输出
 * @param mixer 混合器指针
 * @param output 输出帧
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode audio_mixer_get_output(AudioMixer *mixer, MediaFrame *output);

/**
 * @brief 清空所有轨道数据
 * @param mixer 混合器指针
 */
void audio_mixer_clear(AudioMixer *mixer);

/**
 * @brief 重置轨道配置为默认值
 * @param config 配置结构体指针
 */
void audio_track_config_default(AudioTrackConfig *config);

/* ============================================================================
 * 360°视频处理
 * ============================================================================ */

/**
 * @brief 360°视频投影格式
 */
typedef enum {
    MEDIA_PROJ_EQUIRECTANGULAR = 0,    ///< 等距圆柱投影
    MEDIA_PROJ_CUBEMAP = 1,            ///< 立方体贴图
    MEDIA_PROJ_EQUIANGULAR = 2,        ///< 等角立方体贴图
    MEDIA_PROJ_HEMISPHERE = 3,         ///< 半球投影
    MEDIA_PROJ_FISHEYE = 4,            ///< 鱼眼投影
} MediaProjectionFormat;

/**
 * @brief 360°视频元数据
 */
typedef struct {
    MediaProjectionFormat projection;  ///< 投影格式
    double yaw;                        ///< 偏航角
    double pitch;                      ///< 俯仰角
    double roll;                       ///< 翻滚角
    double fov;                        ///< 视场角
    int32_t stereo_mode;               ///< 立体模式
    double initial_yaw;                ///< 初始偏航角
    double initial_pitch;              ///< 初始俯仰角
    double initial_fov;                ///< 初始视场角
} Video360Metadata;

/**
 * @brief 360°视频热点
 */
typedef struct {
    char id[64];                    ///< 热点ID
    char name[256];                 ///< 热点名称
    double yaw;                     ///< 偏航角
    double pitch;                   ///< 俯仰角
    double radius;                  ///< 半径
    int32_t type;                   ///< 类型 (0=信息, 1=链接, 2=交互)
    char data[1024];                ///< 附加数据
    int64_t start_time;             ///< 起始时间
    int64_t end_time;               ///< 结束时间
} Video360Hotspot;

/**
 * @brief 360°视频处理配置
 */
typedef struct {
    MediaProjectionFormat input_format;  ///< 输入投影格式
    MediaProjectionFormat output_format; ///< 输出投影格式
    int32_t cubemap_size;                ///< 立方体贴图尺寸
    int32_t output_width;                ///< 输出宽度
    int32_t output_height;               ///< 输出高度
    double yaw;                          ///< 偏航角
    double pitch;                        ///< 俯仰角
    double roll;                         ///< 翻滚角
    double fov;                          ///< 视场角
    int32_t interpolation;               ///< 插值方法
} Video360Config;

/**
 * @brief 创建360°视频处理器
 * @param config 处理配置
 * @return 处理器指针，失败返回NULL
 */
void* media_video360_create(const Video360Config *config);

/**
 * @brief 释放360°视频处理器
 * @param processor 处理器指针
 */
void media_video360_free(void *processor);

/**
 * @brief 投影转换
 * @param processor 处理器指针
 * @param input 输入帧
 * @param output 输出帧
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_video360_convert(void *processor, 
                                       const MediaFrame *input, 
                                       MediaFrame *output);

/**
 * @brief 设置视角
 * @param processor 处理器指针
 * @param yaw 偏航角
 * @param pitch 俯仰角
 * @param fov 视场角
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_video360_set_view(void *processor, 
                                        double yaw, double pitch, double fov);

/**
 * @brief 提取视角区域
 * @param processor 处理器指针
 * @param input 输入帧
 * @param output 输出帧（普通视频）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_video360_extract_view(void *processor, 
                                            const MediaFrame *input, 
                                            MediaFrame *output);

/**
 * @brief 添加热点
 * @param processor 处理器指针
 * @param hotspot 热点配置
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_video360_add_hotspot(void *processor, 
                                           const Video360Hotspot *hotspot);

/**
 * @brief 移除热点
 * @param processor 处理器指针
 * @param hotspot_id 热点ID
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_video360_remove_hotspot(void *processor, 
                                              const char *hotspot_id);

/**
 * @brief 渲染热点
 * @param processor 处理器指针
 * @param frame 帧指针（会被修改）
 * @param timestamp 当前时间戳
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_video360_render_hotspots(void *processor, 
                                               MediaFrame *frame, 
                                               int64_t timestamp);

/**
 * @brief 添加引导线
 * @param processor 处理器指针
 * @param start_yaw 起始偏航角
 * @param start_pitch 起始俯仰角
 * @param end_yaw 结束偏航角
 * @param end_pitch 结束俯仰角
 * @param color 颜色（ARGB格式）
 * @param width 线宽
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_video360_add_guide_line(void *processor, 
                                              double start_yaw, double start_pitch, 
                                              double end_yaw, double end_pitch, 
                                              uint32_t color, int32_t width);

/**
 * @brief 获取投影格式名称
 * @param format 投影格式
 * @return 名称字符串
 */
const char* media_projection_format_name(MediaProjectionFormat format);

/**
 * @brief 重置360°视频配置为默认值
 * @param config 配置结构体指针
 */
void media_video360_config_default(Video360Config *config);

/**
 * @brief 重置360°视频元数据为默认值
 * @param metadata 元数据结构体指针
 */
void media_video360_metadata_default(Video360Metadata *metadata);

/**
 * @brief 重置360°视频热点配置为默认值
 * @param hotspot 热点结构体指针
 */
void media_video360_hotspot_default(Video360Hotspot *hotspot);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_ADVANCED_H */
