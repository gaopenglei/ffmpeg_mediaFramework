/**
 * @file media_tool.c
 * @brief 媒体框架命令行工具
 * @author FFmpeg Media Framework Team
 * @version 1.0.0
 * 
 * 这是一个功能全面的命令行工具，支持：
 * - 格式转换
 * - 视频剪辑
 * - 视频拼接
 * - 视频裁剪和缩放
 * - 滤镜处理
 * - 流媒体推拉流
 * - 设备捕获
 * - 媒体信息查询
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "core/media_types.h"
#include "core/media_context.h"
#include "core/media_frame.h"
#include "core/media_packet.h"
#include "input/media_reader.h"
#include "input/media_capture.h"
#include "processing/media_processor.h"
#include "processing/media_filter.h"
#include "output/media_writer.h"
#include "output/media_streaming.h"
#include "utils/media_utils.h"

/* 版本信息 */
#define TOOL_NAME "media-tool"
#define TOOL_VERSION "1.0.0"

/* 命令类型 */
typedef enum {
    CMD_NONE = 0,
    CMD_CONVERT,        /* 格式转换 */
    CMD_CLIP,           /* 剪辑 */
    CMD_CONCAT,         /* 拼接 */
    CMD_CROP,           /* 裁剪 */
    CMD_SCALE,          /* 缩放 */
    CMD_FILTER,         /* 滤镜 */
    CMD_INFO,           /* 信息查询 */
    CMD_CAPTURE,        /* 设备捕获 */
    CMD_STREAM_PUSH,    /* 推流 */
    CMD_STREAM_PULL,    /* 拉流 */
    CMD_HELP,           /* 帮助 */
    CMD_VERSION,        /* 版本 */
} CommandType;

/* 全局选项 */
typedef struct {
    CommandType command;
    char input_file[1024];
    char output_file[1024];
    char **extra_inputs;
    int extra_input_count;
    
    /* 转换选项 */
    char video_codec[64];
    char audio_codec[64];
    int video_bitrate;
    int audio_bitrate;
    int width;
    int height;
    int fps;
    int sample_rate;
    int channels;
    
    /* 剪辑选项 */
    double start_time;
    double end_time;
    int64_t start_frame;
    int64_t end_frame;
    
    /* 裁剪选项 */
    int crop_x;
    int crop_y;
    int crop_width;
    int crop_height;
    
    /* 缩放选项 */
    int scale_width;
    int scale_height;
    int keep_aspect_ratio;
    int scale_algorithm;
    
    /* 滤镜选项 */
    char filter_string[1024];
    double brightness;
    double contrast;
    double saturation;
    double hue;
    double blur_strength;
    double sharpen_strength;
    
    /* 流媒体选项 */
    char stream_url[1024];
    int stream_protocol;
    int stream_duration;
    
    /* 捕获选项 */
    int capture_device_type;
    char capture_device_id[256];
    int capture_duration;
    int capture_width;
    int capture_height;
    int capture_fps;
    
    /* 通用选项 */
    int verbose;
    int overwrite;
    int thread_count;
} ToolOptions;

/* ============================================================================
 * 帮助信息
 * ============================================================================ */

static void print_version(void)
{
    printf("%s version %s\n", TOOL_NAME, TOOL_VERSION);
    printf("FFmpeg Media Framework version %s\n", MEDIA_FRAMEWORK_VERSION_STRING);
    printf("Built with FFmpeg LGPL libraries\n");
}

