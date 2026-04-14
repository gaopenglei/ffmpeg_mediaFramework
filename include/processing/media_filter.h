/**
 * @file media_filter.h
 * @brief 媒体滤镜接口
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 本文件定义了视频和音频滤镜的接口。
 * 支持色彩调整、几何变换、模糊锐化、音频均衡器等滤镜。
 */

#ifndef MEDIA_FILTER_H
#define MEDIA_FILTER_H

#include "core/media_types.h"
#include "core/media_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 滤镜上下文结构体（不透明指针）
 */
typedef struct MediaFilterContext MediaFilterContext;

/**
 * @brief 滤镜图结构体（不透明指针）
 */
typedef struct MediaFilterGraph MediaFilterGraph;

/**
 * @brief 滤镜类型
 */
typedef enum {
    MEDIA_FILTER_TYPE_UNKNOWN = 0,
    MEDIA_FILTER_TYPE_VIDEO = 1,
    MEDIA_FILTER_TYPE_AUDIO = 2,
} MediaFilterType;

/* ============================================================================
 * 视频滤镜参数结构体
 * ============================================================================ */

/**
 * @brief 色彩调整参数
 */
typedef struct {
    double brightness;      ///< 亮度调整 (-1.0 ~ 1.0)
    double contrast;        ///< 对比度调整 (0.0 ~ 2.0)
    double saturation;      ///< 饱和度调整 (0.0 ~ 3.0)
    double hue;             ///< 色调调整 (0 ~ 360)
    double gamma;           ///< 伽马值 (0.1 ~ 10.0)
    int32_t invert;         ///< 反色
} ColorAdjustParams;

/**
 * @brief 几何变换参数
 */
typedef struct {
    MediaRotation rotation; ///< 旋转角度
    MediaFlipDirection flip;///< 翻转方向
    double angle;           ///< 任意角度旋转
    double shear_x;         ///< X方向倾斜
    double shear_y;         ///< Y方向倾斜
    double scale_x;         ///< X方向缩放
    double scale_y;         ///< Y方向缩放
    double translate_x;     ///< X方向平移
    double translate_y;     ///< Y方向平移
} GeometricParams;

/**
 * @brief 模糊参数
 */
typedef struct {
    int32_t type;           ///< 模糊类型 (0=高斯, 1=均值, 2=中值)
    int32_t kernel_size;    ///< 核大小
    double sigma;           ///< 高斯sigma值
    int32_t iterations;     ///< 迭代次数
} BlurParams;

/**
 * @brief 锐化参数
 */
typedef struct {
    double amount;          ///< 锐化强度
    double threshold;       ///< 阈值
    int32_t kernel_size;    ///< 核大小
} SharpenParams;

/**
 * @brief 去噪参数
 */
typedef struct {
    int32_t type;           ///< 去噪类型 (0=均值, 1=中值, 2=双边)
    double strength;        ///< 去噪强度
    int32_t window_size;    ///< 窗口大小
    double sigma_color;     ///< 颜色sigma（双边滤波）
    double sigma_space;     ///< 空间sigma（双边滤波）
} DenoiseParams;

/**
 * @brief 边缘检测参数
 */
typedef struct {
    int32_t type;           ///< 边缘检测类型 (0=Sobel, 1=Canny, 2=Laplacian)
    double threshold1;      ///< 低阈值
    double threshold2;      ///< 高阈值
    int32_t aperture_size;  ///< 孔径大小
} EdgeDetectParams;

/**
 * @brief 色彩空间转换参数
 */
typedef struct {
    const char *input_space;   ///< 输入色彩空间
    const char *output_space;  ///< 输出色彩空间
    const char *input_range;   ///< 输入范围 (tv/pc)
    const char *output_range;  ///< 输出范围 (tv/pc)
    const char *input_primaries;  ///< 输入原色
    const char *output_primaries; ///< 输出原色
    const char *input_trc;     ///< 输入传输特性
    const char *output_trc;    ///< 输出传输特性
} ColorSpaceParams;

/* ============================================================================
 * 音频滤镜参数结构体
 * ============================================================================ */

/**
 * @brief 均衡器参数
 */
typedef struct {
    double gains[10];       ///< 各频段增益 (dB)
    double frequencies[10]; ///< 各频段频率 (Hz)
    int32_t band_count;     ///< 频段数量
    double q_factor;        ///< Q因子
} EqualizerParams;

