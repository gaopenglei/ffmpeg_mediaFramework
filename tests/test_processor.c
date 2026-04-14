/**
 * @file test_processor.c
 * @brief 媒体处理器测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/media_context.h"
#include "processing/media_processor.h"
#include "processing/media_filter.h"
#include "utils/media_utils.h"

void test_processor_create_destroy(void)
{
    printf("Testing processor create/destroy...\n");
    
    MediaProcessor *processor = media_processor_create();
    assert(processor != NULL);
    
    MediaProcessorState state = media_processor_get_state(processor);
    assert(state == MEDIA_PROCESSOR_STATE_IDLE);
    
    media_processor_destroy(&processor);
    assert(processor == NULL);
    
    printf("  Processor create/destroy tests passed!\n");
}

void test_processor_params(void)
{
    printf("Testing processor parameters...\n");
    
    /* 剪辑参数 */
    MediaClipParams clip;
    media_clip_params_default(&clip);
    assert(clip.start_time == 0.0);
    assert(clip.end_time == 0.0);
    
    /* 缩放参数 */
    MediaScaleParams scale;
    media_scale_params_default(&scale);
    assert(scale.width == 0);
    assert(scale.height == 0);
    assert(scale.algorithm == MEDIA_SCALE_BILINEAR);
    
    /* 裁剪参数 */
    MediaCropParams crop;
    media_crop_params_default(&crop);
    assert(crop.rect.x == 0);
    assert(crop.rect.y == 0);
    
    printf("  Processor parameter tests passed!\n");
}

void test_filter_params(void)
{
    printf("Testing filter parameters...\n");
    
    /* 色彩调整参数 */
    ColorAdjustParams color;
    media_filter_color_adjust_params_default(&color);
    assert(color.brightness == 0.0);
    assert(color.contrast == 1.0);
    assert(color.saturation == 1.0);
    
    /* 模糊参数 */
    BlurParams blur;
    media_filter_blur_params_default(&blur);
    assert(blur.radius > 0);
    
    /* 锐化参数 */
    SharpenParams sharpen;
    media_filter_sharpen_params_default(&sharpen);
    assert(sharpen.amount >= 0);
    
    /* 均衡器参数 */
    EqualizerParams eq;
    media_filter_equalizer_params_default(&eq);
    assert(eq.band_count > 0);
    
    /* 混响参数 */
    ReverbParams reverb;
    media_filter_reverb_params_default(&reverb);
    assert(reverb.decay >= 0);
    
    printf("  Filter parameter tests passed!\n");
}

void test_processor_progress(void)
{
    printf("Testing processor progress...\n");
    
    MediaProcessor *processor = media_processor_create();
    assert(processor != NULL);
    
    /* 初始进度应为0 */
    double progress = media_processor_get_progress(processor);
    assert(progress == 0.0);
    
    int64_t frames = media_processor_get_processed_frames(processor);
    assert(frames == 0);
    
    media_processor_destroy(&processor);
    
    printf("  Processor progress tests passed!\n");
}

void test_concat_params(void)
{
    printf("Testing concatenation parameters...\n");
    
    MediaConcatParams concat;
    media_concat_params_default(&concat);
    
    assert(concat.input_count == 0);
    assert(concat.inputs == NULL);
    assert(concat.safe_mode == 1);
    
    printf("  Concatenation parameter tests passed!\n");
}

void test_stream_config(void)
{
    printf("Testing stream configuration...\n");
    
    MediaStreamConfig video_config;
    media_stream_config_default(&video_config, MEDIA_STREAM_TYPE_VIDEO);
    
    assert(video_config.type == MEDIA_STREAM_TYPE_VIDEO);
    assert(video_config.bit_rate > 0);
    assert(video_config.thread_count >= 1);
    
    MediaStreamConfig audio_config;
    media_stream_config_default(&audio_config, MEDIA_STREAM_TYPE_AUDIO);
    
    assert(audio_config.type == MEDIA_STREAM_TYPE_AUDIO);
    assert(audio_config.sample_rate > 0);
    assert(audio_config.channels > 0);
    
    printf("  Stream configuration tests passed!\n");
}

void progress_callback(MediaProcessor *processor, double progress, 
                       int64_t current_frame, int64_t total_frames, 
                       void *user_data)
{
    (void)processor;
    (void)user_data;
    printf("    Progress: %.1f%% (%lld/%lld frames)\n", 
           progress * 100.0, (long long)current_frame, (long long)total_frames);
}

void complete_callback(MediaProcessor *processor, MediaErrorCode result, 
                       void *user_data)
{
    (void)processor;
    (void)user_data;
    if (result == MEDIA_SUCCESS) {
        printf("    Processing completed successfully!\n");
    } else {
        printf("    Processing failed with error: %d\n", result);
    }
}

int main(int argc, char *argv[])
{
    printf("=== Media Processor Tests ===\n\n");
    
    /* 初始化媒体框架 */
    MediaErrorCode ret = media_init();
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Failed to initialize media framework\n");
        return 1;
    }
    
    test_processor_create_destroy();
    test_processor_params();
    test_filter_params();
    test_processor_progress();
    test_concat_params();
    test_stream_config();
    
    /* 如果提供了输入和输出文件，进行实际处理测试 */
    if (argc >= 3) {
        printf("\nTesting transcoding: %s -> %s\n", argv[1], argv[2]);
        
        MediaProcessor *processor = media_processor_create();
        assert(processor != NULL);
        
        /* 设置回调 */
        media_processor_set_progress_callback(processor, progress_callback, NULL);
        media_processor_set_complete_callback(processor, complete_callback, NULL);
        
        /* 执行转码 */
        ret = media_processor_transcode(processor, argv[1], argv[2],
                                        MEDIA_CODEC_ID_H264, MEDIA_CODEC_ID_AAC);
        
        if (ret != MEDIA_SUCCESS) {
            printf("  Transcoding failed: %d\n", ret);
            printf("  Error: %s\n", media_processor_get_error_message(processor));
        }
        
        media_processor_destroy(&processor);
    }
    
    /* 清理 */
    media_cleanup();
    
    printf("\n=== All tests passed! ===\n");
    return 0;
}