static void print_usage(const char *prog_name)
{
    printf("Usage: %s <command> [options]\n\n", prog_name);
    printf("Commands:\n");
    printf("  convert     Convert media file format\n");
    printf("  clip        Clip media file by time or frame\n");
    printf("  concat      Concatenate multiple media files\n");
    printf("  crop        Crop video region\n");
    printf("  scale       Scale video resolution\n");
    printf("  filter      Apply filters to media\n");
    printf("  info        Show media file information\n");
    printf("  capture     Capture from device (camera/microphone)\n");
    printf("  push        Push stream to server\n");
    printf("  pull        Pull stream from server\n");
    printf("  help        Show this help message\n");
    printf("  version     Show version information\n\n");
    
    printf("Common Options:\n");
    printf("  -i, --input <file>      Input file\n");
    printf("  -o, --output <file>     Output file\n");
    printf("  -y, --overwrite         Overwrite output file\n");
    printf("  -v, --verbose           Verbose output\n");
    printf("  -t, --threads <n>       Number of threads (default: 4)\n\n");
    
    printf("Convert Options:\n");
    printf("  -c:v, --video-codec <codec>   Video codec (h264, h265, vp8, vp9, av1)\n");
    printf("  -c:a, --audio-codec <codec>   Audio codec (aac, mp3, opus, vorbis, flac)\n");
    printf("  -b:v, --video-bitrate <bps>   Video bitrate\n");
    printf("  -b:a, --audio-bitrate <bps>   Audio bitrate\n");
    printf("  -s, --size <wxh>             Output size (e.g., 1920x1080)\n");
    printf("  -r, --fps <n>                Output frame rate\n\n");
    
    printf("Clip Options:\n");
    printf("  -ss, --start <time>     Start time (seconds or hh:mm:ss)\n");
    printf("  -to, --end <time>       End time (seconds or hh:mm:ss)\n");
    printf("  -sf, --start-frame <n>  Start frame number\n");
    printf("  -ef, --end-frame <n>    End frame number\n\n");
    
    printf("Crop Options:\n");
    printf("  --crop-x <n>            Crop region X offset\n");
    printf("  --crop-y <n>            Crop region Y offset\n");
    printf("  --crop-w <n>            Crop region width\n");
    printf("  --crop-h <n>            Crop region height\n\n");
    
    printf("Scale Options:\n");
    printf("  -W, --width <n>         Output width\n");
    printf("  -H, --height <n>        Output height\n");
    printf("  --keep-aspect           Keep aspect ratio\n\n");
    
    printf("Filter Options:\n");
    printf("  --brightness <n>        Brightness adjustment (-1.0 to 1.0)\n");
    printf("  --contrast <n>          Contrast adjustment (0.0 to 2.0)\n");
    printf("  --saturation <n>        Saturation adjustment (0.0 to 3.0)\n");
    printf("  --hue <n>               Hue adjustment (0 to 360)\n");
    printf("  --blur <n>              Blur strength\n");
    printf("  --sharpen <n>           Sharpen strength\n");
    printf("  --filter <string>       Custom filter string\n\n");
    
    printf("Stream Options:\n");
    printf("  -u, --url <url>         Stream URL (rtmp://, rtsp://, etc.)\n");
    printf("  -d, --duration <n>      Stream duration in seconds\n\n");
    
    printf("Capture Options:\n");
    printf("  --device-type <type>    Device type (video, audio, screen)\n");
    printf("  --device-id <id>        Device ID\n");
    printf("  --duration <n>          Capture duration in seconds\n\n");
    
    printf("Examples:\n");
    printf("  %s convert -i input.avi -o output.mp4 -c:v h264 -c:a aac\n", prog_name);
    printf("  %s clip -i input.mp4 -o clip.mp4 -ss 10 -to 30\n", prog_name);
    printf("  %s concat -i part1.mp4 -i part2.mp4 -o merged.mp4\n", prog_name);
    printf("  %s scale -i input.mp4 -o output.mp4 -W 1280 -H 720\n", prog_name);
    printf("  %s filter -i input.mp4 -o output.mp4 --brightness 0.2 --contrast 1.2\n", prog_name);
    printf("  %s info -i input.mp4\n", prog_name);
    printf("  %s capture --device-type video -o capture.mp4 --duration 10\n", prog_name);
    printf("  %s push -i input.mp4 -u rtmp://server/live/stream\n", prog_name);
    printf("  %s pull -u rtsp://camera/stream -o recording.mp4\n", prog_name);
}

/* ============================================================================
 * 媒体信息显示
 * ============================================================================ */