/**
 * @brief 混响参数
 */
typedef struct {
    double room_size;       ///< 房间大小 (0.0 ~ 1.0)
    double damping;         ///< 阻尼 (0.0 ~ 1.0)
    double wet_level;       ///< 湿信号电平 (0.0 ~ 1.0)
    double dry_level;       ///< 干信号电平 (0.0 ~ 1.0)
    double width;           ///< 立体声宽度 (0.0 ~ 1.0)
    double pre_delay;       ///< 预延迟 (毫秒)
    double decay_time;      ///< 衰减时间 (秒)
} ReverbParams;

/**
 * @brief 回声参数
 */
typedef struct {
    double delay;           ///< 延迟时间 (毫秒)
    double decay;           ///< 衰减系数 (0.0 ~ 1.0)
    int32_t echoes;         ///< 回声次数
    double volume;          ///< 回声音量 (0.0 ~ 1.0)
} EchoParams;

/**
 * @brief 音频降噪参数
 */
typedef struct {
    double noise_reduction; ///< 降噪强度 (dB)
    double noise_floor;     ///< 噪声底噪 (dB)
    int32_t profile_mode;   ///< 配置文件模式
    const char *profile_file; ///< 噪声配置文件
} AudioDenoiseParams;

/**
 * @brief 音频压缩参数
 */
typedef struct {
    double threshold;       ///< 阈值 (dB)
    double ratio;           ///< 压缩比
    double attack;          ///< 起始时间 (毫秒)
    double release;         ///< 释放时间 (毫秒)
    double makeup_gain;     ///< 补偿增益 (dB)
    double knee;            ///< 膝点 (dB)
} CompressorParams;

/**
 * @brief 音频标准化参数
 */
typedef struct {
    double target_level;    ///< 目标电平 (dB)
    double peak_level;      ///< 峰值电平 (dB)
    int32_t mode;           ///< 模式 (0=峰值, 1=RMS, 2=响度)
    double loudness_target; ///< 响度目标 (LUFS)
} NormalizeParams;

/**
 * @brief 音频变速参数
 */
typedef struct {
    double tempo;           ///< 节奏变化 (0.5 ~ 2.0)
    double pitch;           ///< 音调变化 (半音)
    double rate;            ///< 速率变化
    int32_t preserve_pitch; ///< 保持音调
} TimeStretchParams;

/* ============================================================================
 * 滤镜图管理
 * ============================================================================ */

/**
 * @brief 创建滤镜图
 * @param type 滤镜类型
 * @return 滤镜图指针，失败返回NULL
 */
MediaFilterGraph* media_filter_graph_create(MediaFilterType type);

/**
 * @brief 释放滤镜图
 * @param graph 滤镜图指针
 */
void media_filter_graph_free(MediaFilterGraph *graph);

/**
 * @brief 配置滤镜图
 * @param graph 滤镜图指针
 * @param params 输入参数（视频或音频）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_filter_graph_config(MediaFilterGraph *graph, 
                                          const void *params);

/**
 * @brief 解析滤镜描述字符串
 * @param graph 滤镜图指针
 * @param filter_desc 滤镜描述字符串
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_filter_graph_parse(MediaFilterGraph *graph, 
                                         const char *filter_desc);

/**
 * @brief 添加滤镜到滤镜图
 * @param graph 滤镜图指针
 * @param filter_name 滤镜名称
 * @param filter_args 滤镜参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_graph_add_filter(MediaFilterGraph *graph, 
                                                   const char *filter_name, 
                                                   const char *filter_args);

/**
 * @brief 连接滤镜
 * @param src 源滤镜上下文
 * @param src_pad 源输出端口
 * @param dst 目标滤镜上下文
 * @param dst_pad 目标输入端口
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_filter_link(MediaFilterContext *src, int32_t src_pad, 
                                  MediaFilterContext *dst, int32_t dst_pad);

/**
 * @brief 处理帧
 * @param graph 滤镜图指针
 * @param input_frame 输入帧
 * @param output_frame 输出帧（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_filter_graph_process(MediaFilterGraph *graph, 
                                           const MediaFrame *input_frame, 
                                           MediaFrame *output_frame);

/**
 * @brief 获取输出帧
 * @param graph 滤镜图指针
 * @param output_frame 输出帧（输出）
 * @return 成功返回MEDIA_SUCCESS
 */