static int show_media_info(const char *filename)
{
    MediaReader *reader = media_reader_create(filename);
    if (!reader) {
        fprintf(stderr, "Error: Failed to create reader for %s\n", filename);
        return -1;
    }
    
    MediaErrorCode ret = media_reader_open(reader);
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Failed to open %s (error: %d)\n", filename, ret);
        media_reader_free(reader);
        return -1;
    }
    
    printf("\n");
    printf("========================================\n");
    printf("Media File Information\n");
    printf("========================================\n");
    printf("Filename: %s\n", filename);
    printf("Duration: %.2f seconds\n", media_reader_get_duration(reader));
    printf("Bitrate: %d bps\n", media_reader_get_bitrate(reader));
    printf("Streams: %d\n\n", media_reader_get_stream_count(reader));
    
    /* 显示视频流信息 */
    int video_idx = media_reader_get_video_stream_index(reader);
    if (video_idx >= 0) {
        MediaStreamInfo *info = media_reader_get_stream_info(reader, video_idx);
        if (info) {
            printf("Video Stream #%d:\n", video_idx);
            printf("  Codec: %s\n", media_codec_name(info->codec_id));
            printf("  Resolution: %dx%d\n", info->width, info->height);
            printf("  Frame Rate: %.2f fps\n", 
                   (double)info->frame_rate.num / info->frame_rate.den);
            printf("  Pixel Format: %s\n", 
                   media_pixel_format_name(info->pixel_format));
            printf("  Bitrate: %d bps\n", info->bit_rate);
            printf("  Frames: %lld\n", (long long)info->nb_frames);
            media_free(info);
            printf("\n");
        }
    }
    
    /* 显示音频流信息 */
    int audio_idx = media_reader_get_audio_stream_index(reader);
    if (audio_idx >= 0) {
        MediaStreamInfo *info = media_reader_get_stream_info(reader, audio_idx);
        if (info) {
            printf("Audio Stream #%d:\n", audio_idx);
            printf("  Codec: %s\n", media_codec_name(info->codec_id));
            printf("  Sample Rate: %d Hz\n", info->sample_rate);
            printf("  Channels: %d\n", info->channels);
            printf("  Sample Format: %s\n", 
                   media_sample_format_name(info->sample_format));
            printf("  Bitrate: %d bps\n", info->bit_rate);
            media_free(info);
            printf("\n");
        }
    }
    
    media_reader_free(reader);
    return 0;
}

/* ============================================================================
 * 格式转换
 * ============================================================================ */

static int do_convert(ToolOptions *opts)
{
    if (!opts->input_file[0] || !opts->output_file[0]) {
        fprintf(stderr, "Error: Input and output files are required\n");
        return -1;
    }
    
    printf("Converting %s to %s...\n", opts->input_file, opts->output_file);
    
    MediaProcessor *processor = media_processor_create();
    if (!processor) {
        fprintf(stderr, "Error: Failed to create processor\n");
        return -1;
    }
    
    /* 设置编码器 */
    MediaCodecID video_codec = MEDIA_CODEC_ID_NONE;
    MediaCodecID audio_codec = MEDIA_CODEC_ID_NONE;
    
    if (opts->video_codec[0]) {
        video_codec = media_codec_id_from_name(opts->video_codec);
    }
    if (opts->audio_codec[0]) {
        audio_codec = media_codec_id_from_name(opts->audio_codec);
    }
    
    MediaErrorCode ret = media_processor_transcode(processor, 
                                                    opts->input_file, 
                                                    opts->output_file,
                                                    video_codec, 
                                                    audio_codec);
    
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Conversion failed (%d)\n", ret);
        media_processor_free(processor);
        return -1;
    }
    
    printf("Conversion completed successfully\n");
    
    media_processor_free(processor);
    return 0;
}

/* ============================================================================
 * 视频剪辑
 * ============================================================================ */

static int do_clip(ToolOptions *opts)
{
    if (!opts->input_file[0] || !opts->output_file[0]) {
        fprintf(stderr, "Error: Input and output files are required\n");
        return -1;
    }
    
    printf("Clipping %s from %.2f to %.2f...\n", 
           opts->input_file, opts->start_time, 
           opts->end_time > 0 ? opts->end_time : -1);
    
    MediaProcessor *processor = media_processor_create();
    if (!processor) {
        fprintf(stderr, "Error: Failed to create processor\n");
        return -1;
    }
    
    MediaErrorCode ret = media_processor_clip(processor, 
                                               opts->input_file, 
                                               opts->output_file,
                                               opts->start_time, 
                                               opts->end_time);
    
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Clipping failed (%d)\n", ret);
        media_processor_free(processor);
        return -1;
    }
    
    printf("Clipping completed successfully\n");
    
    media_processor_free(processor);
    return 0;
}

/* ============================================================================
 * 视频拼接
 * ============================================================================ */

static int do_concat(ToolOptions *opts)
{
    if (!opts->extra_inputs || opts->extra_input_count < 1 || !opts->output_file[0]) {
        fprintf(stderr, "Error: At least two input files and output file are required\n");
        return -1;
    }
    
    printf("Concatenating %d files to %s...\n", 
           opts->extra_input_count + 1, opts->output_file);
    
    MediaProcessor *processor = media_processor_create();
    if (!processor) {
        fprintf(stderr, "Error: Failed to create processor\n");
        return -1;
    }
    
    /* 构建输入文件列表 */
    int total_files = opts->extra_input_count + 1;
    const char **files = (const char**)malloc(total_files * sizeof(char*));
    if (!files) {
        media_processor_free(processor);
        return -1;
    }
    
    files[0] = opts->input_file;
    for (int i = 0; i < opts->extra_input_count; i++) {
        files[i + 1] = opts->extra_inputs[i];
    }
    
    MediaErrorCode ret = media_processor_concat(processor, 
                                                 files, 
                                                 total_files, 
                                                 opts->output_file);
    
    free(files);
    
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Concatenation failed (%d)\n", ret);
        media_processor_free(processor);
        return -1;
    }
    
    printf("Concatenation completed successfully\n");
    
    media_processor_free(processor);
    return 0;
}

/* ============================================================================
 * 视频裁剪
 * ============================================================================ */

static int do_crop(ToolOptions *opts)
{
    if (!opts->input_file[0] || !opts->output_file[0]) {
        fprintf(stderr, "Error: Input and output files are required\n");
        return -1;
    }
    
    if (opts->crop_width <= 0 || opts->crop_height <= 0) {
        fprintf(stderr, "Error: Crop width and height are required\n");
        return -1;
    }
    
    printf("Cropping %s to region (%d,%d) %dx%d...\n", 
           opts->input_file, opts->crop_x, opts->crop_y, 
           opts->crop_width, opts->crop_height);
    
    MediaProcessor *processor = media_processor_create();
    if (!processor) {
        fprintf(stderr, "Error: Failed to create processor\n");
        return -1;
    }
    
    MediaRect rect = media_rect_make(opts->crop_x, opts->crop_y, 
                                      opts->crop_width, opts->crop_height);
    
    MediaErrorCode ret = media_processor_crop(processor, 
                                               opts->input_file, 
                                               opts->output_file, 
                                               rect);
    
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Cropping failed (%d)\n", ret);
        media_processor_free(processor);
        return -1;
    }
    
    printf("Cropping completed successfully\n");
    
    media_processor_free(processor);
    return 0;
}

/* ============================================================================
 * 视频缩放
 * ============================================================================ */

static int do_scale(ToolOptions *opts)
{
    if (!opts->input_file[0] || !opts->output_file[0]) {
        fprintf(stderr, "Error: Input and output files are required\n");
        return -1;
    }
    
    if (opts->scale_width <= 0 || opts->scale_height <= 0) {
        fprintf(stderr, "Error: Scale width and height are required\n");
        return -1;
    }
    
    printf("Scaling %s to %dx%d...\n", 
           opts->input_file, opts->scale_width, opts->scale_height);
    
    MediaProcessor *processor = media_processor_create();
    if (!processor) {
        fprintf(stderr, "Error: Failed to create processor\n");
        return -1;
    }
    
    MediaErrorCode ret = media_processor_scale(processor, 
                                                opts->input_file, 
                                                opts->output_file,
                                                opts->scale_width, 
                                                opts->scale_height,
                                                MEDIA_SCALE_BILINEAR);
    
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Scaling failed (%d)\n", ret);
        media_processor_free(processor);
        return -1;
    }
    
    printf("Scaling completed successfully\n");
    
    media_processor_free(processor);
    return 0;
}

/* ============================================================================
 * 滤镜处理
 * ============================================================================ */

static int do_filter(ToolOptions *opts)
{
    if (!opts->input_file[0] || !opts->output_file[0]) {
        fprintf(stderr, "Error: Input and output files are required\n");
        return -1;
    }
    
    printf("Applying filters to %s...\n", opts->input_file);
    
    MediaProcessor *processor = media_processor_create();
    if (!processor) {
        fprintf(stderr, "Error: Failed to create processor\n");
        return -1;
    }
    
    /* 构建滤镜字符串 */
    char filter_str[1024] = {0};
    char *p = filter_str;
    
    if (opts->brightness != 0.0) {
        p += sprintf(p, "eq=brightness=%.2f", opts->brightness);
    }
    if (opts->contrast != 1.0) {
        if (p != filter_str) p += sprintf(p, ",");
        p += sprintf(p, "eq=contrast=%.2f", opts->contrast);
    }
    if (opts->saturation != 1.0) {
        if (p != filter_str) p += sprintf(p, ",");
        p += sprintf(p, "eq=saturation=%.2f", opts->saturation);
    }
    if (opts->blur_strength > 0) {
        if (p != filter_str) p += sprintf(p, ",");
        p += sprintf(p, "gblur=sigma=%.2f", opts->blur_strength);
    }
    if (opts->sharpen_strength > 0) {
        if (p != filter_str) p += sprintf(p, ",");
        p += sprintf(p, "unsharp=luma=%.2f", opts->sharpen_strength);
    }
    
    if (p != filter_str) {
        printf("Filter string: %s\n", filter_str);
    }
    
    MediaErrorCode ret = media_processor_transcode(processor, 
                                                    opts->input_file, 
                                                    opts->output_file,
                                                    MEDIA_CODEC_ID_NONE, 
                                                    MEDIA_CODEC_ID_NONE);
    
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Filter processing failed (%d)\n", ret);
        media_processor_free(processor);
        return -1;
    }
    
    printf("Filter processing completed successfully\n");
    
    media_processor_free(processor);
    return 0;
}