MediaErrorCode media_filter_graph_get_frame(MediaFilterGraph *graph, 
                                             MediaFrame *output_frame);

/* ============================================================================
 * 视频滤镜便捷函数
 * ============================================================================ */

/**
 * @brief 创建色彩调整滤镜
 * @param params 色彩调整参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_color_adjust(const ColorAdjustParams *params);

/**
 * @brief 创建几何变换滤镜
 * @param params 几何变换参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_geometric(const GeometricParams *params);

/**
 * @brief 创建模糊滤镜
 * @param params 模糊参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_blur(const BlurParams *params);

/**
 * @brief 创建锐化滤镜
 * @param params 锐化参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_sharpen(const SharpenParams *params);

/**
 * @brief 创建去噪滤镜
 * @param params 去噪参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_denoise(const DenoiseParams *params);

/**
 * @brief 创建边缘检测滤镜
 * @param params 边缘检测参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_edge_detect(const EdgeDetectParams *params);

/**
 * @brief 创建色彩空间转换滤镜
 * @param params 色彩空间参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_colorspace(const ColorSpaceParams *params);

/**
 * @brief 创建叠加滤镜
 * @param overlay 叠加图像路径或帧
 * @param x X坐标
 * @param y Y坐标
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_overlay(const char *overlay, 
                                          int32_t x, int32_t y);

/**
 * @brief 创建文字叠加滤镜
 * @param text 文字内容
 * @param x X坐标
 * @param y Y坐标
 * @param font_size 字体大小
 * @param font_color 字体颜色
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_drawtext(const char *text, 
                                           int32_t x, int32_t y, 
                                           int32_t font_size, 
                                           const char *font_color);

/**
 * @brief 创建淡入淡出滤镜
 * @param type 类型 (0=淡入, 1=淡出)
 * @param start_time 起始时间（秒）
 * @param duration 持续时间（秒）
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_fade(int32_t type, 
                                       double start_time, 
                                       double duration);

/* ============================================================================
 * 音频滤镜便捷函数
 * ============================================================================ */

/**
 * @brief 创建均衡器滤镜
 * @param params 均衡器参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_equalizer(const EqualizerParams *params);

/**
 * @brief 创建混响滤镜
 * @param params 混响参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_reverb(const ReverbParams *params);

/**
 * @brief 创建回声滤镜
 * @param params 回声参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_echo(const EchoParams *params);

/**
 * @brief 创建音频降噪滤镜
 * @param params 降噪参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_audio_denoise(const AudioDenoiseParams *params);

/**
 * @brief 创建音频压缩滤镜
 * @param params 压缩参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_compressor(const CompressorParams *params);

/**
 * @brief 创建音频标准化滤镜
 * @param params 标准化参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_normalize(const NormalizeParams *params);

/**
 * @brief 创建音频变速滤镜
 * @param params 变速参数
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_time_stretch(const TimeStretchParams *params);

/**
 * @brief 创建音量调节滤镜
 * @param volume 音量 (0.0 ~ 2.0)
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_volume(double volume);

/**
 * @brief 创建声道混合滤镜
 * @param input_layout 输入声道布局
 * @param output_layout 输出声道布局
 * @return 滤镜上下文指针，失败返回NULL
 */
MediaFilterContext* media_filter_channel_mix(int32_t input_layout, 
                                              int32_t output_layout);

/* ============================================================================
 * 滤镜参数默认值
 * ============================================================================ */

/**
 * @brief 获取默认色彩调整参数
 * @param params 参数结构体指针
 */
void media_filter_color_adjust_params_default(ColorAdjustParams *params);

/**
 * @brief 获取默认几何变换参数
 * @param params 参数结构体指针
 */
void media_filter_geometric_params_default(GeometricParams *params);

/**
 * @brief 获取默认模糊参数
 * @param params 参数结构体指针
 */
void media_filter_blur_params_default(BlurParams *params);

/**
 * @brief 获取默认锐化参数
 * @param params 参数结构体指针
 */
void media_filter_sharpen_params_default(SharpenParams *params);

/**
 * @brief 获取默认均衡器参数
 * @param params 参数结构体指针
 */
void media_filter_equalizer_params_default(EqualizerParams *params);

/**
 * @brief 获取默认混响参数
 * @param params 参数结构体指针
 */
void media_filter_reverb_params_default(ReverbParams *params);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_FILTER_H */