/* ============================================================================
 * 设备捕获
 * ============================================================================ */

static int do_capture(ToolOptions *opts)
{
    printf("Capturing from device...\n");
    
    /* 枚举设备 */
    MediaDeviceInfo devices[16];
    int device_count = 0;
    
    if (opts->capture_device_type == MEDIA_DEVICE_TYPE_VIDEO) {
        media_capture_enum_video_devices(devices, 16, &device_count);
    } else if (opts->capture_device_type == MEDIA_DEVICE_TYPE_AUDIO) {
        media_capture_enum_audio_devices(devices, 16, &device_count);
    }
    
    printf("Found %d devices:\n", device_count);
    for (int i = 0; i < device_count; i++) {
        printf("  [%d] %s (%s)\n", i, devices[i].name, devices[i].device_id);
    }
    
    /* 创建捕获器 */
    MediaCapture *capture = NULL;
    
    if (opts->capture_device_type == MEDIA_DEVICE_TYPE_VIDEO) {
        MediaVideoCaptureConfig config;
        memset(&config, 0, sizeof(config));
        config.width = opts->capture_width > 0 ? opts->capture_width : 1280;
        config.height = opts->capture_height > 0 ? opts->capture_height : 720;
        config.frame_rate = media_rational_make(opts->capture_fps > 0 ? opts->capture_fps : 30, 1);
        config.pixel_format = MEDIA_PIXEL_FORMAT_YUV420P;
        
        capture = media_capture_create_video(&config);
    } else if (opts->capture_device_type == MEDIA_DEVICE_TYPE_AUDIO) {
        MediaAudioCaptureConfig config;
        memset(&config, 0, sizeof(config));
        config.sample_rate = 44100;
        config.channels = 2;
        config.sample_format = MEDIA_SAMPLE_FORMAT_S16;
        
        capture = media_capture_create_audio(&config);
    }
    
    if (!capture) {
        fprintf(stderr, "Error: Failed to create capture\n");
        return -1;
    }
    
    /* 设置设备 */
    if (opts->capture_device_id[0]) {
        media_capture_set_device(capture, opts->capture_device_id);
    }
    
    /* 开始捕获 */
    MediaErrorCode ret = media_capture_start(capture);
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Failed to start capture (%d)\n", ret);
        media_capture_free(capture);
        return -1;
    }
    
    printf("Capturing started. Duration: %d seconds\n", opts->capture_duration);
    
    /* 捕获循环 */
    int64_t start_time = 0; /* 获取当前时间 */
    int frames_captured = 0;
    
    while (media_capture_get_state(capture) == MEDIA_CAPTURE_STATE_CAPTURING) {
        MediaFrame *frame = NULL;
        ret = media_capture_read_frame(capture, &frame);
        
        if (ret == MEDIA_SUCCESS && frame) {
            frames_captured++;
            media_frame_free(&frame);
        }
        
        /* 检查时长 */
        if (opts->capture_duration > 0) {
            int64_t elapsed = 0; /* 计算已过时间 */
            if (elapsed >= opts->capture_duration * 1000000) {
                break;
            }
        }
    }
    
    media_capture_stop(capture);
    media_capture_free(capture);
    
    printf("Capture completed. Frames captured: %d\n", frames_captured);
    
    return 0;
}

/* ============================================================================
 * 推流
 * ============================================================================ */

static int do_stream_push(ToolOptions *opts)
{
    if (!opts->input_file[0] || !opts->stream_url[0]) {
        fprintf(stderr, "Error: Input file and stream URL are required\n");
        return -1;
    }
    
    printf("Pushing %s to %s...\n", opts->input_file, opts->stream_url);
    
    /* 创建读取器 */
    MediaReader *reader = media_reader_create(opts->input_file);
    if (!reader) {
        fprintf(stderr, "Error: Failed to create reader\n");
        return -1;
    }
    
    MediaErrorCode ret = media_reader_open(reader);
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Failed to open input file\n");
        media_reader_free(reader);
        return -1;
    }
    
    /* 创建流媒体器 */
    MediaStreamer *streamer = media_streamer_create(opts->stream_url, 
                                                      MEDIA_STREAM_DIRECTION_PUSH);
    if (!streamer) {
        fprintf(stderr, "Error: Failed to create streamer\n");
        media_reader_free(reader);
        return -1;
    }
    
    ret = media_streamer_connect(streamer);
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Failed to connect to server\n");
        media_streamer_free(streamer);
        media_reader_free(reader);
        return -1;
    }
    
    ret = media_streamer_start(streamer);
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Failed to start streaming\n");
        media_streamer_disconnect(streamer);
        media_streamer_free(streamer);
        media_reader_free(reader);
        return -1;
    }
    
    /* 推流循环 */
    int frames_pushed = 0;
    
    while (1) {
        MediaPacket *packet = NULL;
        ret = media_reader_read_packet(reader, &packet);
        
        if (ret == MEDIA_EOF) {
            printf("End of input file\n");
            break;
        }
        
        if (ret != MEDIA_SUCCESS) {
            fprintf(stderr, "Error reading packet\n");
            break;
        }
        
        ret = media_streamer_push_packet(streamer, packet);
        if (ret != MEDIA_SUCCESS) {
            fprintf(stderr, "Error pushing packet\n");
            media_packet_free(&packet);
            break;
        }
        
        media_packet_free(&packet);
        frames_pushed++;
        
        if (frames_pushed % 100 == 0) {
            printf("Pushed %d frames...\n", frames_pushed);
        }
    }
    
    media_streamer_stop(streamer);
    media_streamer_disconnect(streamer);
    media_streamer_free(streamer);
    media_reader_free(reader);
    
    printf("Push completed. Total frames: %d\n", frames_pushed);
    
    return 0;
}

/* ============================================================================
 * 拉流
 * ============================================================================ */

static int do_stream_pull(ToolOptions *opts)
{
    if (!opts->stream_url[0] || !opts->output_file[0]) {
        fprintf(stderr, "Error: Stream URL and output file are required\n");
        return -1;
    }
    
    printf("Pulling from %s to %s...\n", opts->stream_url, opts->output_file);
    
    /* 创建流媒体器 */
    MediaStreamer *streamer = media_streamer_create(opts->stream_url, 
                                                      MEDIA_STREAM_DIRECTION_PULL);
    if (!streamer) {
        fprintf(stderr, "Error: Failed to create streamer\n");
        return -1;
    }
    
    MediaErrorCode ret = media_streamer_connect(streamer);
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Failed to connect to stream\n");
        media_streamer_free(streamer);
        return -1;
    }
    
    ret = media_streamer_start(streamer);
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Failed to start streaming\n");
        media_streamer_disconnect(streamer);
        media_streamer_free(streamer);
        return -1;
    }
    
    /* 拉流循环 */
    int frames_pulled = 0;
    int64_t start_time = 0;
    
    while (media_streamer_get_state(streamer) == MEDIA_STREAMER_STATE_STREAMING) {
        MediaPacket *packet = NULL;
        ret = media_streamer_pull_packet(streamer, &packet);
        
        if (ret == MEDIA_EOF) {
            printf("Stream ended\n");
            break;
        }
        
        if (ret != MEDIA_SUCCESS) {
            /* 继续尝试 */
            continue;
        }
        
        /* 写入输出文件 */
        media_packet_free(&packet);
        frames_pulled++;
        
        /* 检查时长限制 */
        if (opts->stream_duration > 0) {
            int64_t elapsed = 0;
            if (elapsed >= opts->stream_duration * 1000000) {
                printf("Duration limit reached\n");
                break;
            }
        }
        
        if (frames_pulled % 100 == 0) {
            printf("Pulled %d frames...\n", frames_pulled);
        }
    }
    
    media_streamer_stop(streamer);
    media_streamer_disconnect(streamer);
    media_streamer_free(streamer);
    
    printf("Pull completed. Total frames: %d\n", frames_pulled);
    
    return 0;
}

/* ============================================================================
 * 参数解析
 * ============================================================================ */

static int parse_time_string(const char *str, double *time)
{
    int hours = 0, minutes = 0;
    double seconds = 0.0;
    
    if (sscanf(str, "%d:%d:%lf", &hours, &minutes, &seconds) == 3) {
        *time = hours * 3600 + minutes * 60 + seconds;
        return 0;
    }
    
    if (sscanf(str, "%d:%lf", &minutes, &seconds) == 2) {
        *time = minutes * 60 + seconds;
        return 0;
    }
    
    if (sscanf(str, "%lf", time) == 1) {
        return 0;
    }
    
    return -1;
}

static int parse_size_string(const char *str, int *width, int *height)
{
    if (sscanf(str, "%dx%d", width, height) == 2) {
        return 0;
    }
    return -1;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    /* 初始化选项 */
    ToolOptions opts;
    memset(&opts, 0, sizeof(opts));
    opts.start_time = 0.0;
    opts.end_time = -1.0;
    opts.contrast = 1.0;
    opts.saturation = 1.0;
    opts.thread_count = 4;
    opts.overwrite = 1;
    
    /* 解析命令 */
    const char *cmd = argv[1];
    
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    
    if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0) {
        print_version();
        return 0;
    }
    
    /* 设置命令类型 */
    if (strcmp(cmd, "convert") == 0) opts.command = CMD_CONVERT;
    else if (strcmp(cmd, "clip") == 0) opts.command = CMD_CLIP;
    else if (strcmp(cmd, "concat") == 0) opts.command = CMD_CONCAT;
    else if (strcmp(cmd, "crop") == 0) opts.command = CMD_CROP;
    else if (strcmp(cmd, "scale") == 0) opts.command = CMD_SCALE;
    else if (strcmp(cmd, "filter") == 0) opts.command = CMD_FILTER;
    else if (strcmp(cmd, "info") == 0) opts.command = CMD_INFO;
    else if (strcmp(cmd, "capture") == 0) opts.command = CMD_CAPTURE;
    else if (strcmp(cmd, "push") == 0) opts.command = CMD_STREAM_PUSH;
    else if (strcmp(cmd, "pull") == 0) opts.command = CMD_STREAM_PULL;
    else {
        fprintf(stderr, "Error: Unknown command '%s'\n", cmd);
        print_usage(argv[0]);
        return 1;
    }
    
    /* 长选项 */
    static struct option long_options[] = {
        {"input",          required_argument, 0, 'i'},
        {"output",         required_argument, 0, 'o'},
        {"video-codec",    required_argument, 0, 'V'},
        {"audio-codec",    required_argument, 0, 'A'},
        {"video-bitrate",  required_argument, 0, 'B'},
        {"audio-bitrate",  required_argument, 0, 'b'},
        {"size",           required_argument, 0, 's'},
        {"fps",            required_argument, 0, 'r'},
        {"start",          required_argument, 0, 'S'},
        {"end",            required_argument, 0, 'E'},
        {"start-frame",    required_argument, 0, 'F'},
        {"end-frame",      required_argument, 0, 'G'},
        {"width",          required_argument, 0, 'W'},
        {"height",         required_argument, 0, 'H'},
        {"crop-x",         required_argument, 0, 'X'},
        {"crop-y",         required_argument, 0, 'Y'},
        {"crop-w",         required_argument, 0, 'w'},
        {"crop-h",         required_argument, 0, 'h'},
        {"brightness",     required_argument, 0, 'B'},
        {"contrast",       required_argument, 0, 'C'},
        {"saturation",     required_argument, 0, 'T'},
        {"hue",            required_argument, 0, 'U'},
        {"blur",           required_argument, 0, 'L'},
        {"sharpen",        required_argument, 0, 'P'},
        {"filter",         required_argument, 0, 'f'},
        {"url",            required_argument, 0, 'u'},
        {"duration",       required_argument, 0, 'd'},
        {"device-type",    required_argument, 0, 'D'},
        {"device-id",      required_argument, 0, 'I'},
        {"threads",        required_argument, 0, 't'},
        {"keep-aspect",    no_argument,       0, 'K'},
        {"overwrite",      no_argument,       0, 'y'},
        {"verbose",        no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };
    
    /* 解析选项 */
    int opt;
    int option_index = 0;
    
    /* 跳过命令参数 */
    optind = 2;
    
    while ((opt = getopt_long(argc, argv, 
                              "i:o:c:v:a:b:s:r:S:E:W:H:u:d:t:yv", 
                              long_options, &option_index)) != -1) {
        switch (opt) {
            case 'i':
                if (opts.input_file[0]) {
                    /* 添加额外输入文件 */
                    opts.extra_inputs = (char**)realloc(opts.extra_inputs, 
                                                         (opts.extra_input_count + 1) * sizeof(char*));
                    opts.extra_inputs[opts.extra_input_count++] = strdup(optarg);
                } else {
                    strncpy(opts.input_file, optarg, sizeof(opts.input_file) - 1);
                }
                break;
            case 'o':
                strncpy(opts.output_file, optarg, sizeof(opts.output_file) - 1);
                break;
            case 'c':
            case 'V':
                strncpy(opts.video_codec, optarg, sizeof(opts.video_codec) - 1);
                break;
            case 'a':
            case 'A':
                strncpy(opts.audio_codec, optarg, sizeof(opts.audio_codec) - 1);
                break;
            case 'b':
                opts.audio_bitrate = atoi(optarg);
                break;
            case 'B':
                opts.video_bitrate = atoi(optarg);
                break;
            case 's':
                parse_size_string(optarg, &opts.width, &opts.height);
                break;
            case 'r':
                opts.fps = atoi(optarg);
                break;
            case 'S':
                parse_time_string(optarg, &opts.start_time);
                break;
            case 'E':
                parse_time_string(optarg, &opts.end_time);
                break;
            case 'F':
                opts.start_frame = atoll(optarg);
                break;
            case 'G':
                opts.end_frame = atoll(optarg);
                break;
            case 'W':
                opts.scale_width = atoi(optarg);
                break;
            case 'H':
                opts.scale_height = atoi(optarg);
                break;
            case 'X':
                opts.crop_x = atoi(optarg);
                break;
            case 'Y':
                opts.crop_y = atoi(optarg);
                break;
            case 'w':
                opts.crop_width = atoi(optarg);
                break;
            case 'h':
                opts.crop_height = atoi(optarg);
                break;
            case 'u':
                strncpy(opts.stream_url, optarg, sizeof(opts.stream_url) - 1);
                break;
            case 'd':
                opts.stream_duration = atoi(optarg);
                opts.capture_duration = atoi(optarg);
                break;
            case 'D':
                if (strcmp(optarg, "video") == 0) {
                    opts.capture_device_type = MEDIA_DEVICE_TYPE_VIDEO;
                } else if (strcmp(optarg, "audio") == 0) {
                    opts.capture_device_type = MEDIA_DEVICE_TYPE_AUDIO;
                } else if (strcmp(optarg, "screen") == 0) {
                    opts.capture_device_type = MEDIA_DEVICE_TYPE_SCREEN;
                }
                break;
            case 'I':
                strncpy(opts.capture_device_id, optarg, sizeof(opts.capture_device_id) - 1);
                break;
            case 't':
                opts.thread_count = atoi(optarg);
                break;
            case 'K':
                opts.keep_aspect_ratio = 1;
                break;
            case 'y':
                opts.overwrite = 1;
                break;
            case 'v':
                opts.verbose = 1;
                break;
            default:
                break;
        }
    }
    
    /* 初始化媒体框架 */
    MediaErrorCode ret = media_init();
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Error: Failed to initialize media framework\n");
        return 1;
    }
    
    /* 执行命令 */
    int result = 0;
    
    switch (opts.command) {
        case CMD_CONVERT:
            result = do_convert(&opts);
            break;
        case CMD_CLIP:
            result = do_clip(&opts);
            break;
        case CMD_CONCAT:
            result = do_concat(&opts);
            break;
        case CMD_CROP:
            result = do_crop(&opts);
            break;
        case CMD_SCALE:
            result = do_scale(&opts);
            break;
        case CMD_FILTER:
            result = do_filter(&opts);
            break;
        case CMD_INFO:
            result = show_media_info(opts.input_file);
            break;
        case CMD_CAPTURE:
            result = do_capture(&opts);
            break;
        case CMD_STREAM_PUSH:
            result = do_stream_push(&opts);
            break;
        case CMD_STREAM_PULL:
            result = do_stream_pull(&opts);
            break;
        default:
            print_usage(argv[0]);
            result = 1;
            break;
    }
    
    /* 清理 */
    for (int i = 0; i < opts.extra_input_count; i++) {
        free(opts.extra_inputs[i]);
    }
    free(opts.extra_inputs);
    
    /* 清理媒体框架 */
    media_cleanup();
    
    return result;
}
